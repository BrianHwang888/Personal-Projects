.PHONY : clean

server: server.c
		gcc -o server -std=c99 -Wall -g -D_BSD_SOURCE -pthread server.c

client: client.c
		gcc -o client -std=c99 -Wall -g -D_BSD_SOURCE -pthread client.c

clean :
	rm -f client
	rm -f server

