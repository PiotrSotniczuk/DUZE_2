#ifndef DUZE_2__RADIOREADER_H_
#define DUZE_2__RADIOREADER_H_

#include <string>
#include <set>
#include <netdb.h>
#define MAX_META_SIZE 4080
#define HEAD_SIZE 4

struct addr_comp {
	bool operator() (const struct sockaddr_in lhs,
		const struct sockaddr_in rhs) const {
		return lhs.sin_addr.s_addr < rhs.sin_addr.s_addr;
	}
};

class RadioReader {
 private:
	int sockA;
	char *buff;
	char message[MAX_META_SIZE + 4];
	bool to_cout;
	bool meta;
	int metaInt;
	int afterMeta;
	int readSendNoMeta(std::set<struct sockaddr_in,
						   addr_comp> *clients_list);
	int readSendMeta(std::set<struct sockaddr_in,
						 addr_comp> *clients_list);
	int readMeta(std::set<struct sockaddr_in,
					 addr_comp> *clients_list);


 public:
	RadioReader(bool meta, int metaInt,
		int sockA, bool to_cout) {
		this->meta = meta;
		this->metaInt = metaInt;
		this->sockA = sockA;
		this->to_cout = to_cout;
		afterMeta = 0;
		buff = message + 4;
	}
	int readSendChunk(std::set<struct sockaddr_in,
		addr_comp> *clients_list);
};



#endif //DUZE_2__RADIOREADER_H_
