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




void sigint_handlerA([[maybe_unused]]int signal_num){
	finishA = true;
	finishB = true;
}

void read_write(FILE* response, int sockA, int descriptor, char *buff, unsigned int size,
	unsigned long timeoutA){
	fd_set set;
	struct timeval time_struct;
	time_struct.tv_sec = timeoutA;
	time_struct.tv_usec = 0;
	FD_ZERO(&set); /* clear the set */
	FD_SET(sockA, &set); /* add our file descriptor to the set */

	while(size > 0 && !finishA) {
		int rv = select(sockA + 1, &set, nullptr, nullptr, &time_struct);
		if(rv == -1) {
			if(fclose(response) < 0){
				fatal("select and close");
			}
			fatal("select");
		} else {
			if (rv == 0){
				if(fclose(response) < 0){
					fatal("timeout and close");
				}
				fatal("radio server timeout"); /* a timeout occured */
			}else{
				unsigned int act = 0;
				if ((act = fread(buff, 1, size, response)) < 0) {
					fatal("reading from sockA");
				}
				if (write(descriptor, buff, act) != act) {
					fatal("writing to stream");
				}
				size -= act;
			}
		}
		time_struct.tv_sec = timeoutA;
		time_struct.tv_usec = 0;
	}
}

void print_cout_cerr(FILE *response, const string& meta,
	unsigned long int metaInt, int sockA, unsigned long timeoutA){
	char buff[MAX_META_SIZE];

	if(meta == "0"){
		while(!finishA){
			read_write(response, sockA, STDOUT_FILENO, buff, MAX_META_SIZE, timeoutA);
		}
	}else {
		unsigned long int n = metaInt / MAX_META_SIZE;
		while (!finishA) {
			for (unsigned int i = 0; i < n; i++) {
				read_write(response, sockA, STDOUT_FILENO, buff, MAX_META_SIZE, timeoutA);
			}
			read_write(response, sockA, STDOUT_FILENO, buff, metaInt % MAX_META_SIZE, timeoutA);

			if (fread(&buff, 1, 1, response) != 1) {
				fatal("Cannot read meta size byte");
			}
			char size = buff[0];
			read_write(response, sockA, STDERR_FILENO, buff, size * 16, timeoutA);
		}
	}
	if (close(sockA) < 0) {
		syserr("closing stream socket");
	}
	exit (0);
}

struct addr_comp {
	bool operator() (const struct sockaddr_in lhs, const struct sockaddr_in rhs) const {
		return lhs.sin_addr.s_addr < rhs.sin_addr.s_addr;
	}
};

int main(int argc, char* argv[]) {

	int sockA;
	string host, resource, portA, meta = "0", portB, multi;
	unsigned long int timeoutA = 5, timeoutB = 5;
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
	/*struct timeval timeout;
	timeout.tv_sec = timeoutA;
	timeout.tv_usec = 0;
	// setting timeout option on socket
	if (setsockopt(sockA, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout)) < 0)
		syserr("setsockopt failed");*/

	// catch signal
	signal(SIGINT, sigint_handlerA);

	// send request
	long size_send = buffer_send.size();
	if (write(sockA, buffer_send.c_str(), size_send) != size_send) {
		syserr("partial / failed write");
	}

	// receiving stream turned into a file
	FILE *response;
	response = fdopen(sockA, "rb");
	if(response == nullptr){
		close(sockA);
		fatal("Cannot open socekt as file");
	}

	// read header set metaInt
	unsigned long int metaInt;
	// TODO catch name or something
	read_header(response, &meta, &metaInt, sockA);

	if(portB.empty()){
		// Part A
		print_cout_cerr(response, meta, metaInt, sockA, timeoutA);
	}

	// Part B
	// initiate pollfd for receiving message
	struct sockaddr_in server;
	struct pollfd poll_tab[1];

	// get socket for poll_tab[0] (PF_INET = IPv4)
	poll_tab[0].fd = socket(AF_INET, SOCK_DGRAM, 0);
	poll_tab[0].events = POLL_IN;
	poll_tab[0].revents = 0;
	if (poll_tab[0].fd == -1){
		syserr("Opening stream socket");
	}

	// bind socket to receive from port
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(stoi(portB));
	cout << "portB " << stoi(portB) << "\n";
	if (bind(poll_tab[0].fd, (struct sockaddr*)&server,
			 (socklen_t)sizeof(server)) < 0){
		syserr("Binding stream socket");
	}

	set<struct sockaddr_in, addr_comp> clients_list = {};
	struct sockaddr_in client_address;
	socklen_t snda_len = (socklen_t) sizeof(client_address);
	socklen_t rcva_len = (socklen_t) sizeof(client_address);
	short buf[BUFF_SIZE];
	while(!finishB) {
		int ret = poll(poll_tab, 1, 1000);
		if(ret == 0){
			cout << "Nic nie zlapalem poll\n";
		}
		if (ret == -1) {
			if (errno == EINTR){
				cerr << "Interrupted system call\n";
			}else{
				syserr("poll");
			}
		} else if (ret > 0) {
			if (poll_tab[0].revents & (POLL_IN | POLLERR)) {
				cerr << "Tak kurna zlapalem clienta\n";
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
		}
	}
	close(poll_tab[0].fd);
	close(sockA);

	return 0;
}