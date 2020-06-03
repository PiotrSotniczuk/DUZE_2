#include <string>
#include "initialize.h"
#include "err.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>
#include <poll.h>
#include <netdb.h>
#include <iostream>
#include <map>
#include "RadioReader.h"
#include <cstring>
#include <chrono>

#define DISCOVER 1
#define IAM 2
#define KEEPALIVE 3
#define AUDIO 4
#define METADATA 6

#define HEAD_SIZE 4

#define BUFF_SIZE 1024

using namespace std;

static bool finishA;
static bool finishB;


void sigint_handler([[maybe_unused]]int signal_num){
	finishA = true;
	finishB = true;
}

void print_cout_cerr(bool metaData,
	int metaInt, int sockA, int timeoutA){

	RadioReader reader(metaData, metaInt, sockA, true);

	pollfd poll_tab[1];
	poll_tab[0].fd = sockA;
	poll_tab[0].events = POLLIN;
	poll_tab[0].revents = 0;

	while (!finishA){
		int ret = poll(poll_tab, 1, timeoutA * 1000);
		// error in poll or timeout exceeded or error in RadioReader
		if(ret <= 0){
			break;
		}
		if ((poll_tab[0].revents & (POLLIN | POLLERR))
			&& reader.readSendChunk(map<struct sockaddr_in,
				clock_t, addr_comp>(), 0) < 0) {
			break;
		}
		poll_tab[0].revents = 0;
	}

	if (close(sockA) < 0) {
		syserr("closing stream socket");
	}
	exit(finishA);
}


int main(int argc, char* argv[]) {

	int sockA;
	string radio_host, radio_resource, portA, portB, multi, name;
	bool metaData = false;
	int timeoutA = 5, timeoutB = 5;
	finishA = false;
	finishB = false;
	set_args(argc, argv, &radio_host, &radio_resource, &portA, &metaData, &timeoutA, &portB,
		&multi, &timeoutB);

	// writing down the request
	string buffer_send = "GET " + radio_resource + " HTTP/1.0\r\nHost: "
		+ radio_host + "\r\n" + "Icy-MetaData:"+ (metaData ? "1" : "0") + "\r\n" +
		+ "Connection: close\r\n\r\n";

	// setting global sock
	sockA = get_socket(radio_host.c_str(), portA.c_str());

	// catch signal
	signal(SIGINT, sigint_handler);

	// send request
	if (write(sockA, buffer_send.c_str(), buffer_send.size()) !=
		buffer_send.size()) {
		syserr("partial / failed write");
	}

	// read header set metaInt
	int metaInt = 0;
	read_header(&metaData, &name, &metaInt, sockA);

	if(portB.empty()){
		// Part A
		print_cout_cerr(metaData, metaInt, sockA, timeoutA);
	}

	// Part B
	// initiate pollfd for receiving message
	struct sockaddr_in server;
	struct pollfd poll_tab[2];
	init_poll(sockA, portB, poll_tab, &server);


	map<struct sockaddr_in, clock_t, addr_comp> clients_map = {};
	struct sockaddr_in client_address;
	auto rcva_len = (socklen_t) sizeof(client_address);

	RadioReader reader(metaData, metaInt, sockA, false);

	short buf_rcv[HEAD_SIZE/2];
	char messageIAM[BUFF_SIZE + HEAD_SIZE];
	strcpy(messageIAM + HEAD_SIZE, name.c_str());
	unsigned short size_snd = strlen(messageIAM + HEAD_SIZE);
	messageIAM[0] = htons(IAM) & 0xff;
	messageIAM[1] = (htons(IAM) >> 8) & 0xff;
	messageIAM[2] = htons(size_snd) & 0xff;
	messageIAM[3] = (htons(size_snd) >> 8) & 0xff;

	while(!finishB) {
		int ret = poll(poll_tab, 2, timeoutA * 1000);
		if(ret <= 0){
			// timeout from server or error in poll
			break;
		}

		// client request
		if (poll_tab[0].revents & (POLLIN | POLLERR)) {
			cerr << "Tak kurna zlapalem clienta\n";
			poll_tab[0].revents = 0;
			if(recvfrom(poll_tab[0].fd, buf_rcv, HEAD_SIZE, 0,
				(struct sockaddr *) &client_address, &rcva_len) < 0){
				// error reading request
				break;
			}

			int type = ntohs(buf_rcv[0]);
			if(ntohs(buf_rcv[1])){
				// bad length
				cerr << "Bad length of message\n";
			}

			if(type == DISCOVER) {
				clock_t start = clock();
				clients_map.emplace(client_address, start);
				sendto(poll_tab[0].fd, messageIAM, size_snd, 0,
					   (struct sockaddr *) &client_address, rcva_len);
			}
			if(type == KEEPALIVE){
				auto it = clients_map.find(client_address);
				if(it != clients_map.end()){
					(*it).second = clock();
				}

			}
			if(type != KEEPALIVE && type != DISCOVER){
				cerr << "odd type\n";
			}


		}
		if(poll_tab[1].revents & (POLLIN | POLLERR)) {
			cerr << "Tak zlapalem server radio\n";
			poll_tab[1].revents = 0;
			reader.readSendChunk(clients_map, poll_tab[0].fd);
			// TODO
			sleep(1);

		}

	}
	close(poll_tab[0].fd);
	close(sockA);

	// if finished by user than ok if not then error
	return finishB;
}