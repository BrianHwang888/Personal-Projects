#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BUFFER 1024
#define MAX_NAMELEN 24

void send_username(char* read_buffer, char* name, int socket);

int main(void){
	char* writing_buffer, *reading_buffer, *message, *username;
	int socketfd, namelen, bits, done = 0;
	char* port = "8080";
	char* ip = "127.0.0.1"; 
	struct sockaddr_in ser_addr;
	fd_set readfds;

	reading_buffer = malloc(sizeof(char) * MAX_BUFFER);
	writing_buffer = malloc(sizeof(char) * MAX_BUFFER);
	username = malloc(sizeof(char) * MAX_NAMELEN);
	message = malloc(sizeof(char) * MAX_BUFFER);

	//attempt to create socket for the client
	if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Failed to create socket for client");
		exit(EXIT_FAILURE);
	}

	memset(&ser_addr, '0', sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons((int)strtol(port, (char **)NULL, 10));

	if((inet_pton(AF_INET, ip, &ser_addr.sin_addr)) <= 0){
		perror("Not a valid ip address\n");
		exit(EXIT_FAILURE);
	}

	if((connect(socketfd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))) < 0){
		perror("Failed to connect to server\n");
		exit(EXIT_FAILURE);
	}

	send_username(reading_buffer, username, socketfd);
	namelen = strlen(username);
	strcpy(writing_buffer, username);
	writing_buffer[namelen-1] = ':';

	FD_CLR(STDIN_FILENO, &readfds);
	FD_CLR(socketfd, &readfds);

	while(!done){
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		FD_SET(socketfd, &readfds);

		if((select(FD_SETSIZE, &readfds, NULL, NULL, NULL)) < 0){
			perror("Failed to use select function\n");
			exit(EXIT_FAILURE);
		}

		if(FD_ISSET(socketfd, &readfds) && !done){
			if((bits = read(socketfd, reading_buffer, MAX_BUFFER)) < 0){
				perror("Failed to read server message\n");
			}
			if(bits == 0)
				break;
			puts(reading_buffer);
			memset(reading_buffer, 0, MAX_BUFFER);
		}

		if(FD_ISSET(STDIN_FILENO, &readfds)){
			read(STDIN_FILENO, message, MAX_BUFFER);
			strcpy(writing_buffer + namelen, message);
			write(socketfd, writing_buffer, MAX_BUFFER);
			memset(message, 0, MAX_BUFFER);
			if(strcmp(message, "/quit\r\n") == 0 || strcmp(message, "/quit\n") == 0){
				FD_CLR(socketfd, &readfds);
				done = 1;
			}
		}

	}
	free(reading_buffer);
	free(writing_buffer);
	free(message);
	free(username);
	close(socketfd);
	exit(EXIT_SUCCESS);

}

void send_username(char* read_buffer, char* name, int socket){
	fd_set readfds;
	read(socket, read_buffer, MAX_BUFFER);
	puts(read_buffer);
	memset(read_buffer, 0, MAX_BUFFER);
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
	if(FD_ISSET(STDIN_FILENO, &readfds)){
		read(STDIN_FILENO, name, MAX_NAMELEN);
		write(socket, name, strlen(name));
	}
}
