#include "RadioReader.h"
#include <unistd.h>
#include <iostream>
#include <cassert>

using namespace std;

int RadioReader::readSendChunk(set<struct sockaddr_in,
	addr_comp> *clients_list){
	if(meta){
		return readSendMeta(*clients_list);
	}else{
		return readSendNoMeta(*clients_list);
	}
}

int RadioReader::readSendNoMeta(set<struct sockaddr_in,
									addr_comp> *clients_list){
	int rv = read(sockA, buff, MAX_META_SIZE);
	if(rv < 0){
		return -1;
	}
	int size = rv;
	if(to_cout) {
		rv = write(STDOUT_FILENO, buff, size);
	}else{
		// TODO zakoduj wysylanie
		for(sockaddr_in clients : clients_list){
			cout << "WYSYLAM COS NO META\n";
		}
	}
	return rv;
}

int RadioReader::readSendMeta(set<struct sockaddr_in,
								  addr_comp> *clients_list){
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
		for(sockaddr_in clients : clients_list){
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
			if(readMeta(clients_list) < 0){
				return -1;
			}
		}

	}

	return rv;
}

// read number of bytes
int RadioReader::readMeta(set<struct sockaddr_in,
							  addr_comp> *clients_list){
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
			for(sockaddr_in clients : clients_list){
				cout << "WYSYLAM COS MET STDERRA\n";
			}
		}
		afterMeta = 0;
	}
	return rv;
}
