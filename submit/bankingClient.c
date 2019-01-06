#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <sys/socket.h>
# include <netinet/in.h> 
#include <string.h> 
#define PORT 11111 
#define BUFF 1024

int main(int argc, char const *argv[]) 
{ 
   
	int sock = 0,conn =0; 
	struct sockaddr_in serv_addr; 

	
	//char *hello = "Hello from client"; 
 
	sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   	memset(&serv_addr, '0', sizeof(serv_addr)); 
   
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
        serv_addr.sin_addr.s_addr = INADDR_ANY;

	
	conn = connect(sock,(struct sockaddr *)&serv_addr, sizeof(serv_addr));                                    
     if (conn < 0) 
     { 
     	printf("\nConnection Failed \n"); 
     	return -1; 
     }
	else{
		printf("connection made\n");
	}
	int cont = 1;
	while(cont!=0){
	
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
	}

    	close(sock);
     
	return 0; 
} 
