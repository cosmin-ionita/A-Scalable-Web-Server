/*
    C socket server example, handles multiple clients using threads
    Compile
    gcc server.c -lpthread -o server
*/
 
#include<stdio.h>
#include<string.h>    	//strlen
#include<stdlib.h>    	//strlen
#include<sys/socket.h>
#include<arpa/inet.h> 	//inet_addr
#include<unistd.h>    	//write
#include<pthread.h> 	//for threading , link with lpthread


#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>

 
//the thread function
void *connection_handler(void *);
 
pthread_mutex_t lock;
int parallel_clients = 0;


int named_pipe_fd;
char *named_pipe_name = "/tmp/web_clients";


void matrixMultiplication(int sizeOfMatrix) {
	int i, j, k;
	int **A = (int **)malloc(sizeOfMatrix * sizeof(int *));
	int **B = (int **)malloc(sizeOfMatrix * sizeof(int *));
	int **C = (int **)malloc(sizeOfMatrix * sizeof(int *));
	for (i = 0; i < sizeOfMatrix; i++) {
		A[i] = (int *)malloc(sizeOfMatrix * sizeof(int));
		B[i] = (int *)malloc(sizeOfMatrix * sizeof(int));
		C[i] = (int *)malloc(sizeOfMatrix * sizeof(int));
	}

	for (i = 0; i < sizeOfMatrix; i++) {
		for (j = 0; j < sizeOfMatrix; j++) {
			A[i][j] = rand() % 30;
			B[i][j] = rand() % 30;
			C[i][j] = 0;
		}
	}
	
	for(i = 0; i < sizeOfMatrix; i++)
		for(j = 0; j < sizeOfMatrix; j++)
		    for(k = 0; k < sizeOfMatrix; k++) {
		        C[i][j] += A[i][k] * B[k][j];
		    }

}

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;

    /* We will use this named pipe to pass the parallel clients count to the cpu analyzer*/
    mkfifo(named_pipe_name, 0666);

    named_pipe_fd = open(named_pipe_name, O_WRONLY);


    /* This mutex is needed for parallel clients atomicity */
    pthread_mutex_init(&lock, NULL);

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
	
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    pthread_t thread_id;

    int client_socks[100000];

    int i = 0;

    while( (client_socks[i] = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {

	/*_____LOCK_IN_____*/
	pthread_mutex_lock(&lock);

	parallel_clients += 1;

	write(named_pipe_fd, &parallel_clients, sizeof(int));

	pthread_mutex_unlock(&lock);

	/*_____LOCK_OUT_____*/

        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &(client_socks[i])) < 0)
        {
            perror("could not create thread");
            return 1;
        }
 
	i++;
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char filename[100];

    char *message , client_message[4096];

    while( (read_size = recv(sock , client_message , 4096 , 0)) > 0 )
    {
	client_message[read_size] = '\0';

	if(strlen(client_message) < 5) 
	{	
		int matrix_size = atoi(client_message);

		matrixMultiplication(matrix_size);

		write(sock , client_message , strlen(client_message));
		
		memset(client_message, 0, 4096);

		shutdown(sock, SHUT_RDWR);
	}
	else 			 	
	{
		strcpy(filename, "Content/");
		strcat(filename, client_message);

		int file_fd = open(filename, O_RDONLY);
		int read_bytes = 0;
	
		char buffer[100];

		while( (read_bytes = read(file_fd, buffer, 100)) > 0 )
		{
			write(sock , buffer , read_bytes);
		}

		sleep(10);

		close(file_fd);

		shutdown(sock, SHUT_RDWR);
	}
    }

    /*________LOCK_IN__________*/
    pthread_mutex_lock(&lock);

    parallel_clients -= 1;
    write(named_pipe_fd, &parallel_clients, sizeof(int));

    pthread_mutex_unlock(&lock);
    /*________LOCK_OUT__________*/

    return 0;
} 
