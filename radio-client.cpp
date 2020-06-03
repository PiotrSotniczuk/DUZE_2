#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include "err.h"
#include <signal.h>
#include <cassert>

#define ESSENTIALS_SUM 3
#define ESSEN_H 0
#define ESSEN_PB 1
#define ESSEN_PC 2

#define HEAD_SIZE 4

#define DISCOVER 1
#define IAM 2
#define KEEPALIVE 3
#define AUDIO 4
#define METADATA 6

#define BUFFER_SIZE 10000

bool finish;
using namespace std;

void sigint_handler([[maybe_unused]]int signal_num){
	finish = true;
}

static void checkEssen(bool *essentials, int nr_essen){
	if(essentials[nr_essen]){
		fatal("Double set of argument");
	}
	essentials[nr_essen] = true;
}

static void set_args_client(int argc, char** argv, string *host,
	string *portB, string *portC, unsigned long int *timeoutA){
	bool essentials[ESSENTIALS_SUM];
	if(argc < ESSENTIALS_SUM*2 + 1){
		fatal("Bad number of args");
	}

	memset(essentials, false, 3);
	for(int i = 1; i < argc; i = i + 2){
		if(strcmp(argv[i], "-H") == 0){
			checkEssen(essentials, ESSEN_H);
			(*host) = argv[i+1];
		}

		if(strcmp(argv[i], "-P") == 0){
			checkEssen(essentials, ESSEN_PB);
			(*portB) = argv[i+1];
		}

		if(strcmp(argv[i], "-p") == 0){
			checkEssen(essentials, ESSEN_PC);
			(*portC) = argv[i+1];
		}

		if(strcmp(argv[i], "-T") == 0){
			char *endptr;
			(*timeoutA) = strtoul(argv[i+1], &endptr, 10);
			if(endptr == argv[i+1]){
				fatal("Argument -t bad timeout");
			}
		}
	}

	for(bool essential : essentials){
		if(!essential){
			fatal("Missing essential argument");
		}
	}
}


int main(int argc, char *argv[]) {
	string host, portB, portC;
	unsigned long timeout;
	set_args_client(argc, argv, &host, &portB, &portC, &timeout);

	finish = false;
	// catch signal
	signal(SIGINT, sigint_handler);

	int sockB;
	struct addrinfo addr_hints;
	struct addrinfo *addr_result;

	int sflags;
	ssize_t snd_len, rcv_len;
	struct sockaddr_in my_address;
	struct sockaddr_in srvr_address;

	socklen_t rcva_len;

	// 'converting' host/port in string to struct addrinfo
	(void) memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_family = AF_INET; // IPv4
	addr_hints.ai_socktype = SOCK_DGRAM;
	addr_hints.ai_protocol = IPPROTO_UDP;
	addr_hints.ai_flags = 0;
	addr_hints.ai_addrlen = 0;
	addr_hints.ai_addr = NULL;
	addr_hints.ai_canonname = NULL;
	addr_hints.ai_next = NULL;
	if (getaddrinfo(host.c_str(), NULL, &addr_hints, &addr_result) != 0) {
		syserr("getaddrinfo");
	}

	my_address.sin_family = AF_INET; // IPv4
	my_address.sin_addr.s_addr =
			((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
	my_address.sin_port = htons((uint16_t) stoi(portB)); // port from the command line

	sockB = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockB < 0){
		syserr("socket");
	}

	freeaddrinfo(addr_result);


	unsigned short communicat[HEAD_SIZE];
	memset(communicat, 0, HEAD_SIZE);
	communicat[0] = htons(DISCOVER);
	communicat[1] = htons(0);

	cerr << "send: " <<  communicat[0]  <<
	" =ntohs"<< ntohs(communicat[0])<< "\n";
	sflags = 0;
	rcva_len = (socklen_t) sizeof(my_address);

	snd_len = sendto(sockB, communicat, HEAD_SIZE, sflags,
			(struct sockaddr *) &my_address, rcva_len);
	if (snd_len != (ssize_t) (HEAD_SIZE)) {
		syserr("partial / failed write");
	}

	char buf[BUFFER_SIZE];
	memset(buf, 0, BUFFER_SIZE);
	recvfrom(sockB, communicat, 4, 0, NULL, NULL);
	cerr << "\n" <<communicat[0] << "------" << communicat[1] << "\n";
	int type = ntohs(communicat[0]);
	int size_rc = ntohs(communicat[1]);
	cerr << "received" << type << "\n" << size_rc <<"\n";


	communicat[0] = htons(KEEPALIVE);
	communicat[1] = htons(0);

	cerr << "send: " <<  communicat[0]  <<
		 " =ntohs"<< ntohs(communicat[0])<< "\n";
	sflags = 0;
	rcva_len = (socklen_t) sizeof(my_address);

	snd_len = sendto(sockB, communicat, HEAD_SIZE, sflags,
					 (struct sockaddr *) &my_address, rcva_len);
	if (snd_len != (ssize_t) (HEAD_SIZE)) {
		syserr("partial / failed write");
	}

	// ODBIOR DANYCH
	while(!finish) {
		char buf[BUFFER_SIZE];
		memset(buf, 0, BUFFER_SIZE);
		/*recvfrom(sockB, communicat, 4, 0, NULL, NULL);
		cerr << "\n" << communicat[0] << "------" << communicat[1] << "\n";
		int type = ntohs(communicat[0]);
		assert(type == AUDIO);
		size_rc = ntohs(communicat[1]);*/
		recvfrom(sockB, buf, 4080, 0, NULL, NULL);
		//cerr << "received" << type << "\n" << size_rc << "\n";
		cerr << "-";
		write(STDOUT_FILENO, buf, 4080);
	}

	 if (close(sockB) == -1) { //very rare errors can occur here, but then
		syserr("close"); //it's healthy to do the check
	}

	return 0;
}