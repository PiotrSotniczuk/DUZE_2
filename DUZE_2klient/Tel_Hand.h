#ifndef DUZE_2KLIENT__TEL_HAND_H_
#define DUZE_2KLIENT__TEL_HAND_H_

#include <vector>
#include <string>
class Tel_Hand {
 private:
	int act_line;
	std::vector<std::string> senders;
	std::string meta;
	int msg_sock;
	int write_menu();

 public:
	Tel_Hand(int msg_sock){
		this->msg_sock = msg_sock;
		act_line = 0;
		senders = std::vector<std::string>();
		meta = std::string();
	}
	int read_write_init();
	int read_write();



};

#endif
