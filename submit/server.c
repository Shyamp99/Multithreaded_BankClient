#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <string.h>
#include "server.h"
#include <netinet/in.h>
#include <pthread.h>

void* startClient(void*);

int main(int argc, char** argv){
  int socketfd, newSocket;// val;
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  pthread_t sClient;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons( PORT );

  int addrlen = sizeof(addr);
  if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr))< 0){
    perror("bind failed try again");
    exit(EXIT_FAILURE);
  }
  if(listen(socketfd, 100)){
    perror("listen failed");
    exit(EXIT_FAILURE);
  }
  while(1){
    newSocket = accept(socketfd, (struct sockaddr *)&addr, (unsigned int *)&addrlen);
    printf("connection made?\n");
	//CHECK TO SEE IF YOU NEED TO EXUT WHEN newSocket == -1
    if(newSocket < 0){
      perror("accept failed");
    }
    else{
      //need to add the args struct
      pthread_create(&sClient, NULL, startClient, (void *) &newSocket);
      //pthread_join
    }
  }
  return 0;
}

struct account* findNode(char* name, struct account* curr){
  struct account* temp = curr;
  while(temp != NULL){
    if(strcmp(name, temp->name)==0)
      return curr;
    temp = temp->next;
  }
  return NULL;
}

struct account* addToList(struct account* new){
  pthread_mutex_lock(&lock);
  new->next = current;
  pthread_mutex_unlock(&lock);
  return new;
}

//DEPRECIATED METHOD, NOT USED
char* printData(struct account* current, char* retval){
  sprintf("Account Name: %s\tCurrent Balance: %f",current->name,current->balance);
  
 // printf("account name:%s\t current balance:%lf", current->name, current->balance);
  if(current->inSesh == 1)
    strcat(retval,"\tIN SERVICE");
//  printf("\n");
  strcat(retval,"\n");
	printf("%s\n",retval);
  return retval;
}


