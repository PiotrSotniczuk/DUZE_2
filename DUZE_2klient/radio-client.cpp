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
#include "Tel_Hand.h"

#define HEAD_SIZE 4

#define DISCOVER 1
#define IAM 2
#define KEEPALIVE 3
#define AUDIO 4
#define METADATA 6

//max size of message
#define BUFFER_SIZE 66000

#define BUFFER_TEL_SIZE   200

bool finishAll;
bool finishTel;
using namespace std;

void sigint_handler([[maybe_unused]]int signal_num){
	finishAll = true;
}

int main(int argc, char *argv[]) {
	string host, portB, portC;
	int timeout = 5;
	set_args_client(argc, argv, &host, &portB, &portC, &timeout);

	finishAll = false;
	finishTel = false;
	bool no_telnet = true;
	// catch signal
	signal(SIGINT, sigint_handler);

	// get socket to sender
	struct sockaddr_in my_address{};
	int sockB = get_socketB(host, portB, &my_address);
	int sockC = init_sockC(portC);
	//struct sockaddr_in client_address{};
	//socklen_t client_address_len = 0;

	Tel_Hand tel_hand = Tel_Hand(-1);

	pollfd poll_tab[3];
	init_poll_client(poll_tab, sockB, sockC);


	unsigned short discover[HEAD_SIZE];
	memset(discover, 0, HEAD_SIZE);
	discover[0] = htons(DISCOVER);
	discover[1] = htons(0);

	auto rcva_len = (socklen_t) sizeof(my_address);

	while(!finishAll){
		// TODO change timeout
		int ret_poll = poll(poll_tab, 3, timeout * 6969);
		if(ret_poll <= 0){
			// nothing came especialyy from B
			// actualize list
			if(ret_poll < 0){
				cerr << "poll mistake\n";
			}else {
				cerr << "timeout\n";
			}
			break;
		}
		// sockB
		if(poll_tab[0].revents & POLLIN) {
			// TODO odbieram od chujka proxy
			poll_tab[0].revents = 0;
		}
		// telnet
		if(poll_tab[1].revents & POLLIN) {
			// TODO odbieram od chujka telnet
			int ret = tel_hand.read_write();
			if(ret < 0) {
				break;
			}
			if(ret == SEARCH_RV) {
				int snd_size = sendto(sockB, discover, HEAD_SIZE, 0,
									  (struct sockaddr *) &my_address, rcva_len);
				assert(snd_size == HEAD_SIZE);
			}
			if(ret == END_RV) {
				break;
			}
			if(ret == SOCK_CLOSE){
				no_telnet = true;
				poll_tab[1].fd = -1;
				tel_hand.msg_sock = -1;
			}

			if(ret >= VEC_BASE) {
				// TODO odbierz numer z enterem
			}
			poll_tab[1].revents = 0;
		}
		if(poll_tab[2].revents & POLLIN && no_telnet) {

			no_telnet = false;
			int msg_sock = accept(sockC, nullptr, nullptr);
			if (msg_sock < 0) {
				if (close(sockC) < 0)
					syserr("close");
				if (close(sockB))
					syserr("close");
				syserr("accept");
			}
			tel_hand.msg_sock = msg_sock;
			poll_tab[1].fd = msg_sock;
			tel_hand.read_write_init();
			poll_tab[2].revents = 0;

		}
	}


	if (close(sockC) < 0)
		syserr("close");
	if (tel_hand.msg_sock != -1 && close(tel_hand.msg_sock) < 0)
		syserr("close");
	if(close(sockB))
		syserr("close");
	return 0;


	/*unsigned short recv_head[HEAD_SIZE];
	recvfrom(sockB, recv_head, 4, 0, NULL, NULL);

	int type = ntohs(recv_head[0]);
	unsigned short size_rc = ntohs(recv_head[1]);
	cerr << "received" << type << "\n" << size_rc <<"\n";










	while(!finishAll){

			while(tel_hand.read_write() != SEARCH_RV){}


			pollfd poll_tab[2];
			init_poll_client(poll_tab, sockB, msg_sock);


			while(!finish){
				int ret_poll = poll(poll_tab, 2, timeout * 1000);
				if(ret_poll <= 0){
					// nothing came especialyy from B
					// actualize list
					if(ret_poll < 0){
						cerr << "wild_mistake\n";
					}
					cerr << "timeout\n";
					break;
				}
			}
		}






		int ret_poll = poll(poll_tab, 2, timeout * 1000);
		if(ret_poll <= 0){
			// nothing came especialyy from B
			// actualize list
			if(ret_poll < 0){
				cerr << "wild_mistake\n";
			}
			cerr << "timeout\n";
			break;
		}
		if(poll_tab[0].revents & POLLIN && wait_telnet) {
			wait_telnet = false;
			struct sockaddr_in client_address{};
			socklen_t client_address_len;
			int msg_sock = accept(poll_tab[0].fd, (struct sockaddr *) &client_address,
					   &client_address_len);
			if (msg_sock < 0) {
				if (close(poll_tab[0].fd) < 0)
					syserr("close");
				syserr("accept");
			}
			poll_tab[2].fd = msg_sock;
			Tel_Hand tel_hand = Tel_Hand(msg_sock);
			tel_hand.read_write_init();

			socklen_t rcva_len;
			int sflags;
			ssize_t snd_len, rcv_len;
		}
		// TODO send DISCOVER
		// recive IAMS
		while (!finishC){
			int ret = poll(poll_tab, 2, timeout * 1000);
			if(ret <= 0){
				// nothing came especialyy from B
				// actualize list
				cerr << "timeout\n";
				break;
			}
			if(poll_tab[1].revents & (POLLIN | POLLERR)) {
				cerr << "Tak zlapalem server radio\n";
				poll_tab[1].revents = 0;
				reader.readSendChunk(&clients_map, poll_tab[0].fd, timeoutB);

			}


		}
		if (close(msg_sock) < 0)
			syserr("close");



	}


	if(close(poll_tab[0].fd) < 0)
		syserr("close");

*/




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
	}

	return 0;*/
}