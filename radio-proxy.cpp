#include <string>
#include "parse_args.h"
#include "err.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define BUFF_SIZE 5000
#define HTTP_0 "HTTP/1.0 200 OK\r\n"
#define HTTP_1 "HTTP/1.1 200 OK\r\n"
#define ICY "ICY 200 OK\r\n"



using namespace std;

// instructions from labs combined into one function to initialize socket
int get_socket(const char *connect_adr, const char *port){
	int err;
	int sock;
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
	return sock;
}

int main(int argc, char* argv[]) {

	string host;
	string resource;
	string port;
	bool metaData = false;
	unsigned long int timeout = 5;

	set_args(argc, argv, &host, &resource, &port, &metaData, &timeout);


	// writing down the request
	// cookies start with space so no space after "Cookie:"
	string buffer_send = "GET " + resource + " HTTP/1.0\r\nHost: "
		+ host + "\r\n"
		   + "Icy-MetaData:1\r\n" +
		   + "Connection: close\r\n\r\n";

	int sock = get_socket(host.c_str(), port.c_str());

	// send request
	unsigned long size_send = buffer_send.size();
	if (write(sock, buffer_send.c_str(), size_send) != size_send) {
		syserr("partial / failed write");
	}

	// receiving stream turned into a file
	FILE *response = NULL;
	response = fdopen(sock, "r");
	if(response == NULL){
		fatal("Cannot open socekt as file");
	}

	unsigned long line_size = BUFF_SIZE;
	char* line = static_cast<char *>(malloc(line_size * sizeof(char)));
	if(line == NULL){
		fatal("Cannot alloc memory");
	}

	if(getline(&line, &line_size, response) <= 0){
		fatal("Cannot read from file or no response from server");
	}

	// checking if status line is OK
	if(strcmp(line, ICY) != 0 && strcmp(line, HTTP_0) != 0 &&
		strcmp(line, HTTP_1) != 0){
		cerr << "ERROR: Bad status line\n";
		if (close(sock) < 0)
			syserr("closing stream socket");
		return 0;
	}

	cout << line;
	//read header
	bool chunked = false;
	while(getline(&line, &line_size, response) > 2){ //break if only \r\n
		cout << string(line);
	}

	if(strcmp(line, "\r\n") != 0){
		fatal("While reading header getline error");
	}
	free(line);

	char buff[BUFF_SIZE];
	while(fread(&buff, 1, BUFF_SIZE, response) == BUFF_SIZE){
		cout << string(buff);
	}


	return 0;
}
