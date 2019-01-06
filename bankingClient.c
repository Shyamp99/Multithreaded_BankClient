#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
# include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <assert.h>
#define BUFF 1024


void* readServer(void*);
void* writeServer(void*);

pthread_t threads[2];
int end = 0;

typedef struct sockfd{
	int sockNum;
}sockfd;

int main(int argc, char *argv[])
{
	//invalid # of arguments
	if(argc != 3){

	  fprintf(stderr,"Must call program with 2 arguments, found %d\n",argc);

	  return -1;
	}

	int PORT = 11111;
	PORT = atoi(argv[2]);

	int sock = 0,conn =0;
	struct sockaddr_in serv_addr;
	struct hostent * server_ip;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	server_ip = gethostbyname(argv[1]);

	if(server_ip == NULL){
		fprintf(stderr,"Requested host not found. Errno set.\n");
		return -1;
	}

    if (sock < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	bcopy((char *)server_ip->h_addr,(char*)&serv_addr.sin_addr.s_addr,server_ip->h_length);


	conn = connect(sock,(struct sockaddr *)&serv_addr, sizeof(serv_addr));

   if (conn < 0)
     {
     	printf("\nConnection Failed \n");

	while(conn < 0){
	//	sleep(3);
		conn = connect(sock,(struct sockaddr *)&serv_addr, sizeof(serv_addr));
	}
     }
	else{
		printf("connection made\n");
	}



	int err = 1;

	err = pthread_create(&(threads[0]),NULL,&writeServer,(void *)&sock);

	if(err != 0){
		fprintf(stderr,"Can't make thread to write to server\n");
	}

	err = pthread_create(&(threads[1]),NULL,&readServer,(void *)&sock);

	if(err != 0){

		fprintf(stderr,"Can't make thread to read from server\n");
	}

	pthread_join(threads[0],NULL);
	pthread_join(threads[1],NULL);
	//int cont = 1;
	/*while(cont!=0){
		char buffer[BUFF];
		char response[BUFF] ;
		read(STDIN_FILENO, buffer, 1024);
		printf("Send: %s",buffer);
		int sent = write(sock, buffer,strlen(buffer));
		if(sent < 0){
			printf("Error: Message not sent.\n");
		}
		cont = strcmp(buffer,"quit");
		int rec = read(sock,response,BUFF);
		if(rec < 0){
			printf("Error: No server response\n");
		}
		printf("Response: %s\n",response);
		sleep(3);
	}*/

    	close(sock);

	return 0;
}


void* readServer(void* _arg){

	struct  sockfd* sockDes = (struct sockfd*) _arg;
	int sock = sockDes->sockNum;

	char buffer[BUFF];

	while(end == 0){
		memset(buffer,0,sizeof(buffer));
		int resp = read(sock,buffer,BUFF);

		if(resp < 0){
			printf("Error: No server response.\n");

		}
		else{
			printf("Response: %s\n",buffer);

		}

		if(strcmp(buffer,"quit")==0){
			end = 1;
		}
	}
return 0;
}

void* writeServer(void* _arg){

	struct  sockfd* sockDes = (struct sockfd*) _arg;
	int sock = sockDes->sockNum;


	char buffer[BUFF];

	while(end == 0){

		read(STDIN_FILENO,buffer,BUFF);
		printf("Send: %s\n",buffer);
		int send = write(sock,buffer,BUFF);
		memset(buffer,0,sizeof(buffer));
		if(send < 0){
			printf("Error: Message Not sent. Try again.\n");
		}
		sleep(2);
	}

return 0;

}
