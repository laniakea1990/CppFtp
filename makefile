server:server.cc ThreadPoll.cc
	@g++ -g -pthread -std=c++11 server.cc ThreadPoll.cc -o server
clean:
	@rm *.o server 
all:server.cc ThreadPoll.cc
	@g++ -g -pthread -std=c++11 server.cc ThreadPoll.cc -o server
