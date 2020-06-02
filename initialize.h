#ifndef DUZE_2__INITIALIZE_H_
#define DUZE_2__INITIALIZE_H_

#include <string>
// checks and sets given arguments
void set_args(int argc, char** argv, std::string *host, std::string *resource,
			  std::string *portA, bool *metaData,
			  int *timeoutA, std::string *portB,
			  std::string *multi, int *timeoutB);

// instructions from labs combined into one function to initialize socket
int get_socket(const char *connect_adr, const char *port);

// reads header
void read_header(bool *metaData, std::string *name, int *metaInt,
				 int sockA);

// initiates poll table
void init_poll(int sockA, const std::string& portB, struct pollfd *poll_tab,
			   struct sockaddr_in *server);

#endif //DUZE_2__INITIALIZE_H_
