#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include<pthread.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>


struct acount* list;
int PORT = 11111;
pthread_mutex_t lock;

typedef struct account{
  char* name;
  struct account* next;
  double balance;
  //1 = in sesh 0 = na
  int inSesh;
} account;

typedef struct account account_t;

struct account_list {
	int num_accounts;
	account_t *first;
	account_t *last;
};
typedef struct account_list account_list_t;
account_list_t accounts;

struct connection {
	pthread_t *thread;
	pthread_cond_t *cond;
	pthread_mutex_t *mutex;
	int fd;
	int stopped;
	int suspend;
	struct connection *next;
};
typedef struct connection conn_t;

struct connections {
	conn_t *first;
	conn_t *last;
};
typedef struct connections connections_t;
connections_t connects;

typedef struct clientArgs{
  int fd;
}clientArgs;
