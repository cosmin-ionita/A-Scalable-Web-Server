#define _XOPEN_SOURCE 700

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include<pthread.h>

#include <unistd.h>
#include <fcntl.h>

#define PORT 8888

enum CONSTEXPR { MAX_REQUEST_LEN = 4096};

void *connection_handler(void *);

void *connection_handler(void *workload)
{
	/* Necessary local variables */
	char buffer[100000];
	char request[MAX_REQUEST_LEN];

	struct protoent *protoent;
	char *hostname = "localhost";

	in_addr_t in_addr;

	int request_len;
	int socket_file_descriptor;

	ssize_t nbytes_total, nbytes_last;

	struct hostent *hostent;
	struct sockaddr_in sockaddr_in;

	unsigned short server_port = PORT;


	/* Build the socket. */				/* SET_SOCKET */
	protoent = getprotobyname("tcp");
	if (protoent == NULL) {
		perror("getprotobyname");
		exit(EXIT_FAILURE);
	}

	socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);


	//setsockopt(socket_file_descriptor, SOL_SOCKET, SO_SNDBUF, (int[]){10}, sizeof(int));


	if (socket_file_descriptor == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Build the address. */
	hostent = gethostbyname(hostname);
	if (hostent == NULL) {
		fprintf(stderr, "error: gethostbyname(\"%s\")\n", hostname);
	}

	in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));

	if (in_addr == (in_addr_t)-1) {
		fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
		exit(EXIT_FAILURE);
	}

	sockaddr_in.sin_addr.s_addr = in_addr;
	sockaddr_in.sin_family = AF_INET;
	sockaddr_in.sin_port = htons(server_port);

	memset(request, 0, 4096);

	strcpy(request, (char*)workload);			/* SET_REQUEST LOAD */

	request_len = strlen(request);

	if (request_len >= MAX_REQUEST_LEN) {
		fprintf(stderr, "request length large: %d\n", request_len);
		exit(EXIT_FAILURE);
	}	

	/* Actually connect - this needs to match accept call. */
	if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	/* Send the request. */					
	nbytes_total = 0;
	while (nbytes_total < request_len) {
		nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
		if (nbytes_last == -1) {
		    perror("write");
		    exit(EXIT_FAILURE);
		}
		nbytes_total += nbytes_last;
	}

	printf("avem len: %d\n", request_len);

	if(request_len > 5)
	{
		char filename[100];

		strcpy(filename, "Content/");
		strcat(filename, request);

		int file_fd = open(filename, O_WRONLY | O_CREAT, 0777);

		while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) 
		{
			write(file_fd, buffer, nbytes_total);
		}

		close(file_fd);	
	}
	else
	{
		while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) 
		{
			printf("got bytes: %d\n", nbytes_total);
			write(STDOUT_FILENO, buffer, nbytes_total);
		}
	}
		
  	close(socket_file_descriptor);
}

int main(int argc, char** argv) 
{
    int i = 0;
    int socket_desc , client_sock;
    char req[] = "0";

    char *cpu_workloads[] = {"1000", "1000", "1000", "1500", "1500", "1500", "1500", "1500"};

    char *io_workloads[] = {"test_file1.html", "test_file2.html", "test_file3.html", "test_file4.html", "test_file5.html"};

    /*char *io_workloads[101];

    char base_file_name[] = "test_file";

    char number[12];

    for(i = 1; i <= 100; i++)
    {
	io_workloads[i] = (char*)malloc(50 * sizeof(char));
	strcpy(io_workloads[i], base_file_name);
	sprintf(number, "%d", i);
	strcat(io_workloads[i], number);
	strcat(io_workloads[i], ".html");
    }*/

    pthread_t thread_id;

    for(i = 0; i < 2; i++)
    {
	    if( pthread_create( &thread_id , NULL ,  connection_handler , (void*)(io_workloads[i])) < 0)
	    {
		  perror("could not create thread");
		  return 1;
	    }
    }

    printf("Client generator: awaiting termination\n");
    scanf("%d", &i);

    return 0;
}

