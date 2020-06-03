#include "initialize_client.h"
#include "err.h"
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <poll.h>


#define ESSENTIALS_SUM 3
#define ESSEN_H 0
#define ESSEN_PB 1
#define ESSEN_PC 2

using namespace std;

static void checkEssen(bool *essentials, int nr_essen){
	if(essentials[nr_essen]){
		fatal("Double set of argument");
	}
	essentials[nr_essen] = true;
}

void set_args_client(int argc, char** argv, string *host,
							string *portB, string *portC, int *timeoutA){
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
			(*timeoutA) = stoi(argv[i+1]);
		}
	}

	for(bool essential : essentials){
		if(!essential){
			fatal("Missing essential argument");
		}
	}
}

int get_socketB(const string& host, const string& portB, struct sockaddr_in *my_address){
	struct addrinfo addr_hints{};
	struct addrinfo *addr_result;
	int sock;


	// 'converting' host/port in string to struct addrinfo
	(void) memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_family = AF_INET; // IPv4
	addr_hints.ai_socktype = SOCK_DGRAM;
	addr_hints.ai_protocol = IPPROTO_UDP;
	addr_hints.ai_flags = 0;
	addr_hints.ai_addrlen = 0;
	addr_hints.ai_addr = nullptr;
	addr_hints.ai_canonname = nullptr;
	addr_hints.ai_next = nullptr;
	if (getaddrinfo(host.c_str(), nullptr, &addr_hints, &addr_result) != 0) {
		syserr("getaddrinfo");
	}

	my_address->sin_family = AF_INET; // IPv4
	my_address->sin_addr.s_addr =
		((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
	my_address->sin_port = htons((uint16_t) stoi(portB)); // port from the command line

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0){
		syserr("socket");
	}

	freeaddrinfo(addr_result);
	return sock;
}

void init_poll_client(const string& portC, struct pollfd *poll_tab){

	// get socket for poll_tab[0] (PF_INET = IPv4)
	poll_tab[0].fd = socket(AF_INET, SOCK_STREAM, 0);
	poll_tab[0].events = POLLIN;
	poll_tab[0].revents = 0;
	if (poll_tab[0].fd < 0){
		syserr("Opening stream socket");
	}


	struct sockaddr_in server{};
	// bind socket to receive from port
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(stoi(portC));

	if (bind(poll_tab[0].fd, (struct sockaddr*)&server,
			 (socklen_t)sizeof(server)) < 0){
		syserr("Binding stream socket");
	}

	// backlog 1, only one tcp accepted ?
	if (listen(poll_tab[0].fd, 1) == -1)
		syserr("Starting to listen");
}