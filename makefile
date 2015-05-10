server:server.o
	@g++ server.cc -g -o server
clean:
	@rm *.o server 
all:server.o 
	@g++ server.cc -g -o server
