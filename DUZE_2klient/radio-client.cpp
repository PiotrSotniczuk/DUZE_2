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
#include <poll.h>
#include "initialize_client.h"

#define HEAD_SIZE 4

#define DISCOVER 1
#define IAM 2
#define KEEPALIVE 3
#define AUDIO 4
#define METADATA 6

//max size of message
#define BUFFER_SIZE 66000

#define BUFFER_TEL_SIZE   200

bool finish;
using namespace std;

void sigint_handler([[maybe_unused]]int signal_num){
	finish = true;
}

int main(int argc, char *argv[]) {
	string host, portB, portC;
	int timeout = 5;
	set_args_client(argc, argv, &host, &portB, &portC, &timeout);

	finish = false;
	// catch signal
	signal(SIGINT, sigint_handler);

	socklen_t rcva_len;
	int sflags;
	ssize_t snd_len, rcv_len;
	struct sockaddr_in my_address{};
	// get socket to sender
	int sockB = get_socketB(host, portB, &my_address);

	struct pollfd poll_tab[0];
	init_poll_client(portC, poll_tab);

	struct sockaddr_in client_address{};
	socklen_t client_address_len;
	int msg_sock = accept(poll_tab[0].fd, (struct sockaddr *) &client_address, &client_address_len);

	if (msg_sock < 0)
		syserr("accept");

	unsigned char do_linemode[3] = {255, 253, 34};
	int len = 3;
	write(msg_sock, do_linemode, len);
	len = 7;
	unsigned char linemode_options[7] = {255, 250, 34, 1, 0, 255, 240};
	write(msg_sock, linemode_options, len);

	//unsigned char will_echo[3] = {255, 251, 1};

	char stuff[80];
	int bw = snprintf(stuff, 80, "\x1b[0m\x1b[H\x1b[2J");
	if (write(msg_sock, stuff, bw) == -1) {
		perror("Unable to clear screen");
	}



	sleep(10);
	if (close(msg_sock) < 0)
		syserr("close");

	if(close(poll_tab[0].fd) < 0)
		syserr("close");









	/*unsigned short communicat[HEAD_SIZE];
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
	unsigned short size_rc = ntohs(communicat[1]);
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
		int rcv = recvfrom(sockB, buf, BUFFER_SIZE, 0, NULL, NULL);
		//cerr << "typebefore:" << communicat[0] << " sizebefore:" << communicat[1] << "\n";
		unsigned short type;
		memcpy(&type, buf, 2);
		type = ntohs(type);
		assert(type == AUDIO);
		memcpy(&size_rc, buf + 2, 2);
		size_rc = ntohs(size_rc);
		cerr << "type real" << type << "sizereal;" << size_rc << "\n";
		cerr << "\n";
		write(STDOUT_FILENO, buf + HEAD_SIZE, size_rc);

		communicat[0] = htons(KEEPALIVE);
		communicat[1] = htons(0);

		cerr << "received :" << rcv <<"\n";
		sflags = 0;
		rcva_len = (socklen_t) sizeof(my_address);

		sendto(sockB, communicat, HEAD_SIZE, sflags,
						 (struct sockaddr *) &my_address, rcva_len);
	}

	 if (close(sockB) == -1) { //very rare errors can occur here, but then
		syserr("close"); //it's healthy to do the check
	}*/

	return 0;
}