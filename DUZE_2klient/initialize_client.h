#ifndef DUZE_2KLIENT__INITIALIZE_CLIENT_H_
#define DUZE_2KLIENT__INITIALIZE_CLIENT_H_

#include <string>
void set_args_client(int argc, char** argv, std::string *host,
							std::string *portB, std::string *portC, int *timeoutA);

int get_socketB(const std::string& host, const std::string& portB, struct sockaddr_in *my_address);


// 0 - UDP from sender
// 1 - socket after connect
// 2- socket listening for new telnets
void init_poll_client(struct pollfd *poll_tab, int sockB, int sockC);

int init_sockC(const std::string& portC);

#endif
