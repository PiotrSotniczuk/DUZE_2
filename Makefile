EXECS = radio-proxy

CXX = g++
CPPFLAGS = -Wall -Wextra -O2 -std=c++17
LDFLAGS	=
LDLIBS =

TARGET: $(EXECS)

radio-proxy.o err.o parse_args.o: err.h

radio-proxy: radio-proxy.o err.o parse_args.o
	$(CXX) $(CPPFLAGS) radio-proxy.o err.o parse_args.o -o $(EXECS)

.PHONY: clean TARGET
clean:
	rm -f $(EXECS) *.o *~