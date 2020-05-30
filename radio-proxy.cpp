#include <string>
#include "initialize.h"
#include "err.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>
#include <poll.h>
#include <netdb.h>

#define BUFF_SIZE 4080

using namespace std;

static bool finishA;
static bool finishB;

// TODO czy zawsze close nawet przy bledzie


void sigint_handlerA([[maybe_unused]]int signal_num){
	finishA = true;
	finishB = true;
}

void read_write(FILE *response, int descriptor, char *buff, unsigned int size){
	if(fread(buff, 1, size, response) != size){
		fatal("Partial read of data");
	}
	if(write(descriptor, buff, size) != size){
		fatal("Partial write of data");
	}
}

void print_cout_cerr(FILE *response, const string& meta,
	unsigned long int metaInt, int sockA){
	char buff[BUFF_SIZE];

	if(meta == "0"){
		while(!finishA){
			read_write(response, STDOUT_FILENO, buff, BUFF_SIZE);
		}
	}else {
		unsigned long int n = metaInt / BUFF_SIZE;
		while (!finishA) {
			for (unsigned int i = 0; i < n; i++) {
				read_write(response, STDOUT_FILENO, buff, BUFF_SIZE);
			}
			read_write(response, STDOUT_FILENO, buff, metaInt % BUFF_SIZE);

			if (fread(&buff, 1, 1, response) != 1) {
				fatal("Cannot read meta size byte");
			}
			char size = buff[0];
			read_write(response, STDERR_FILENO, buff, size * 16);
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
		print_cout_cerr(response, meta, metaInt, sockA);
	}

	// Part B
	// initiate pollfd for receiving message and sending data
	struct pollfd receiver;
	struct sockaddr_in server;

	// get socket for receiver (PF_INET = IPv4)
	receiver.fd = socket(AF_INET, SOCK_DGRAM, 0);
	receiver.events = POLL_IN;
	receiver.revents = 0;
	if (receiver.fd == -1){
		syserr("Opening stream socket");
	}

	// bind socket to receive from port
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(stoi(portB));;
	if (bind(receiver.fd, (struct sockaddr*)&server,
			 (socklen_t)sizeof(server)) == -1){
		syserr("Binding stream socket");
	}

	// chyba to jedno styka




	return 0;
}
