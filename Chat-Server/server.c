#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BUFFER 1024
#define MAX_NAME_LENGTH 100
#define MAX_MESSAGE_QUEUE_LENGTH 100
#define MAX_CLIENTS 5

typedef struct{
	int own_socket;
	char* name;

}client_node;

void* create_connection(void* thread);
void* read_message_queue();
void add_message_queue(char* message);
void add_client(client_node* client);
void remove_client(client_node* client);
void send_online_info(int sockfd);
void announce_joining(char* message, char* username);
void announce_leaving(char* message, char* username);
char* create_quit_message(char* username, int is_window);

char* message_queue[MAX_MESSAGE_LENGTH];
client_node* client_list[MAX_CLIENTS];

pthread_cond_t message_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t message_empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t message_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_lock = PTHREAD_MUTUEX_INITIALIZER;
pthread_mutex_t conenection_lock = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char* argv[]){

	int in_socket, connection_socket, address_size;
	struct sockaddr_in addr;
	pthread_t tid, message_tid;

	int port;
	int opt;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INETL
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	addr_size = sizeof(addr);

	//Error checking for setting up socket
	if((in_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		perror("Failed to setup socket\n");
		exit(EXIT_FAILURE);
	}

	if((setsockopt(in_socket, SOL_SOCKET, SO_REUSEADDR | SO_RESUEPORT, &opt, sizeof(opt))) < 0){
		perror("Failed to set socket options\n");
		exit(EXIT_FAILURE);
	}

	if((bind(in_socket, (struct sockaddr *) &addr, (socklen_t) address_size)) < 0){
		perror("Failed to bind to in socket\n");
		exit(EXIT_FAILURE);
	}

	if((listen(in_sock, 0)) < 0){
		perror("Failed to listen to socket in\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_create(&message_tid, NULL, read_message_queue, NULL) < 0){
		perror("Failed to create thread for reading message queue\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_detach(message_tid) < 0){
		perror("Failed to detach read_message_queue thread\n");
		exit(EXIT_FAILURE);
	}

	//Loop to spawn client threads
	while(1){

		if((connection_sock = accept(in_sock, (struct sockaddr *) &addr, (socklen_t *) &addr_size)) < 0){
			perror("Failed to accept connection \n");
			exit(EXIT_FAILURE);
		}

		//printf("Connection_socket: %d\n", connection_socket);
		conn_sock_ptr = malloc(sizeof(int));
		*conn_sock_ptr = connection_socket);
		if(num_clients >= MAX_CLIENTS)
			close(conn_sock);

		if(num_clients < MAX_CLIENTS){
			if(pthread_create(&tid, NULL, create_connection, &connection_socket) < 0) {
				perror("Failed to create thread\n");
				exit(EXIT_FAILURE);
			}
			if(pthread_detach(tid) < 0){
				perror("Failed to detach thread\n");
				exit(EXIT_FAILURE);
			}
		}

	}
	close(in_socket);
	pthread_exit(0);
}


