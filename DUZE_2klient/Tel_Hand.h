#ifndef DUZE_2KLIENT__TEL_HAND_H_
#define DUZE_2KLIENT__TEL_HAND_H_

#define SEARCH_RV 1
#define END_RV 2
#define VEC_BASE 10

#include <vector>
#include <string>
class Tel_Hand {
 private:
	int act_line;
	std::string meta;
	int msg_sock;
	int write_menu();
	void bold(std::string *menu, const std::string& line,int i);

 public:
	int playing;
	std::vector<std::string> senders;

	Tel_Hand(int msg_sock){
		this->msg_sock = msg_sock;
		act_line = 0;
		senders = std::vector<std::string>();
		meta = std::string();
		playing = -1;
	}
	int read_write_init();
	int read_write();
	void set_msg_sock(int new_sock){
		this->msg_sock = new_sock;
	};



};

#endif
