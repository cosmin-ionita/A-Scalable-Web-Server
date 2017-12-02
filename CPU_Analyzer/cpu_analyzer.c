#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_BUF 1024

int parallel_clients;

void *read_named_pipe(void *);

void *read_named_pipe(void *dummy_param)
{
	int fd;

	char * myfifo = "/tmp/web_clients";
	//char buf[MAX_BUF];

    	fd = open(myfifo, O_RDONLY);

	int copy_val = 0;

	for(;;)
	{
	    int bytes = read(fd, &copy_val, sizeof(int));

	    if(bytes > 0)
		parallel_clients = copy_val;
		
	}
}


int main(void)
{
    long double a[4], b[4], loadavg;
    FILE *fp;
    char dump[50];


    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    
    
    parallel_clients = 0;

    pthread_t thread_id;
	
    if( pthread_create( &thread_id , NULL ,  read_named_pipe , NULL) < 0)
    {
     	perror("could not create thread");
    	return 1;
    }
    
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
        printf("CPU usage (percent) : %Lf -- Parallel clients: %d -- Time: %s\n",loadavg * 100, parallel_clients, buffer);
    }

    return(0);
}
