#ifndef DUZE_2KLIENT__INITIALIZE_CLIENT_H_
#define DUZE_2KLIENT__INITIALIZE_CLIENT_H_

#include <string>
void set_args_client(int argc, char** argv, std::string *host,
							std::string *portB, std::string *portC, int *timeoutA);

int get_socketB(const std::string& host, const std::string& portB, struct sockaddr_in *my_address);
void init_poll_client(const std::string& portC, struct pollfd *poll_tab);

#endif
