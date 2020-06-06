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
#include <chrono>

#define HEAD_SIZE 4

#define DISCOVER 1
#define IAM 2
#define KEEPALIVE 3
#define AUDIO 4
#define METADATA 6

//max size of message
#define BUFFER_SIZE 66000


bool finishAll;
using namespace std;
using namespace std::chrono;

void sigint_handler([[maybe_unused]]int signal_num){
	finishAll = true;
}

int main(int argc, char *argv[]) {
	string host, portB, portC;
	int timeout = 5;
	set_args_client(argc, argv, &host, &portB, &portC, &timeout);

	finishAll = false;
	bool no_telnet = true;
	// catch signal
	signal(SIGINT, sigint_handler);

	// get socket to sender
	struct sockaddr_in my_address{};
	int sockB = get_socketB(host, portB, &my_address);
	int sockC = init_sockC(portC);

	time_point<steady_clock> keep_alive_time;
	time_point<steady_clock> playing_timeout;
	Tel_Hand tel_hand = Tel_Hand(-1);

	pollfd poll_tab[3];
	init_poll_client(poll_tab, sockB, sockC);

	unsigned short discover[HEAD_SIZE];
	memset(discover, 0, HEAD_SIZE);
	discover[0] = htons(DISCOVER);
	discover[1] = htons(0);

	unsigned short keep_alive_com[HEAD_SIZE];
	memset(keep_alive_com, 0, HEAD_SIZE);
	keep_alive_com[0] = htons(KEEPALIVE);
	keep_alive_com[1] = htons(0);

	char buf[BUFFER_SIZE];

	while (!finishAll) {
		// TODO change timeout
		int ret_poll = poll(poll_tab, 3, 500);
		if (ret_poll < 0) {
			cerr << "poll error\n";
			break;
		}
		if (ret_poll == 0) {
			cerr << "poll nothing happened\n";
		}
		// sockB
		if (poll_tab[0].revents & POLLIN) {
			// MOZNA USUNAC

			struct sockaddr_in rcv_address{};
			socklen_t rcv_len = sizeof(rcv_address);
			int rcv = recvfrom(sockB, buf, BUFFER_SIZE,0,
				(struct sockaddr *) &rcv_address, &rcv_len);
			if (rcv == 0) {
				cerr << "DUPA0\n";
			}
			if (rcv < 0) {
				syserr("recfrom");
			}

			unsigned short communicat[HEAD_SIZE];
			memcpy(communicat, buf, 4);
			int type = ntohs(communicat[0]);
			unsigned short size_rc = ntohs(communicat[1]);
			assert(size_rc == (rcv - HEAD_SIZE));
			cerr << "received type: " << type << "  size:" << size_rc << "\n";
			buf[HEAD_SIZE + size_rc] = 0;
			switch (type) {
				case IAM:
					tel_hand.senders.emplace(rcv_address,
											 string(buf + HEAD_SIZE));
					tel_hand.write_menu();
					break;
				case AUDIO:
					if(addr_equal(tel_hand.playing->first, rcv_address)) {
						write(STDOUT_FILENO, buf + HEAD_SIZE, size_rc);
						playing_timeout = steady_clock::now();
					}
					break;
				case METADATA:
					// only need string so dont care about zeros
					if(addr_equal(tel_hand.playing->first, rcv_address)) {
						tel_hand.meta = string(buf + HEAD_SIZE);
						tel_hand.write_menu();
						playing_timeout = steady_clock::now();
					}
					break;
				default: cerr << "weird HEADER from proxy\n";
			}

			poll_tab[0].revents = 0;
		}
		// telnet
		if (poll_tab[1].revents & POLLIN) {
			int ret = tel_hand.read_write();
			if (ret < 0) {
				break;
			}
			if (ret == SEARCH_RV) {
				int snd_size = sendto(sockB, discover, HEAD_SIZE,0,
					(struct sockaddr *) &my_address, sizeof(my_address));
				assert(snd_size == HEAD_SIZE);
			}
			if (ret == END_RV) {
				break;
			}
			if (ret == SOCK_CLOSE) {
				no_telnet = true;
				poll_tab[1].fd = -1;
				tel_hand.msg_sock = -1;
			}

			if (ret >= VEC_BASE) {
				auto it = tel_hand.senders.begin();
				for (int i = 0; i < ret - VEC_BASE; i++) {
					it++;
				}
				if(tel_hand.playing != it) {
					tel_hand.playing = it;
					keep_alive_time = steady_clock::now();
					struct sockaddr_in chosen = it->first;
					int snd_size = sendto(sockB, discover, HEAD_SIZE, 0,
						(struct sockaddr *) &chosen, sizeof(chosen));
					playing_timeout = steady_clock::now();
					assert(snd_size == HEAD_SIZE);
					tel_hand.write_menu();
				}
			}
			poll_tab[1].revents = 0;
		}
		if (poll_tab[2].revents & POLLIN && no_telnet) {

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

		if (tel_hand.playing != tel_hand.senders.end() &&
			duration_cast<milliseconds>(steady_clock::now() - keep_alive_time).count() > 3500) {
			struct sockaddr_in chosen = tel_hand.playing->first;
			cerr << "sendind keep_alive\n";
			int snd_size = sendto(sockB, keep_alive_com, HEAD_SIZE,0,
				(struct sockaddr *) &chosen, sizeof(chosen));
			keep_alive_time = steady_clock::now();
			assert(snd_size == HEAD_SIZE);
		}

		if(tel_hand.playing != tel_hand.senders.end() &&
		duration_cast<milliseconds>(steady_clock::now() - playing_timeout).count() > timeout * 1000){
			tel_hand.meta = string();
			tel_hand.act_line = 0;
			tel_hand.senders.erase(tel_hand.playing);
			tel_hand.playing = tel_hand.senders.end();
			tel_hand.write_menu();
		}
	}

	if (close(sockC) < 0)
		syserr("close");
	if (tel_hand.msg_sock != -1 && close(tel_hand.msg_sock) < 0)
		syserr("close");
	if (close(sockB))
		syserr("close");
	return 0;
}