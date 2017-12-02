/*
	This is a scalable implementation of a webserver that simulates
	an I/O intensive and a CPU intensive workload.

	The server scales horizontally using MPI and each request is 
	processed using pthreads (for fiability purposes).
*/

#include<stdio.h>
#include<string.h>    	
#include<stdlib.h>    	
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>   
#include<pthread.h>


#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>

 
/* The thread function */
void *connection_handler(void *);
 
/* Parallel clients count with the associated lock */
pthread_mutex_t lock;
int parallel_clients = 0;

/* Named pipe to send the parallel clients count */
int named_pipe_fd;
char *named_pipe_name = "/tmp/web_clients";

/* CPU intensive load */
void matrixMultiplication(int sizeOfMatrix) 
{
	int i, j, k;
	int **A = (int **)malloc(sizeOfMatrix * sizeof(int *));
	int **B = (int **)malloc(sizeOfMatrix * sizeof(int *));
	int **C = (int **)malloc(sizeOfMatrix * sizeof(int *));
	for (i = 0; i < sizeOfMatrix; i++) 
	{
		A[i] = (int *)malloc(sizeOfMatrix * sizeof(int));
		B[i] = (int *)malloc(sizeOfMatrix * sizeof(int));
		C[i] = (int *)malloc(sizeOfMatrix * sizeof(int));
	}

	for (i = 0; i < sizeOfMatrix; i++) 
	{
		for (j = 0; j < sizeOfMatrix; j++) 
		{
			A[i][j] = rand() % 30;
			B[i][j] = rand() % 30;
			C[i][j] = 0;
		}
	}
	
	for(i = 0; i < sizeOfMatrix; i++)
		for(j = 0; j < sizeOfMatrix; j++)
		    for(k = 0; k < sizeOfMatrix; k++) 
		    {
		        C[i][j] += A[i][k] * B[k][j];
		    }

}

int main(int argc , char *argv[])
{
    
    int client_socks[100000];
    int socket_desc , client_sock , c, i;
    struct sockaddr_in server , client;

    pthread_t thread_id;

    /* We will use this named pipe to pass the parallel clients count to the cpu analyzer*/
    mkfifo(named_pipe_name, 0666);

    printf("Waiting for the other end of the named pipe...");
    named_pipe_fd = open(named_pipe_name, O_WRONLY);


    /* This mutex is needed for parallel clients atomicity */
    pthread_mutex_init(&lock, NULL);

    /* Create socket */
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    puts("Socket created");
     
    /* Prepare the sockaddr_in structure */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
	
    /* Bind */
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }

    puts("bind done");
     
    /* Listen */
    listen(socket_desc , 3);
     
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
   
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

  
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
 
/* This will handle connection for each client */
void *connection_handler(void *socket_desc)
{
    /* Get the socket descriptor */
    int sock = *(int*)socket_desc;
    int read_size;
    char filename[100];

    char *message , client_message[4096];

    while( (read_size = recv(sock , client_message , 4096 , 0)) > 0 )
    {
	client_message[read_size] = '\0';


	/* If we have a CPU intesive request */
	if(strlen(client_message) < 5) 
	{	
		int matrix_size = atoi(client_message);

		/* Launch the load */
		matrixMultiplication(matrix_size);

		/* Write an ACK response */
		write(sock , client_message , strlen(client_message));
		
		memset(client_message, 0, 4096);

		/* Close the connection */
		shutdown(sock, SHUT_RDWR);
	}
	/* If we have a I/O intensive request */
	else 			 	
	{
		/* Open the requested file */
		strcpy(filename, "Content/");
		strcat(filename, client_message);

		int file_fd = open(filename, O_RDONLY);
		int read_bytes = 0;
	
		char buffer[100];
		
		/* Send it to the client */
		while( (read_bytes = read(file_fd, buffer, 100)) > 0 )
		{
			write(sock , buffer , read_bytes);
		}

		sleep(10);

		close(file_fd);

		/* Close the connection */
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
