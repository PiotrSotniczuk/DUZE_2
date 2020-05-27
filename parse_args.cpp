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
	string *portA, string *meta, unsigned long int *timeoutA, string *portB,
	string *multi, unsigned long int *timeoutB){
	bool essentials[ESSENTIALS_SUM];
	if(argc < ESSENTIALS_SUM*2 + 1){
		fatal("Bad number of args");
	}

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
			(*portA) = argv[i+1];
		}

		if(strcmp(argv[i], "-m") == 0){
			if(strcmp(argv[i+1], "yes") == 0){
				(*meta) = "1";
			}else{
				if(strcmp(argv[i+1], "no") == 0){
					(*meta) = "0";
				}else{
					fatal("Argument -m is not yes|no");
				}
			}
		}

		if(strcmp(argv[i], "-t") == 0){
			char *endptr;
			(*timeoutA) = strtoul(argv[i+1], &endptr, 10);
			if(endptr == argv[i+1]){
				fatal("Argument -t bad timeout");
			}
		}

		if(strcmp(argv[i], "-P") == 0){
			(*portB) = argv[i+1];
		}

		if(strcmp(argv[i], "-B") == 0){
			(*multi) = argv[i+1];
		}

		if(strcmp(argv[i], "-T") == 0){
			char *endptr;
			(*timeoutB) = strtoul(argv[i+1], &endptr, 10);
			if(endptr == argv[i+1]){
				fatal("Argument -T bad timeout");
			}
		}
	}

	for(bool essential : essentials){
		if(!essential){
			fatal("Missing essential argument");
		}
	}
}