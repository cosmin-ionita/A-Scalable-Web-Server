/*
	This is a scalable implementation of a webserver that simulates
	an I/O intensive and a CPU intensive workload.

	The server scales horizontally using MPI and each request is 
	processed using pthreads (for fiability purposes).
*/

#include <stdio.h>
#include <string.h>    	
#include <stdlib.h>    	
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h> 
#include <unistd.h>   
#include <pthread.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

#include <time.h>

#include <mpi.h>

#include <fcntl.h>

#define MAX_CLIENT_SOCK 100000

/* Those variables refer only to MPI context */

int process_count; 	/* The total number of MPI processes*/
int process_id;		/* The id of the current MPI process */

int next_machine_id = 0;

long double cpu_usage = 0;
 
/* The thread function */
void *connection_handler(void *);

/* The thread function of slave processing */
void *slave_handler(void *load_size);

/* The thread function of slave dispatching */
void *slave_dispatcher(void *socket_desc);
 
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

void InitializeMPIContext()
{
    int provided;

    /* Initialize the MPI environment */
    MPI_Init(NULL, NULL);

    /* Get the number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    /* Get the rank of the current process */
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
}


void *update_cpu_usage(void *dummy_param)
{
    long double a[4], b[4], loadavg;
    
    FILE *fp;
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    for(;;)
    {
        fp = fopen("/proc/stat","r");
        fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3]);
        fclose(fp);
        sleep(1);

        fp = fopen("/proc/stat","r");
        fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&b[0],&b[1],&b[2],&b[3]);
        fclose(fp);

	time(&timer);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        loadavg = ((b[0]+b[1]+b[2]) - (a[0]+a[1]+a[2])) / ((b[0]+b[1]+b[2]+b[3]) - (a[0]+a[1]+a[2]+a[3]));
        
        cpu_usage = loadavg * 100;

	if(cpu_usage >= 30)
		next_machine_id = 1;
	//else
	//	next_machine_id = 0;
    }
}

int main(int argc , char *argv[])
{
    
    int client_socks[MAX_CLIENT_SOCK];
    int socket_desc , client_sock , c, i;
    struct sockaddr_in server , client;

    pthread_t thread_id;

    /* Create the MPI environment */	
    InitializeMPIContext();


    /* This section handles the MASTER logic */
    if(process_id == 0)
    {

	    /* Power on the CPU analyzer thread */
	    if( pthread_create( &thread_id , NULL ,  update_cpu_usage , NULL) < 0)
	    {
		    perror("Could not create the cpu_analyzer thread");
		    return 1;
	    }

	    /* We will use this named pipe to pass the parallel clients count to the cpu analyzer */
	    //mkfifo(named_pipe_name, 0666);

	    //printf("Waiting for the other end of the named pipe...");
	    //named_pipe_fd = open(named_pipe_name, O_WRONLY);


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


	    int fd; struct ifreq ifr;
	    fd = socket(AF_INET, SOCK_DGRAM, 0);
 	    ifr.ifr_addr.sa_family = AF_INET;
 	    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
 	    ioctl(fd, SIOCGIFADDR, &ifr);
	    close(fd);


	    printf("[MASTER] IP Address: %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)); 

	  
	    while( (client_socks[i] = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	    {

		if(next_machine_id != 0)
		{
			if( pthread_create( &thread_id , NULL ,  slave_dispatcher , (void*) &(client_socks[i])) < 0)
			{
			    perror("Could not create the client handler thread");
			    return 1;
			}
		}
		else
		{
			/*_____LOCK_IN_____
			pthread_mutex_lock(&lock);

			parallel_clients += 1;

			write(named_pipe_fd, &parallel_clients, sizeof(int));

			pthread_mutex_unlock(&lock);

			_____LOCK_OUT_____*/

			if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &(client_socks[i])) < 0)
			{
			    perror("Could not create the client handler thread");
			    return 1;
			}
		 
			i++;

			if(i == MAX_CLIENT_SOCK)
				i = 0;
		}
	    }

	    MPI_Finalize();
	     
	    return 0;

    } else {	/* This is the SLAVE code */

	int load_size = 0;
	pthread_t thread_id;

	// Get the name of the processor
	char processor_name[MPI_MAX_PROCESSOR_NAME];
        int name_len;
        MPI_Get_processor_name(processor_name, &name_len);

	int fd; struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
 	ifr.ifr_addr.sa_family = AF_INET;
 	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
 	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	printf("[SLAVE] IP Address: %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	while(1)
	{
		
		printf("[SLAVE], waiting for data from the master. Slave rank: %d\n", process_id);

		MPI_Recv(&load_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);

		printf("[SLAVE], received from the master: %d...\n", load_size);

		if( pthread_create( &thread_id , NULL ,  slave_handler , (void*) &load_size) < 0)
		{
		    perror("Could not create the client handler thread");
		    return 1;
		}
	}
    }
}

void *slave_handler(void *load_size)
{
	int size = *(int*)load_size;

	printf("[SLAVE], Execute the load with size: %d...\n", size);


	matrixMultiplication(size);

	int ack = 100;


	printf("[SLAVE], Load processed! Sending back the result\n");

	MPI_Send(&ack, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

/* This is only run by the master process */
void *slave_dispatcher(void *socket_desc)
{
    /* Get the socket descriptor */
    int sock = *(int*)socket_desc;
    int read_size;
    char filename[100];

    char *message , client_message[4096];

    printf("[MASTER] on slave dispatcher\n");

    while( (read_size = recv(sock , client_message , 4096 , 0)) > 0 )
    {
	client_message[read_size] = '\0';


	/* If we have a CPU intesive request */
	if(strlen(client_message) < 5) 
	{	
		int matrix_size = atoi(client_message);
		int response = 0;

		printf("[MASTER] sending %d to the slave: %d\n", matrix_size, next_machine_id);
		/* Send the load data to the slave*/
		MPI_Send(&matrix_size, 1, MPI_INT, next_machine_id, 0, MPI_COMM_WORLD);


		/* Wait for the result */
		MPI_Recv(&response, 1, MPI_INT, next_machine_id, 0, MPI_COMM_WORLD, NULL);

		printf("[MASTER] received response from the slave: %d\n", response);
		/* Write an ACK response */
		write(sock , &response , sizeof(int));
		
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

    /*________LOCK_IN__________
    pthread_mutex_lock(&lock);

    parallel_clients -= 1;
    write(named_pipe_fd, &parallel_clients, sizeof(int));

    pthread_mutex_unlock(&lock);
    ________LOCK_OUT__________*/

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

    /*________LOCK_IN__________
    pthread_mutex_lock(&lock);

    parallel_clients -= 1;
    write(named_pipe_fd, &parallel_clients, sizeof(int));

    pthread_mutex_unlock(&lock);
    ________LOCK_OUT__________*/

    return 0;
} 
