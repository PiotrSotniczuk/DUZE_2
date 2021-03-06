#ifndef DUZE_2__RADIOREADER_H_
#define DUZE_2__RADIOREADER_H_

#include <string>
#include <map>
#include <netdb.h>
#include <chrono>
#define MAX_META_SIZE 4080
#define HEAD_SIZE 4

#define AUDIO 4
#define METADATA 6


struct addr_comp {
	bool operator() (const sockaddr_in lhs,
					 const sockaddr_in rhs) const {
		bool leftSmaller = false;
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

class RadioReader {
 private:
	int sockA;
	char *buff;
	short headerAUD[HEAD_SIZE];
	short headerMETA[HEAD_SIZE];
	char message[MAX_META_SIZE + HEAD_SIZE];
	bool to_cout;
	bool meta;
	int metaInt;
	int afterMeta;
	int readSendNoMeta(std::map<struct sockaddr_in, std::chrono::time_point<std::chrono::steady_clock>, addr_comp> *clients_map, int sockB, int timeout);
	int readSendMeta(std::map<struct sockaddr_in, std::chrono::time_point<std::chrono::steady_clock>, addr_comp> *clients_map, int sockB, int timeout);
	int readMeta(std::map<struct sockaddr_in, std::chrono::time_point<std::chrono::steady_clock>, addr_comp> *clients_map, int sockB, int timeout);
	void sendMessages(bool audio,
		std::map<struct sockaddr_in, std::chrono::time_point<std::chrono::steady_clock>, addr_comp> *clients_map, int sockB, int size, int timeout);

 public:
	RadioReader(bool meta, int metaInt,
		int sockA, bool to_cout) {
		this->meta = meta;
		this->metaInt = metaInt;
		this->sockA = sockA;
		this->to_cout = to_cout;
		afterMeta = 0;
		buff = message + HEAD_SIZE;
		headerAUD[0] = ntohs(AUDIO);
		headerMETA[0] = ntohs(METADATA);

	}
	int readSendChunk(std::map<struct sockaddr_in, std::chrono::time_point<std::chrono::steady_clock>, addr_comp> *clients_map, int sockB, int timeoutClient);
};



#endif //DUZE_2__RADIOREADER_H_
