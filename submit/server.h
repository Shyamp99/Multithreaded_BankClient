#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include<pthread.h>
#include<unistd.h>

struct acount* list;
int PORT= 11111;
pthread_mutex_t lock;
struct account* current = NULL;

typedef struct account{
  char* name;
  struct account* next;
  double balance;
  //1 = in sesh 0 = na
  int inSesh;
} account;

typedef struct clientArgs{
  int fd;

}clientArgs;
