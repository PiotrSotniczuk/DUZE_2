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

#define ESSENTIALS_SUM 3
#define ESSEN_H 0
#define ESSEN_PB 1
#define ESSEN_PC 2
#define BUFFER_SIZE 1000

using namespace std;

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

	int sock;
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

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0){
		syserr("socket");
	}
	if(connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0){
		syserr("connect");
	}



	freeaddrinfo(addr_result);






	int k = 100;
	char napis[k+1];
	napis[k] = '\0';
	memset(napis, '^', k);

	cout << "printing: " <<  napis << "\n";
	sflags = 0;
	rcva_len = (socklen_t) sizeof(my_address);
	snd_len = sendto(sock, napis, k+1, sflags,
			(struct sockaddr *) &my_address, rcva_len);
	if (snd_len != (ssize_t) (k+1)) {
		syserr("partial / failed write");
	}
	if(write(sock, napis, k+1) != k+1){
		cout << "WTF";
	}
	cout << "wyslalem\n";

	/*(void) memset(napis, 0, sizeof(napis));
	int flags = 0;
	size_t len = (size_t) sizeof(napis);
	rcva_len = (socklen_t) sizeof(srvr_address);
	rcv_len = recvfrom(sock, napis, len, flags,
					   (struct sockaddr *) &srvr_address, &rcva_len);

	if (rcv_len < 0) {
		syserr("read");
	}
	(void) printf("read from socket: %zd bytes: %s\n", rcv_len, napis);
	*/
	sleep(10);
	 if (close(sock) == -1) { //very rare errors can occur here, but then
		syserr("close"); //it's healthy to do the check
	}

	return 0;
}