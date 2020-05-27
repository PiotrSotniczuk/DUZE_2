#include <string>
#include "parse_args.h"
#include "err.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <csignal>

#define BUFF_SIZE 7048
#define HTTP_0 "HTTP/1.0 200 OK\r\n"
#define HTTP_1 "HTTP/1.1 200 OK\r\n"
#define ICY "ICY 200 OK\r\n"
#define META_DATA "icy-metaint:"



using namespace std;

int sock;

void sigint_handler(int signal_num){
	if (close(sock) < 0)
		syserr("closing stream socket");
	exit (0);
}

// instructions from labs combined into one function to initialize socket
void get_socket(const char *connect_adr, const char *port){
	int err;
	struct addrinfo addr_hints;
	struct addrinfo *addr_result;

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
}

void print_cout_cerr(FILE *response, unsigned long int metaInt){
	char buff[BUFF_SIZE + 1];
	buff[BUFF_SIZE] = '\0';

	unsigned long int n = metaInt/BUFF_SIZE;
	while(true) {
		for (int i = 0; i < n; i++) {
			fread(&buff, 1, BUFF_SIZE, response);
			printf("%.*s", BUFF_SIZE, buff);
		}
		fread(&buff, 1, metaInt % BUFF_SIZE, response);
		printf("%.*s", metaInt % BUFF_SIZE, buff);

		fread(&buff, 1, 1, response);
		char size = buff[0];
		fread(&buff, 1, size * 16, response);
		fprintf(stderr, "%.*s", size * 16, buff);
	}
}

int main(int argc, char* argv[]) {

	string host;
	string resource;
	string portA;
	string meta = "0";
	unsigned long int timeoutA = 5;
	string portB;
	string multi;
	unsigned long int timeoutB = 5;




	set_args(argc, argv, &host, &resource, &portA, &meta, &timeoutA, &portB,
		&multi, &timeoutB);


	// writing down the request
	string buffer_send = "GET " + resource + " HTTP/1.0\r\nHost: "
		+ host + "\r\n"
		   + "Icy-MetaData:"+ meta + "\r\n" +
		   + "Connection: close\r\n\r\n";

	// setting global sock
	get_socket(host.c_str(), portA.c_str());
	struct timeval timeout;
	timeout.tv_sec = timeoutA;
	timeout.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout)) < 0)
		syserr("setsockopt failed");

	signal(SIGINT, sigint_handler);

	// send request
	unsigned long size_send = buffer_send.size();
	if (write(sock, buffer_send.c_str(), size_send) != size_send) {
		syserr("partial / failed write");
	}

	// receiving stream turned into a file
	FILE *response;
	response = fdopen(sock, "r");
	if(response == nullptr){
		fatal("Cannot open socekt as file");
	}

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
		if (close(sock) < 0)
			syserr("Closing stream socket");
		fatal("Status line");
	}


	//cerr << line;
	//read header
	unsigned long int metaInt;
	while(getline(&line, &line_size, response) > 2){ //break if only \r\n
		if(strncasecmp(line, META_DATA, strlen(META_DATA)) == 0){
			if(meta == "0"){
				fatal("Radio sends unwanted metadata");
			}
			char *endptr;
			metaInt = strtoul(line + strlen(META_DATA), &endptr, 10);
			if(endptr == line){
				fatal("Bad metaInt number");
			}
		}
		//cerr << line;
	}

	if(strcmp(line, "\r\n") != 0){
		fatal("While reading header getline error");
	}
	free(line);


	if(portB.empty()){
		print_cout_cerr(response, metaInt);
	}

	// TODO wysylaj wszystko naraz skad beda wiedzieli

	return 0;
}