//client method
void* startClient(void* _args ){
  struct account* tcurrent = NULL;
  struct account* acc = (struct account*) malloc(sizeof(struct account));
  acc->balance = 0;
  char* temp;
  struct clientArgs* args =(struct clientArgs*) _args;
  int socketfd = args->fd;
  char buffer [1024]; // in format of word_<225 len char account> max# of chars = 262 so i take 300 to be safe
    while(1){
    read(socketfd, buffer, 1024);
    temp = strstr(buffer, " ");
    char* nani = (char*)malloc(strlen(temp));
   char* out = (char*)malloc(sizeof(char)*1024);
    memcpy(nani, temp+1, strlen(temp)-1);
    //char* temp = strchr(buffer," " )
    //need to check what command is there
    //then needs to check if is space
    if(strstr(buffer, "create ") != NULL){
      //create new struct and add to head
      if(buffer[0] != 'c'){
        fprintf(stderr, "invalid command please try another command\n");
         continue;
        }
      if(tcurrent != NULL){
        fprintf(stderr, "Cannot create a new account while another one is in service\n");
        continue;
      }
      //double value = atof(nani);
      if(findNode(nani, current) != NULL){
        fprintf(stderr, "Cannot reuse the same account name\n" );
        continue;
      }
      struct account* newAcc = (struct account*) malloc(sizeof(struct account));
      newAcc -> name = nani;
      newAcc -> balance = 0;
      newAcc -> inSesh = 1;
      newAcc -> next = NULL;
      current = addToList(newAcc);
	memset(out,0,1024);
	sprintf(out,"Account Made: %s \n",newAcc->name);
	write(socketfd,out,1024); 
      //do i need this?
      tcurrent = current;
    }
    else if(strstr(buffer, "serve ") != NULL){
      //search for stuct and set current
      if(buffer[0] != 's'){
          fprintf(stderr, "invalid command please try another command\n");
           continue;
      }
      //no accounts made
      if(current == NULL){
        fprintf(stderr, "no accounts have been made yet\n");
        continue;
      }
      //if one is being served atm
      if(tcurrent != NULL){
        fprintf(stderr, "cannot service more than 1 account at a time\n");
        continue;
      }
      //finding to see if is present
      tcurrent = findNode(nani, current);
      //not found
      if(tcurrent == NULL){
        fprintf(stderr, "Cannot serve an account that hasn't been made\n");
        continue;
      }
      //mutexing to change the specific node's sesh value
      pthread_mutex_lock(&lock);
      tcurrent->inSesh = 1;
      pthread_mutex_unlock(&lock);
	memset(out,0,1024);
	sprintf(out,"Serving: %s\n",tcurrent->name);
	write(socketfd,out,1024);
    }
    else if(strstr(buffer, "deposit ") != NULL){
      //no accounts made
      if(current == NULL){
        fprintf(stderr, "no accounts have been made yet\n");
        continue;
      }
      //none in service atm
      if(tcurrent == NULL){
        fprintf(stderr, "please service or create an account before calling this");
        continue;
      }
      //j checking the command
      if(buffer[0] != 'd'){
        fprintf(stderr, "invalid command please try another command\n");
        continue;
      }
      double value = atof(nani);
      if(nani[0] == '0'){}
      else if(value == 0.0){
        fprintf(stderr,"invalid amount\n");
        continue;
      }
	//send back amount deposited to client
      tcurrent->balance += value;
	memset(out,0,1024);
	sprintf(out,"Deposited: %f\n",value);
	write(socketfd,out,1024);
    }
    else if(strstr(buffer, "withdraw ") != NULL){
      //no accounts made
      if(current == NULL){
        fprintf(stderr, "no accounts have been made yet\n");
        continue;
      }
      //none in service atm
      if(tcurrent == NULL){
        fprintf(stderr, "please service or create an account before calling this");
        continue;
      }
      //j checking command
      if(buffer[0] != 'w'){
        fprintf(stderr, "invalid command please try another command\n");
        continue;
      }
      double value = atof(nani);
      //atof fails or they j put in 0.0
      if(nani[0] == '0'){}
      else if(value == 0.0){
        fprintf(stderr,"invalid amount\n");
        continue;
      }
      //condition franny said
      if(value>tcurrent->balance){
        fprintf(stderr,"invalid amount\n");
        continue;
      }
	//send back amoun withdrawn to client
      tcurrent->balance -= value;
	memset(out,0,1024);
	sprintf(out, "Withdrew: %f",value);
	write(socketfd,out,1024);
    }
    else if(strstr(buffer, "query") != NULL){
      //no accounts made
      if(current == NULL){
        fprintf(stderr, "no accounts have been made yet\n");
        continue;
      }
      //none in service atm
      if(tcurrent == NULL)
        fprintf(stderr, "please service or create an account before calling this");
      //j checking command
      if(buffer[0] != 'q'){
         fprintf(stderr, "invalid command please try another command\n");
        continue;
      }
	//output query back to client
	memset(out,0,1024);
	sprintf(out,"Account Name: %s \tCurrent Balance: %f ",tcurrent->name,tcurrent->balance);
	if(tcurrent->inSesh == 1){
	    strcat(out,"\t IN SERVICE ");
	}
  	strcat(out," \n");
	write(socketfd,out,1024);
    }
    else if(strstr(buffer, "end") != NULL){
      //if null then error
      //no accounts made
      if(current == NULL){
        fprintf(stderr, "no accounts have been made yet\n");
        continue;
      }
      if(buffer[0] != 'c'){
        fprintf(stderr, "invalid command please try another command\n");
         continue;
       }
      tcurrent->inSesh = 0;
      tcurrent = NULL;
    }
    else if(strstr(buffer, "quit") != NULL){
      if(buffer[0] != 'q'){
        fprintf(stderr, "invalid command please try another command\n");
        continue;
      }
      current->inSesh = 0;
      current = NULL;
      //need to figure out how to handle the quit
      //exit thread to stop the client but server should continue
      pthread_exit(NULL);
    }
    else{
       fprintf(stderr, "invalid command please try another command\n");
       continue;
	free(out);
    }

  }
}
