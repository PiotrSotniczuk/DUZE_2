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

#define BUFF_SIZE 4080

using namespace std;

static bool finishA;
static bool finishB;

// TODO czy zawsze close nawet przy bledzie


void sigint_handlerA([[maybe_unused]]int signal_num){
	finishA = true;
	finishB = true;
}

void read_write(int sockA, int descriptor, char *buff, unsigned int size){
	if(read(sockA, buff, size) != size){
		fatal("Partial read of data");
	}
	if(write(descriptor, buff, size) != size){
		fatal("Partial write of data");
	}
}

void print_cout_cerr(const string& meta,
	unsigned long int metaInt, int sockA){
	char buff[BUFF_SIZE];

	if(meta == "0"){
		while(!finishA){
			read_write(sockA, STDOUT_FILENO, buff, BUFF_SIZE);
		}
	}else {
		unsigned long int n = metaInt / BUFF_SIZE;
		while (!finishA) {
			for (unsigned int i = 0; i < n; i++) {
				read_write(sockA, STDOUT_FILENO, buff, BUFF_SIZE);
			}
			read_write(sockA, STDOUT_FILENO, buff, metaInt % BUFF_SIZE);

			if (read(sockA, &buff, 1) != 1) {
				fatal("Cannot read meta size byte");
			}
			char size = buff[0];
			read_write(sockA, STDERR_FILENO, buff, size * 16);
		}
	}
	if (close(sockA) < 0) {
		syserr("closing stream socket");
	}
	exit (0);
}

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
	struct timeval timeout;
	timeout.tv_sec = timeoutA;
	timeout.tv_usec = 0;
	// setting timeout option on socket
	if (setsockopt(sockA, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout)) < 0)
		syserr("setsockopt failed");

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
	read_header(response, &meta, &metaInt, sockA);

	if(portB.empty()){
		// Part A
		print_cout_cerr(meta, metaInt, sockA);
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


	char buf[BUFF_SIZE];

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
				int rval = read(poll_tab[0].fd, buf, BUFF_SIZE);
				if (rval < 0) {
					fprintf(stderr, "Reading message (%d, )\n", errno);
					if (close(poll_tab[0].fd) < 0)
						syserr("close");
				}else {
					printf("-->%.*s\n", (int)rval, buf);
				}
			}
		}
	}
	close(poll_tab[0].fd);
	close(sockA);

	return 0;
}