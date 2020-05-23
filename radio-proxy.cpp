#include <string>
#include "parse_args.h"


using namespace std;

int main(int argc, char* argv[]) {

	string host;
	string resource;
	unsigned long int port;
	bool metaData = false;
	unsigned long int timeout = 5;

	set_args(argc, argv, &host, &resource, &port, &metaData, &timeout);



	return 0;
}
