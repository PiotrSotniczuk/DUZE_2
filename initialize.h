#ifndef DUZE_2__INITIALIZE_H_
#define DUZE_2__INITIALIZE_H_

#include <string>
// checks and sets given arguments
void set_args(int argc, char** argv, std::string *host, std::string *resource,
			  std::string *portA, std::string *meta,
			  unsigned long int *timeoutA, std::string *portB,
			  std::string *multi, unsigned long int *timeoutB);

// instructions from labs combined into one function to initialize socket
int get_socket(const char *connect_adr, const char *port);

//
void read_header(FILE *response, std::string *meta, unsigned long int *metaInt,
				 int sockA);

#endif //DUZE_2__INITIALIZE_H_
