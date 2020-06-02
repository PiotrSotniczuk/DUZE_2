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
#include <set>
#include "RadioReader.h"

#define DISCOVER 1
#define IAM 2
#define KEEPALIVE 3
#define AUDIO 4
#define METADATA 6

#define HEAD_SIZE 4

#define BUFF_SIZE 1024
#define MAX_META_SIZE 4080

using namespace std;

static bool finishA;
static bool finishB;


void sigint_handler([[maybe_unused]]int signal_num){
	finishA = true;
	finishB = true;
}

void print_cout_cerr(const string& meta,
	unsigned long int metaInt, int sockA, int timeoutA){

	bool metaBool;
	if(meta == "0"){
		metaBool = false;
		metaInt = 0;
	}else {
		metaBool = true;
	}

	RadioReader reader(metaBool, metaInt, sockA, true);

	pollfd poll_tab[1];
	poll_tab[0].fd = sockA;
	poll_tab[0].events = POLLIN;
	poll_tab[0].revents = 0;

	while (!finishA){
		int ret = poll(poll_tab, 1, timeoutA * 1000);
		// error in poll or timeout exceeded or error in RadioReader
		if(ret <= 0 || reader.readSendChunk(nullptr) < 0){
			break;
		}
	}

	if (close(sockA) < 0) {
		syserr("closing stream socket");
	}
	if(finishA){
		exit(0);
	} else{
		exit(1);
	}
}


int main(int argc, char* argv[]) {

	int sockA;
	string host, resource, portA, meta = "0", portB, multi, name;
	int timeoutA = 5, timeoutB = 5;
	finishA = false;
	finishB = false;
	set_args(argc, argv, &host, &resource, &portA, &meta, &timeoutA, &portB,
		&multi, &timeoutB);

	// writing down the request
	string buffer_send = "GET " + resource + " HTTP/1.0\r\nHost: "
		+ host + "\r\n" + "Icy-MetaData:"+ meta + "\r\n" +
		+ "Connection: close\r\n\r\n";

	// setting global sock
	sockA = get_socket(host.c_str(), portA.c_str());

	// catch signal
	signal(SIGINT, sigint_handler);

	// send request
	long size_send = buffer_send.size();
	if (write(sockA, buffer_send.c_str(), size_send) != size_send) {
		syserr("partial / failed write");
	}

	// read header set metaInt
	int metaInt;
	read_header(&meta, &name, &metaInt, sockA);

	if(portB.empty()){
		// Part A
		print_cout_cerr(meta, metaInt, sockA, timeoutA);
	}

	// Part B
	// initiate pollfd for receiving message
	struct sockaddr_in server;
	struct pollfd poll_tab[2];

	init_poll(sockA, portB, poll_tab, &server);


	set<struct sockaddr_in, addr_comp> clients_list = {};
	struct sockaddr_in client_address;
	auto rcva_len = (socklen_t) sizeof(client_address);

	short buf[BUFF_SIZE];
	while(!finishB) {
		int ret = poll(poll_tab, 2, timeoutA * 1000);
		if(ret == 0){
			cout << "Nic nie zlapalem poll server nic nie dal LOL\n";
			close(poll_tab[0].fd);
			close(sockA);
			exit(0);
		}
		if (ret == -1) {
			if (errno == EINTR) {
				cerr << "Interrupted system call\n";
			} else {
				syserr("poll");
			}
		}
		if (ret > 0) {
			if (poll_tab[0].revents & (POLLIN | POLLERR)) {
				cerr << "Tak kurna zlapalem clienta\n";
				poll_tab[0].revents = 0;
				int flags = 0;
				int rval = recvfrom(poll_tab[0].fd, buf, HEAD_SIZE, flags,
									(struct sockaddr *) &client_address, &rcva_len);

				int type = ntohs(buf[0]);
				int size = ntohs(buf[1]);
				if (rval < 0) {
					fprintf(stderr, "Reading message (%d, )\n", errno);
					if (close(poll_tab[0].fd) < 0)
						syserr("close");
				}else {
					switch(type){
						case DISCOVER:
							if(clients_list.find(client_address) == clients_list.end()) {
								clients_list.emplace(client_address);
							}
							buf[0] = htons(IAM);
							//sendto(poll_tab[0].fd, bu)
							break;
						default:
							cout << "odd header\n";
					}
				}
			}
			if(poll_tab[1].revents & (POLLIN | POLLERR)){
				cerr << "Tak zlapalem server radio";
				poll_tab[1].revents = 0;
				// TODO wczytaj czesc, i wyslij


			}
		}
	}
	close(poll_tab[0].fd);
	close(sockA);

	return 0;
}