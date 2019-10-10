#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
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

void* create_connection(void* socket);
void* read_message_queue();
void add_message_queue(char* message);
void add_client(client_node* client);
void remove_client(client_node* client);
void send_online_info(int sockfd);
void announce_joinning(char* message, char* username);
void announce_leaving(char* message, char* username);
FILE* create_log(char* username);
char* create_quit_message(char* username, int is_window);

char* message_queue[MAX_MESSAGE_QUEUE_LENGTH];
client_node* client_list[MAX_CLIENTS];

pthread_cond_t message_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t message_empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t message_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t conenection_lock = PTHREAD_MUTEX_INITIALIZER;

int num_clients = 0, front = 0, end = 0;

//in_socket is server's socket and connection_socket is the client's socket
int main(int argc, char* argv[]){

	int in_socket, connection_socket, address_size;
	struct sockaddr_in addr;
	pthread_t tid, message_tid;
	int port = 8080;
	int opt = 1;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	address_size = sizeof(addr);

	//Error checking for setting up socket
	if((in_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		perror("Failed to setup socket\n");
		exit(EXIT_FAILURE);
	}

	if((setsockopt(in_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) < 0){
		perror("Failed to set socket options\n");
		exit(EXIT_FAILURE);
	}

	if((bind(in_socket, (struct sockaddr *) &addr, (socklen_t) address_size)) < 0){
		perror("Failed to bind to in socket\n");
		exit(EXIT_FAILURE);
	}

	if((listen(in_socket, 0)) < 0){
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

		if((connection_socket = accept(in_socket, (struct sockaddr *) &addr, (socklen_t *) &address_size)) < 0){
			perror("Failed to accept connection \n");
			exit(EXIT_FAILURE);
		}

		if(num_clients >= MAX_CLIENTS)
			close(connection_socket);

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

//Creates log file
FILE* create_log(char* username){
	time_t current_time;
    char* c_time_string;
	char* log_name;
	FILE* logs;

	time(&current_time);
	if(current_time == ((time_t)-1)){
		fprintf(stderr, "Failed to obtain current time\n");
		exit(EXIT_FAILURE);
	}

	c_time_string = ctime(&current_time);
	if(c_time_string == NULL){
		fprintf(stderr, "Failed to convert current time\n");
		exit(EXIT_FAILURE);
	}

	log_name = (char*)malloc((strlen(username) + strlen(c_time_string) + 4) * sizeof(char));

	//Creating the logs name
	memcpy(log_name, username, strlen(username));
	memcpy(log_name + strlen(username), " ", 1);
	memcpy(log_name + strlen(username)+1, c_time_string, strlen(c_time_string)-1);
	memcpy(log_name + strlen(log_name),".txt", 5);

	logs = fopen(log_name, "w");
	fprintf(logs, "Logs for user: %s\nDate: %s\n", username, c_time_string);
	return logs;
}


//Creating thread for client connection
void* create_connection(void* socket){
	int* client_socket = socket;
	client_node* client = (client_node*)malloc(sizeof(client_node));
	char* username = malloc(MAX_NAME_LENGTH * sizeof(char));
	char* welcome_message = "==================== Welcome to the Server ====================\nPlease enter your username: ";
	char* message = malloc(sizeof(char) * MAX_BUFFER);
	char* raw_time;
	char* quit_message_window, *quit_message_other;
	FILE* logs;
	time_t current_time;
	char* c_time_str;

	if(write(*client_socket, welcome_message, strlen(welcome_message)) < 0){
		perror("Failed to send greetings\n");
		puts(strerror(errno));
	}

	memset(client, 0, sizeof(client_node));
	if(read(*client_socket, username, MAX_NAME_LENGTH) < 0){
		perror("Failed to read username from client\n");
		exit(EXIT_FAILURE);
	}

	username[strlen(username)-1] = '\0';
	client->name = username;
	client->own_socket = *client_socket;
	add_client(client);

	//Create a log file for client
	logs = create_log(username);

	send_online_info(*client_socket);
	announce_joinning(message, client->name);

	quit_message_window = create_quit_message(username, 1);
	quit_message_other = create_quit_message(username, 0);

	while(1){

		if((read(client->own_socket, message, MAX_BUFFER)) < 0){
			perror("Failed to read client message\n");
		}

		puts(message);
		if(strcmp(message, quit_message_window) == 0 || strcmp(message, quit_message_other) == 0){
			puts("quit");
			announce_leaving(message, client->name);
			break;
		}

		//obtain time when message was sent
		time(&current_time);
		c_time_str = ctime(&current_time);

		//remove \n from c_time_str
		raw_time = (char*)malloc((strlen(c_time_str)) * sizeof(char));
		memcpy(raw_time, c_time_str, strlen(c_time_str) -1);
		memcpy(raw_time + strlen(c_time_str), "\0", 1);

		fprintf(logs,"[%s]: %s", raw_time, message);
//		printf("Message %s Using socket: %d\n", message, client->own_socket);
		add_message_queue(message);
	}

	close(client->own_socket);
	remove_client(client);

	pthread_mutex_lock(&client_lock);
	num_clients--;
	pthread_mutex_unlock(&client_lock);


	free(client);
	free(message);
	free(username);
	free(quit_message_window);
	free(quit_message_other);
	fclose(logs);
	pthread_exit(0);
}

//Reads a message from message queue
void* read_message_queue(){
	while(1){
		pthread_mutex_lock(&message_lock);
		while(message_queue[front] == NULL)
			pthread_cond_wait(&message_full, &message_lock);

		pthread_mutex_lock(&client_lock);
		for(int m = 0; message_queue[m] != NULL && m < MAX_MESSAGE_QUEUE_LENGTH; m++){
			for(int i = 0; client_list[i] != NULL && i < MAX_CLIENTS; i++){
				if(write(client_list[i]->own_socket, message_queue[m], strlen(message_queue[m])) < 0){
					fprintf(stderr, "Filed to send message to socket: %d\n", client_list[i]->own_socket);
					exit(EXIT_FAILURE);
				}
			}

			memset(message_queue[m], 0, MAX_BUFFER);
			message_queue[m] = NULL;
		}

		end = 0;
		pthread_cond_signal(&message_empty);
		pthread_mutex_unlock(&message_lock);
		pthread_mutex_unlock(&client_lock);
	}
	pthread_exit(0);
}

//Add message to message queue
void add_message_queue(char* message){
	pthread_mutex_lock(&message_lock);
	while(message_queue[end] != NULL)
		pthread_cond_wait(&message_empty, &message_lock);
	message_queue[end] = message;
	end++;
	if(end == MAX_BUFFER)
		end = 0;
	pthread_cond_signal(&message_full);
	pthread_mutex_unlock(&message_lock);

}

//Add client to client list
void add_client(client_node* client){
	pthread_mutex_lock(&client_lock);
	for(int i = 0; i < MAX_CLIENTS; i++){
		if(client_list[i] == NULL){
			client_list[i] = client;
			num_clients++;
			break;
		}
	}
	pthread_mutex_unlock(&client_lock);
	printf("User: %s connected\n", client->name);
}

//Removes a client from list
void remove_client(client_node* client){
	pthread_mutex_lock(&client_lock);
	for(int i = 0; i < MAX_CLIENTS; i++){
		if(client_list[i] == client){
			client_list[i] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&client_lock);
	printf("User: %s disconnected\n", client->name);
}

//Sends online information to clients
void send_online_info(int sockfd) {
	char* online_list = malloc(sizeof(char) * MAX_BUFFER);

	strcat(online_list, "\n===========================================================\nUsers online: \n");
	for(int i = 0; client_list[i] != NULL && i < MAX_CLIENTS; i++){
		strcat(online_list, client_list[i]->name);
		strcat(online_list, "\n");
	}
	online_list[strlen(online_list)] = '\0';
	pthread_mutex_lock(&client_lock);
	if(write(sockfd, online_list, strlen(online_list)) < 0){
		fprintf(stderr, "Failed to send oline clients to socket: %d\n", sockfd);
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock(&client_lock);
	free(online_list);
}

/*
Create client quit message; checks if client is using a windows machine
quit_window: quit message for windows machine
quit_other: quit message for other machines
*/
char* create_quit_message(char* username, int is_window){
	int len;
	char* quit;

	quit = strdup(username);
	len = strlen(quit);
	quit[len] = ':';

	if(is_window)
		strcpy(quit + len + 1, "/quit\r\n");
	else
		strcpy(quit + len + 1, "/quit\n");
	return quit;
}

//Notify other clients when new client joins chat
void announce_joinning(char* message, char* username){
	sprintf(message, "%s has joined the chat\n", username);
	add_message_queue(message);
}

//Notify other clients when client leaves chat
void announce_leaving(char* message, char* username){
	sprintf(message, "%s has left the vchat\n", username);
	for(int i = 0; client_list[i] != NULL && i < MAX_CLIENTS; i++)
		write(client_list[i]->own_socket, message, strlen(message));
}
