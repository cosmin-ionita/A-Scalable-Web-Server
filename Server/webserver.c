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
 
//the thread function
void *connection_handler(void *);
 
pthread_mutex_t lock;
int parallel_clients = 0;


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

    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {

	/* Critical section */
	pthread_mutex_lock(&lock);

		parallel_clients += 1;
		printf("Parallel clients: %d\n", parallel_clients);

	pthread_mutex_unlock(&lock);
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
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
    char *message , client_message[2000];

    //setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (int[]){12}, sizeof(int));
     
    //Receive a message from client
    while( (read_size = recv(sock , client_message , 4 , 0)) > 0 )
    {
	client_message[read_size] = '\0';

	printf("Received from client: %s\n", client_message);

	if(strlen(client_message) < 5) /* We have a CPU intensive request */
	{	
		int matrix_size = atoi(client_message);

		matrixMultiplication(matrix_size);

		//Send the message back to client
		write(sock , client_message , strlen(client_message));
		
		//clear the message buffer
		memset(client_message, 0, 2000);

		shutdown(sock, SHUT_RDWR);
	}
	else 			 	/* We have an I/O intensive request */
	{

	}
    }
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }

    else if(read_size == -1)
    {
        perror("recv failed");
    }

	pthread_mutex_lock(&lock);

	parallel_clients -= 1;
	printf("Parallel clients: %d\n", parallel_clients);

	pthread_mutex_unlock(&lock);
         
    return 0;
} 
