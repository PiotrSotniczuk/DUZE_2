#include "parse_args.h"
#include <cstdlib>
#include <cstring>
#include "err.h"

#define ESSENTIALS_SUM 3
#define ESSEN_H 0
#define ESSEN_R 1
#define ESSEN_P 2

using namespace std;

void checkEssen(bool *essentials, int nr_essen){
	if(essentials[nr_essen]){
		fatal("Double set of argument");
	}
	essentials[nr_essen] = true;
}

void set_args(int argc, char** argv, string *host, string *resource,
	string *port, bool *metaData, unsigned long int *timeout){
	bool essentials[ESSENTIALS_SUM];
	if(argc < ESSENTIALS_SUM*2 + 1){
		fatal("Bad number of args");
	}

	// only A
	memset(essentials, false, 3);
	for(int i = 1; i < argc; i = i + 2){
		if(strcmp(argv[i], "-h") == 0){
			checkEssen(essentials, ESSEN_H);
			(*host) = argv[i+1];
		}

		if(strcmp(argv[i], "-r") == 0){
			checkEssen(essentials, ESSEN_R);
			(*resource) = argv[i+1];
		}

		if(strcmp(argv[i], "-p") == 0){
			checkEssen(essentials, ESSEN_P);
			(*port) = argv[i+1];
		}

		if(strcmp(argv[i], "-m") == 0){
			if(strcmp(argv[i+1], "yes") == 0){
				(*metaData) = true;
			}else{
				if(strcmp(argv[i+1], "no") == 0){
					(*metaData) = false;
				}else{
					fatal("Argument -m is not yes|no");
				}
			}
		}

		if(strcmp(argv[i], "-t") == 0){
			char *endptr;
			(*timeout) = strtoul(argv[i+1], &endptr, 10);
			if(endptr == argv[i+1]){
				fatal("Argument -t bad timeout");
			}
		}
	}

	for(bool essential : essentials){
		if(!essential){
			fatal("Missing essential argument");
		}
	}
}