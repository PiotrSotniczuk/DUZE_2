#include "Tel_Hand.h"
#include <unistd.h>
#include <iostream>

#define WEIRD_RESP_SIZE 10000
#define SEARCH "Szukaj posrednika"
#define END "Koniec"
#define SENDER "Posrednik "

using namespace std;
void Tel_Hand::bold(string *menu, const string& line, int i){
	if(act_line == i){
		(*menu) += "\x1b[1;31m";
	}
	(*menu) += line;
	if(act_line == i){
		(*menu) += "\x1b[0m";
	}
	(*menu) += "\n";
}

int Tel_Hand::write_menu(){
	string clear_term = "\x1b[0m\x1b[H\x1b[2J";
	int i = 0;
	int size = clear_term.size();
	if (write(msg_sock, clear_term.c_str(), size) != size) {
		cerr << "ERROR: Cannot clear terminal\n";
		return -1;
	}

	string menu;
	bold(&menu, SEARCH, i);
	i++;

	for(auto sender = senders.begin(); sender != senders.end(); sender++){
		string sender_str = SENDER + sender->second;
		if(playing == sender){
			sender_str += " *";
		}
		bold(&menu, sender_str, i);
		i++;
	}
	bold(&menu, END, i);

	if(!meta.empty()){
		menu += meta + "\n";
	}

	size = menu.size();
	if (write(msg_sock, menu.c_str(), size) != size) {
		cerr << "ERROR: Cannot write menu\n";
		return -1;
	}
	return 0;
}

int Tel_Hand::read_write_init(){
	unsigned char do_linemode[3] = {255, 253, 34};
	int len = 3;
	if(write(msg_sock, do_linemode, len) != len){
		cerr << "ERROR: Cannot enter linemod\n";
		return -1;
	}
	len = 7;
	unsigned char linemode_options[7] = {255, 250, 34, 1, 0, 255, 240};
	if(write(msg_sock, linemode_options, len) != len){
		cerr << "ERROR: Cannot enter linemod options\n";
		return -1;
	}
	len = 0;
	unsigned char weird_response[WEIRD_RESP_SIZE];
	if((len = read(msg_sock, weird_response, WEIRD_RESP_SIZE)) < 0){
		cerr << "ERROR: no weird message??\n";
		return -1;
	}
	cerr << "weird msg size:" << len << "\n";
	return write_menu();
}

int Tel_Hand::read_write() {
	int rcv;
	unsigned char buff[20];
	if ((rcv = read(msg_sock, buff, 20)) < 0) {
		cerr << "ERROR: Cannot read from telnet\n";
		return -1;
	}

	cerr << "size read:" << rcv << "\n";

	// socket closed
	if(rcv == 0){
		cerr << "socket closed\n";
		return SOCK_CLOSE;
	}

	if (rcv > 3 || rcv < 2) {
		cerr << "What do you mean ?? Bad button clicked\n";
		return write_menu();
	}

// up 27, 91, 65
// down 27, 91, 66
// enter 13, 0

	if (buff[0] == 27 && buff[1] == 91 && rcv == 3) {
		switch (buff[2]) {
			case 65: cerr << "arrow up\n";
				if (act_line != 0) {
					act_line--;
				}
				break;
			case 66: cerr << "arrow down\n";
				if (act_line != senders.size() + 1) {
					act_line++;
				}
				break;
			default: cerr << "What do you mean ?? Bad button clicked\n";
		}
		return write_menu();
	}

	if (buff[0] == 13 && buff[1] == 0) {
		if (write_menu() < 0) {
			return -1;
		}
		if(act_line == 0) {
			cerr << "search\n";
			return SEARCH_RV;
		}
		if(act_line == (senders.size() + 1)){
			cerr << "End\n";
			return END_RV;
		}
		return act_line - 1 + VEC_BASE;
	}

	cerr << "What do you mean ?? Bad button clicked\n";
	return write_menu();
}

bool addr_equal(struct sockaddr_in a, struct sockaddr_in b){
	return (a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port);
}