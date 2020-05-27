#ifndef DUZE_2__PARSE_ARGS_H_
#define DUZE_2__PARSE_ARGS_H_

#include <string>
// checks and sets given arguments
void set_args(int argc, char** argv, std::string *host, std::string *resource,
			  std::string *portA, std::string *meta,
			  unsigned long int *timeoutA, std::string *portB,
			  std::string *multi, unsigned long int *timeoutB);

#endif //DUZE_2__PARSE_ARGS_H_
