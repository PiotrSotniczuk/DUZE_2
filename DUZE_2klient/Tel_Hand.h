#ifndef DUZE_2KLIENT__TEL_HAND_H_
#define DUZE_2KLIENT__TEL_HAND_H_

#define SEARCH_RV 1
#define END_RV 2
#define SOCK_CLOSE 3
#define VEC_BASE 10

#include <vector>
#include <string>
#include <map>
// TODO include from RadioReader
#include <netdb.h>
#include <unistd.h>
struct addr_comp_client {
	bool operator() (const sockaddr_in lhs,
					 const sockaddr_in rhs) const {
		if(lhs.sin_addr.s_addr < rhs.sin_addr.s_addr){
			return true;
		}
		if(lhs.sin_addr.s_addr > rhs.sin_addr.s_addr){
			return false;
		}
		// s_addr is equal
		return lhs.sin_port < rhs.sin_port;
	}
};

class Tel_Hand {
 private:
	int act_line;
	void bold(std::string *menu, const std::string& line,int i);

 public:
	std::map<struct sockaddr_in, std::string, addr_comp_client>::iterator playing;
	std::map<struct sockaddr_in, std::string, addr_comp_client> senders;
	int msg_sock;
	std::string meta;

	Tel_Hand(int msg_sock){
		this->msg_sock = msg_sock;
		act_line = 0;
		senders = {};
		meta = std::string();
		playing = senders.end();
	}
	int read_write_init();
	int read_write();
	int write_menu();
	/*void set_msg_sock(int new_sock){
		this->msg_sock = new_sock;
	};*/



};

#endif
