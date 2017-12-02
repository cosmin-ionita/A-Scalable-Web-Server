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

char *io_workloads[] = {

"test_file1.html",
"test_file2.html",
"test_file3.html",
"test_file4.html",
"test_file5.html",
"test_file6.html",
"test_file7.html",
"test_file8.html",
"test_file9.html",
"test_file10.html",
"test_file11.html",
"test_file12.html",
"test_file13.html",
"test_file14.html",
"test_file15.html",
"test_file16.html",
"test_file17.html",
"test_file18.html",
"test_file19.html",
"test_file20.html",
"test_file21.html",
"test_file22.html",
"test_file23.html",
"test_file24.html",
"test_file25.html",
"test_file26.html",
"test_file27.html",
"test_file28.html",
"test_file29.html",
"test_file30.html",
"test_file31.html",
"test_file32.html",
"test_file33.html",
"test_file34.html",
"test_file35.html",
"test_file36.html",
"test_file37.html",
"test_file38.html",
"test_file39.html",
"test_file40.html",
"test_file41.html",
"test_file42.html",
"test_file43.html",
"test_file44.html",
"test_file45.html",
"test_file46.html",
"test_file47.html",
"test_file48.html",
"test_file49.html",
"test_file50.html",
"test_file51.html",
"test_file52.html",
"test_file53.html",
"test_file54.html",
"test_file55.html",
"test_file56.html",
"test_file57.html",
"test_file58.html",
"test_file59.html",
"test_file60.html",
"test_file61.html",
"test_file62.html",
"test_file63.html",
"test_file64.html",
"test_file65.html",
"test_file66.html",
"test_file67.html",
"test_file68.html",
"test_file69.html",
"test_file70.html",
"test_file71.html",
"test_file72.html",
"test_file73.html",
"test_file74.html",
"test_file75.html",
"test_file76.html",
"test_file77.html",
"test_file78.html",
"test_file79.html",
"test_file80.html",
"test_file81.html",
"test_file82.html",
"test_file83.html",
"test_file84.html",
"test_file85.html",
"test_file86.html",
"test_file87.html",
"test_file88.html",
"test_file89.html",
"test_file90.html",
"test_file91.html",
"test_file92.html",
"test_file93.html",
"test_file94.html",
"test_file95.html",
"test_file96.html",
"test_file97.html",
"test_file98.html",
"test_file99.html",
"test_file100.html",

};

int main(int argc, char** argv) 
{
    int i = 0;
    int socket_desc , client_sock;
    char req[] = "0";

    char *cpu_workloads[] = {"1000", "1000", "1000", "1000", "1500", "1500", "1500", "1500"};

    pthread_t thread_id;

    for(i = 0; i < 4; i++)
    {
	    if( pthread_create( &thread_id , NULL ,  connection_handler , (void*)(cpu_workloads[i])) < 0)
	    {
		  perror("could not create thread");
		  return 1;
	    }

	    sleep(3);
    }

    printf("Client generator: awaiting termination\n");
    scanf("%d", &i);

    return 0;
}

