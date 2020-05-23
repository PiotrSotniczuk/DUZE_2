#ifndef DUZE_2__PARSE_ARGS_H_
#define DUZE_2__PARSE_ARGS_H_

#include <string>
// checks and sets given arguments
void set_args(int argc, char** argv, std::string *host, std::string *resource,
			  unsigned long int *port, bool *metaData,
			  unsigned long int *timeout);

#endif //DUZE_2__PARSE_ARGS_H_
