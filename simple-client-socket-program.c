#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

struct thread_data{
   int portno,experiment_time,think_time,thread_id,requests;
   double response_time;
   char* mode;
   struct sockaddr_in serv_addr;
};

void error(char *msg)
{
    perror(msg);
    exit(0);
}

void *startConnection(void *thread_data)
{
    int sockfd, portno, n, experiment_time, think_time,thread_id;
    char* mode;
    struct sockaddr_in serv_addr;
    time_t t;
    srand((unsigned) time(&t));
    int fno; char buffer3[30]; char buffer1[512];
    // char* buffer;

    struct thread_data *my_data = (struct thread_data*) thread_data;
    portno = my_data->portno;
    mode = my_data->mode;
    experiment_time = my_data->experiment_time;
    think_time = my_data->think_time;
    serv_addr = my_data->serv_addr;
    thread_id = my_data->thread_id;

    printf("threadno = %d\n", thread_id);
    // printf("%d %s %d %d tid=%d\n",portno,mode,experiment_time,think_time,thread_id );

    struct timeval start,end,r1,r2;
    gettimeofday(&start,NULL);
    gettimeofday(&end,NULL);
    // printf("%d %d \n",end.tv_sec, start.tv_sec );
    while(end.tv_sec-start.tv_sec < experiment_time)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");

        /* connect to server */

        if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
            error("ERROR connecting");

        
        bzero(buffer3,30);bzero(buffer1,512);
        // printf("length =%d\n",strlen(buffer) );
        if(strcmp(mode, "fixed")==0)
        {
            fno=0;
            sprintf(buffer3,"get files/foo%d.txt\0",fno);
            // printf("%s\n",(buffer3) ); 
        }
        else
        {   
            // printf("came in else\n" );
            fno = rand()%1000;
            sprintf(buffer3,"get files/foo%d.txt\0",fno);
            // printf("%s\n",(buffer3) );
        } 

        gettimeofday(&r1,NULL);

        n = write(sockfd,buffer3,strlen(buffer3));
        printf("bytes written= %d\n",n );
        if (n < 0) 
           error("ERROR writing to socket");

        
        int bytes=0;n=1;
        // printf("before while\n");
        while(n){
            n=read(sockfd,buffer1,512);
            bytes+=n;            
        }

        gettimeofday(&r2,NULL);

        my_data->requests++;
        my_data->response_time+=1000*(r2.tv_sec-r1.tv_sec)+(r2.tv_usec-r1.tv_usec)/1000;
        printf("%f\n", (double)1000*(r2.tv_sec-r1.tv_sec)+(r2.tv_usec-r1.tv_usec)/1000);
        printf("thread %d,file %s recieved ,numbytes=%d\n",thread_id,buffer3,bytes);

        close(sockfd);
        
        sleep(think_time);
        gettimeofday(&end,NULL);
        // printf("%d %d \n",end.tv_sec, start.tv_sec );
    }

    return;
    
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n, num_threads, experiment_time, think_time,total_requests=0,avg_response_time=0.0;
    char* mode;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_attr_t attr;
    void *status;
    int rc;

    if (argc < 7) {
       fprintf(stderr,"usage %s hostname port num_threads experiment_time think_time mode\n", argv[0]);
       exit(0);
    }

    /* create socket, get sockfd handle */

    portno = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    experiment_time = atoi(argv[4]);
    think_time = atoi(argv[5]);
    mode = argv[6];
    printf("mode = %s\n", mode );
    pthread_t threads[num_threads];
    struct thread_data td[num_threads];
    // Initialize and set thread joinable
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    /* fill in server address in sockaddr_in datastructure */

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    int i;
    for( i=0; i < num_threads; i++ )
    {
        td[i].requests=0;
        td[i].response_time=0.0;
        td[i].portno = portno;
        td[i].experiment_time = experiment_time;
        td[i].think_time=think_time;
        td[i].mode = mode;
        td[i].serv_addr = serv_addr;
        td[i].thread_id = i;
    }

    
    for( i=0; i < num_threads; i++ )
    {
          printf("main() : creating thread, %d\n", i);
          rc = pthread_create(&threads[i], NULL, startConnection, (void *)&td[i] );
          if (rc)
          {
             printf("Error:unable to create thread, %d\n", rc);
             exit(-1);
          }
    }

    
   // free attribute and wait for the other threads
   pthread_attr_destroy(&attr);
   for( i=0; i < num_threads; i++ ){
      rc = pthread_join(threads[i], &status);
      total_requests+=td[i].requests;
      avg_response_time+=(double)td[i].response_time/td[i].requests;
      if (rc){
         printf("Error:unable to join, %d\n", rc);
         exit(-1);
      }
      // cout << "Main: completed thread id :" << i ;
      // printf("Exiting with status, %s\n", status); 
   }

   // cout << "Main: program exiting." << endl;
   printf("total requests = %d, throughput=%f, avg_response_time=%f", total_requests,(double)total_requests/(double)experiment_time,(double)avg_response_time/(10E3*num_threads));
   pthread_exit(NULL);

    return 0;
}
