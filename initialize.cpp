#include "initialize.h"
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include "err.h"
#include <sys/socket.h>
#include <string>
#include <unistd.h>


#define ESSENTIALS_SUM 3
#define ESSEN_H 0
#define ESSEN_R 1
#define ESSEN_P 2
#define BUFF_SIZE 4080
#define HTTP_0 "HTTP/1.0 200 OK\r\n"
#define HTTP_1 "HTTP/1.1 200 OK\r\n"
#define ICY "ICY 200 OK\r\n"
#define META_DATA "icy-metaint:"

using namespace std;

void checkEssen(bool *essentials, int nr_essen){
	if(essentials[nr_essen]){
		fatal("Double set of argument");
	}
	essentials[nr_essen] = true;
}

void set_args(int argc, char** argv, string *host, string *resource,
	string *portA, string *meta, unsigned long int *timeoutA, string *portB,
	string *multi, unsigned long int *timeoutB){
	bool essentials[ESSENTIALS_SUM];
	if(argc < ESSENTIALS_SUM*2 + 1){
		fatal("Bad number of args");
	}

	memset(essentials, false, 3);
	for(int i = 1; i < argc; i = i + 2){
		if(strcmp(argv[i], "-h") == 0){
			checkEssen(essentials, ESSEN_H);
			(*host) = argv[i+1];
		}

		if(strcmp(argv[i], "-r") == 0){
			checkEssen(essentials, ESSEN_R);
			(*resource) = argv[i+1];
		}

		if(strcmp(argv[i], "-p") == 0){
			checkEssen(essentials, ESSEN_P);
			(*portA) = argv[i+1];
		}

		if(strcmp(argv[i], "-m") == 0){
			if(strcmp(argv[i+1], "yes") == 0){
				(*meta) = "1";
			}else{
				if(strcmp(argv[i+1], "no") == 0){
					(*meta) = "0";
				}else{
					fatal("Argument -m is not yes|no");
				}
			}
		}

		if(strcmp(argv[i], "-t") == 0){
			char *endptr;
			(*timeoutA) = strtoul(argv[i+1], &endptr, 10);
			if(endptr == argv[i+1]){
				fatal("Argument -t bad timeout");
			}
		}

		if(strcmp(argv[i], "-P") == 0){
			(*portB) = argv[i+1];
		}

		if(strcmp(argv[i], "-B") == 0){
			(*multi) = argv[i+1];
		}

		if(strcmp(argv[i], "-T") == 0){
			char *endptr;
			(*timeoutB) = strtoul(argv[i+1], &endptr, 10);
			if(endptr == argv[i+1]){
				fatal("Argument -T bad timeout");
			}
		}
	}

	for(bool essential : essentials){
		if(!essential){
			fatal("Missing essential argument");
		}
	}
}

int get_socket(const char *connect_adr, const char *port){
	int err;
	struct addrinfo addr_hints;
	struct addrinfo *addr_result;
	int sock;

	// 'converting' host/port in string to struct addrinfo
	memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_family = AF_INET; // IPv4
	addr_hints.ai_socktype = SOCK_STREAM; //TCP takie ma
	addr_hints.ai_protocol = IPPROTO_TCP; //tez ze TCP
	err = getaddrinfo(connect_adr, port, &addr_hints, &addr_result);

	if (err == EAI_SYSTEM) { // system error
		syserr("getaddrinfo: %s", gai_strerror(err));
	} else if (err != 0) { // other error (host not found, etc.)
		fatal("getaddrinfo: %s", gai_strerror(err));
	}

	// initialize socket according to getaddrinfo results
	sock = socket(addr_result->ai_family,
				  addr_result->ai_socktype,
				  addr_result->ai_protocol);
	if (sock < 0)
		syserr("socket");

	// connect socket to the server
	if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
		syserr("connect");

	freeaddrinfo(addr_result);
	return sock;
}

void read_header(FILE *response, string *meta, unsigned long int *metaInt,
	int sockA){
	unsigned long line_size = BUFF_SIZE;
	char* line = static_cast<char *>(malloc(line_size * sizeof(char)));
	if(line == nullptr){
		fatal("Cannot alloc memory");
	}

	if(getline(&line, &line_size, response) <= 0){
		fatal("Cannot read from file or no response from server");
	}

	// checking if status line is OK
	if(strncasecmp(line, ICY, strlen(ICY)) != 0 &&
		strncasecmp(line, HTTP_0, strlen(HTTP_0)) != 0 &&
		strncasecmp(line, HTTP_1, strlen(HTTP_1)) != 0){
		if (close(sockA) < 0)
			syserr("Closing stream socket");
		fatal("Status line");
	}

	//cerr << line;
	//read header
	(*metaInt) = 0;
	while(getline(&line, &line_size, response) > 2){ //break if only \r\n
		if(strncasecmp(line, META_DATA, strlen(META_DATA)) == 0){
			if((*meta) == "0"){
				fatal("Radio sends unwanted metadata");
			}
			char *endptr;
			(*metaInt) = strtoul(line + strlen(META_DATA), &endptr, 10);
			if(endptr == line){
				fatal("Bad metaInt number");
			}
		}
		//cerr << line;
	}

	// server does not support metaData
	if((*meta) == "1" && (*metaInt) == 0){
		(*meta) = "0";
	}

	// end of header
	if(strcmp(line, "\r\n") != 0){
		fatal("While reading header getline error");
	}
	free(line);
}