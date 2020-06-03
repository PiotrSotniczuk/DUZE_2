#include "RadioReader.h"
#include <unistd.h>
#include <iostream>
#include <cassert>
#include <cstring>

using namespace std;

int RadioReader::readSendChunk(map<struct sockaddr_in, clock_t,
	addr_comp> clients_map, int sockB){
	if(meta){
		return readSendMeta(clients_map, sockB);
	}else{
		return readSendNoMeta(clients_map, sockB);
	}
}

int RadioReader::readSendNoMeta(map<struct sockaddr_in, clock_t,
									addr_comp> clients_map, int sockB){
	int rv = read(sockA, buff, MAX_META_SIZE);
	if(rv < 0){
		return -1;
	}
	int size = rv;
	if(to_cout) {
		rv = write(STDOUT_FILENO, buff, size);
	}else{
		headerAUD[1] = ntohs(size);
		memcpy(message, headerAUD, 4);
		for(auto clients : clients_map){
			sendto(sockB, buff, size, 0,
				   (struct sockaddr *) &clients, (socklen_t) sizeof(clients));
		}
	}
	return rv;
}

int RadioReader::readSendMeta(map<struct sockaddr_in, clock_t,
								  addr_comp> clients_map, int sockB){
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
		for(auto clients : clients_map){
			cout << "WYSYLAM COS META\n";
		}
	}

	// read meta
	if(afterMeta == (metaInt - 1)){
		rv = read(sockA, buff, 1);
		if(rv < 0){
			return -1;
		}
		if(rv == 1) {
			if(readMeta(clients_map, sockB) < 0){
				return -1;
			}
		}

	}

	return rv;
}

// read number of bytes
int RadioReader::readMeta(map<struct sockaddr_in, clock_t,
							  addr_comp> clients_map, int sockB){
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
			for(auto clients : clients_map){
				cout << "WYSYLAM COS MET STDERRA\n";
			}
		}
		afterMeta = 0;
	}
	return rv;
}
