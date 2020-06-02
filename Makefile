EXECS = radio-proxy radio-client

CXX = g++
CPPFLAGS = -Wall -Wextra -O2 -std=c++17
LDFLAGS	=
LDLIBS =

TARGET: $(EXECS)

radio-proxy.o err.o parse_args.o radio-client.o RadioReader.o: err.h

radio-proxy: radio-proxy.o err.o initialize.o RadioReader.o
	$(CXX) $(CPPFLAGS) radio-proxy.o err.o initialize.o RadioReader.o -o radio-proxy

radio-client: radio-client.o err.o
	$(CXX) $(CPPFLAGS) radio-client.o err.o -o radio-client


.PHONY: clean TARGET
clean:
	rm -f $(EXECS) *.o *~