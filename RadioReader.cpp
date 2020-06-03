#include "RadioReader.h"
#include <unistd.h>
#include <iostream>
#include <cassert>
#include <cstring>

using namespace std;
using namespace std::chrono;

int RadioReader::readSendChunk(map<struct sockaddr_in, time_point<steady_clock>,
	addr_comp> *clients_map, int sockB, int timeoutClient){
	if(meta){
		return readSendMeta(clients_map, sockB, timeoutClient);
	}else{
		return readSendNoMeta(clients_map, sockB, timeoutClient);
	}
}

void RadioReader::sendMessages(bool audio, map<struct sockaddr_in, time_point<steady_clock>,
								  addr_comp> *clients_map, int sockB, int size, int timeout){
	if(audio){
		headerAUD[1] = ntohs(size);
		memcpy(message, headerAUD, 4);
	}else{
		headerMETA[1] = ntohs(size);
		memcpy(message, headerMETA, 4);
	}
	time_point<steady_clock> now = steady_clock::now();
	for(auto client = clients_map->begin();  client != clients_map->end(); ){
		cerr << (now - client->second).count()/1000000 <<"?" <<1000 * timeout<< "\n";
	 	if((now - client->second).count()/1000000 > 1000 * timeout){
	 		client = clients_map->erase(client);
			continue;
	 	}
		sendto(sockB, message, size + HEAD_SIZE, 0,
			   (struct sockaddr *) &(*client), (socklen_t) sizeof(*client));
	 	client++;
	}
}

int RadioReader::readSendNoMeta(map<struct sockaddr_in, time_point<steady_clock>,
									addr_comp> *clients_map, int sockB, int timeout){
	int rv = read(sockA, buff, MAX_META_SIZE);
	if(rv < 0){
		return -1;
	}
	int size = rv;
	if(to_cout) {
		rv = write(STDOUT_FILENO, buff, size);
	}else{
		sendMessages(true, clients_map, sockB, size, timeout);
	}
	return rv;
}

int RadioReader::readSendMeta(map<struct sockaddr_in, time_point<steady_clock>,
								  addr_comp> *clients_map, int sockB, int timeout){
	int toRead = metaInt - afterMeta - 1;
	if(toRead > MAX_META_SIZE){
		toRead = MAX_META_SIZE;
	}
	int rv = read(sockA, buff, toRead);
	if(rv < 0){
		return -1;
	}
	afterMeta += rv;
	int size = rv;
	if(to_cout){
		rv = write(STDOUT_FILENO, buff, size);
	}else{
		sendMessages(true, clients_map, sockB, size,timeout);
	}

	// read meta
	if(afterMeta == (metaInt - 1)){
		rv = read(sockA, buff, 1);
		if(rv < 0){
			return -1;
		}
		if(rv == 1) {
			if(readMeta(clients_map, sockB, timeout) < 0){
				return -1;
			}
		}

	}

	return rv;
}

// read number of bytes
int RadioReader::readMeta(map<struct sockaddr_in, time_point<steady_clock>,
							  addr_comp> *clients_map, int sockB, int timeout){
	int rv = read(sockA, buff, 1);
	if(rv < 0){
		return -1;
	}
	if(rv == 1){
		char meta_size = buff[0];
		rv = read(sockA, buff, 16 * meta_size);
		if(rv < 0){
			return -1;
		}
		assert(16 * meta_size == rv);
		int size = rv;
		if(to_cout){
			if(write(STDERR_FILENO, buff, size) < 0){
				return -1;
			}
		}else{
			sendMessages(false, clients_map, sockB, size, timeout);
		}
		afterMeta = 0;
	}
	return rv;
}
