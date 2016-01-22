/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, clilen, pid;
     char buffer[256];char buffer1[512];
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     /* create socket */

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     /* fill in port number to listen on. IP address can be anything (INADDR_ANY) */

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     /* bind socket to this port number on this machine */

     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     /* listen for incoming connection requests */

     listen(sockfd,5);
     clilen = sizeof(cli_addr);

     /* accept a new request, create a newsockfd */
     while(1)
     {
        printf("new connection\n");

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");

        pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        // printf("%d\n",pid );

        /* new connection */
        if (pid == 0) 
        {
             close(sockfd);

            /* read message from client */

            // char buffer1[512];
            char file[20];
            bzero(buffer,256);bzero(file,20);
            n = read(newsockfd,buffer,255);
            if (n < 0) error("ERROR reading from socket");

            strncpy(file, buffer+4, strlen(buffer)-4);
            // printf("%s, size=%d\n",(file),n);

            FILE *fp=fopen(file,"r");
            fseek(fp,0,SEEK_END);
            int i=0;
            unsigned int size = ftell(fp);
            fseek(fp,0,SEEK_SET);
           
            // printf("before for\n");
            while(i<size){
                int numread=fread(buffer1,1,512,fp);
                // printf("some part is done %d\n",i);
                i+=write(newsockfd,buffer1,numread);
                bzero(buffer1,512);
            }

            // printf("end of files\n");
            // printf("size = %d\n",i );
            close(newsockfd);
            exit(0);
        }

        else 
        {
            close(newsockfd);
            int wp =1;
            /* reap the dead processes */
            printf("reaping start\n");
            while(wp > 0){
                wp = waitpid(-1, NULL, WNOHANG);
                printf("waitpid %d\n",wp );
            }
            printf("reaping end\n");
            
        }
    }
    return 0; 
}
