#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <string.h>
#include "server.h"
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

int ALARM = 15;
void* commandHandler(void*);
account_t* processClientRequest(char *buffer, account_t* tcurrent,
		char *response, int *quit);
int testServer();
int startServer();
void addConnection(conn_t *conn);
conn_t* findConnection(int fd);
void deleteConn(conn_t *conn);
conn_t * removeConnection(int fd);
volatile int stop = 0;
void printDiagnostics();

void suspendMe(pthread_mutex_t *mutex, int *flag) { // tell the thread to suspend
	pthread_mutex_lock(mutex);
	*flag = 1;
	pthread_mutex_unlock(mutex);
}
void resumeMe(pthread_mutex_t *mutex, pthread_cond_t *cond, int *flag) { // tell the thread to resume
	pthread_mutex_lock(mutex);
	*flag = 0;
	pthread_cond_broadcast(cond);
	pthread_mutex_unlock(mutex);
}
void checkSuspend(pthread_mutex_t *mutex, pthread_cond_t *cond, int *flag) { // if suspended, suspend until resumed
	pthread_mutex_lock(mutex);
	while (*flag != 0)
		pthread_cond_wait(cond, mutex);
	pthread_mutex_unlock(mutex);
}

void handle_sigint(int sig) {
	// close all open session and exit
	printf("Stopping all sessions.\n");
	stop = 1;
	conn_t *con = connects.first;
	while (con != NULL) {
		if (!con->stopped) {
			shutdown(con->fd, SHUT_RD);
			pthread_join(*con->thread, NULL);
		}
		con = con->next;
	}
	printf("done handling signal\n");
	exit(0);
}

void handle_sigalarm(int sig) {
	printf("Processing alarm.\n");
	conn_t *con = connects.first;
	while (con != NULL) {
		suspendMe(con->mutex, &con->suspend);
		con = con->next;
	}
	printDiagnostics();
	con = connects.first;
	while (con != NULL) {
		resumeMe(con->mutex, con->cond, &con->suspend);
		con = con->next;
	}

	alarm(ALARM);
}

int main(int argc, char** argv) {
	if(argc >= 2)
		PORT = atoi(argv[1]);
	else{
		printf("defaulting to port 11111");
	}
	signal(SIGINT, handle_sigint);
	signal(SIGALRM, handle_sigalarm);
	accounts.first = NULL;
	accounts.last = NULL;
	accounts.num_accounts = 0;
	//return testServer();
	return startServer();
}

int testServer() {

	char buffer[1024], response[1024];
	account_t* current = NULL;
	int quit = 0;
	strcpy(buffer, "create acct1");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "serve acct1");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "deposit 100.0");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "deposit 200.0");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "withdraw 50.0");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "query");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "withdraw 250.0");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "end");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s\n", response);

	strcpy(buffer, "quit");
	current = processClientRequest(buffer, current, response, &quit);
	printf("Response: %s  quit=%d", response, quit);

	return 0;
}

int startServer() {
	int socketfd, newSocket; // val;
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	//pthread_t sClient;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	int addrlen = sizeof(addr);
	if (bind(socketfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind failed try again");
		exit(EXIT_FAILURE);
	}
	if (listen(socketfd, 100)) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	alarm(ALARM);
	while (!stop) {
		newSocket = accept(socketfd, (struct sockaddr *) &addr,
				(unsigned int *) &addrlen);
		//CHECK TO SEE IF YOU NEED TO EXUT WHEN newSocket == -1
		if (newSocket < 0) {
			printf("accept failed");
			break;
		} else {
			printf("connection made?\n");
			//need to add the args struct
			conn_t *con = (conn_t*) malloc(sizeof(conn_t));
			con->fd = newSocket;
			con->thread = (pthread_t *) malloc(sizeof(pthread_t));
			con->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(con->mutex, NULL);
			con->cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
			pthread_cond_init(con->cond, NULL);
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
			addConnection(con);
			pthread_create(con->thread, &attr, commandHandler,
					(void *) &newSocket);
		}
	}
	return 0;
}

void trimNewLine(char *buffer) {
	if (strlen(buffer) > 1) {
		while (buffer[strlen(buffer) - 1] == '\n'
				|| buffer[strlen(buffer) - 1] == '\r')
			buffer[strlen(buffer) - 1] = '\0';
	}
}
void deleteConn(conn_t *conn) {
	if (conn == NULL) {
		return;
	}
	if (conn->cond != NULL) {
		pthread_cond_destroy(conn->cond);
		free(conn->cond);
	}
	if (conn->mutex != NULL) {
		pthread_mutex_destroy(conn->mutex);
		free(conn->mutex);
	}
	if (conn->thread != NULL) {
		free(conn->thread);
	}
}
void addConnection(conn_t *conn) {
	if (stop) {
		// do not add...
		return;
	}

	pthread_mutex_lock(&lock);
	conn->next = NULL;
	conn->stopped = 0;
	if (connects.first == NULL) {
		connects.first = conn;
		connects.last = conn;
	} else {
		connects.last->next = conn;
		connects.last = conn;
	}
	pthread_mutex_unlock(&lock);

}

conn_t *findConnection(int fd) {
	pthread_mutex_lock(&lock);
	conn_t *next = connects.first;
	while (next != NULL) {
		if (next->fd == fd) {
			break;
		}
		next = next->next;
	}
	pthread_mutex_unlock(&lock);
	return next;

}
conn_t *removeConnection(int fd) {
	pthread_mutex_lock(&lock);
	conn_t *next = connects.first;
	conn_t *prev = NULL;
	int found = 0;
	while (next != NULL) {
		if (next->fd == fd) {
			found = 1;
			break;
		}
		prev = next;
		next = next->next;
	}
	if (!stop && found) {  // do not remove if stopping server
		if (prev == NULL) {
			connects.first = NULL;
		} else {
			prev->next = next->next;
		}
	}
	pthread_mutex_unlock(&lock);
	return next;

}

account_t* findNode(char* name) {
	account_t* node = accounts.first;
	int i;
	for (i = 0; i < accounts.num_accounts; i++) {
		if (strcmp(name, node->name) == 0)
			return node;
		else
			node = node->next;
	}
	return NULL;
}

account_t* addToList(account_t* new_acct) {
	pthread_mutex_lock(&lock);
	new_acct->next = NULL;
	if (accounts.first == NULL) {
		accounts.first = new_acct;
		accounts.last = new_acct;
	} else {
		accounts.last->next = new_acct;
		accounts.last = new_acct;
	}
	accounts.num_accounts++;
	pthread_mutex_unlock(&lock);
	return new_acct;
}

char* printData(account_t* current, char* retval) {
	retval[0] = '\0';
	strcat(retval, "Account Name: ");
	strcat(retval, current->name);
	strcat(retval, "\tCurrent Balance: ");
	char b[50];
	sprintf(b, "%.2f", current->balance);
	strcat(retval, b);
	// printf("account name:%s\t current balance:%lf", current->name, current->balance);
	if (current->inSesh == 1)
		strcat(retval, "\tIN SERVICE");
	strcat(retval, "\n");
	return retval;
}

void printDiagnostics() {
	pthread_mutex_lock(&lock);

	account_t *acct = accounts.first;
	char buf[1024];
	while (acct != NULL) {
		printf("%s", printData(acct, buf));
		acct = acct->next;
	}
	pthread_mutex_unlock(&lock);

}

void* commandHandler(void* _args) {
	account_t* tcurrent = NULL;
	struct clientArgs* args = (struct clientArgs*) _args;
	int socketfd = args->fd;
	char buffer[1024]; // in format of word_<225 len char account> max# of chars = 262 so i take 300 to be safe
	char response[1024];
	int quit = 0;
	conn_t *conn = findConnection(socketfd);

	while (quit == 0) {
		memset(buffer, 0, 1024);
		memset(response, 0, 1024);
		checkSuspend(conn->mutex, conn->cond, &conn->suspend);
		int numRec = read(socketfd, buffer, 1024);
		if (numRec > 0) {
			tcurrent = processClientRequest(buffer, tcurrent, response, &quit);
		} else {
			printf("error: %d\n", numRec);
			// interrupted... send abort to client and shutdown
			//printf("\n*************** Interrupted **************\n");
			strcpy(response, "aborting.\n");
			quit = 1;
		}
		if (strlen(response) > 0) {
			write(socketfd, response, 1024);
		}
		printf("[%d] %s --> %s\n", socketfd, buffer, response);
	}
	removeConnection(socketfd);
	// If shutting down, we already have stopped receive channel.   only stop transmit channel
	int how = stop==1?1:2;
	int status = shutdown(socketfd, how);
	if (status != 0) {
		fprintf(stderr, "error shutting down socket %d", socketfd);
	}
	//printf("\n*************** Exiting **************\n");
	pthread_exit(NULL);
	if (conn != NULL) {
		conn->stopped = 1;
	}
	if (!stop) {
		deleteConn(conn);
	}
	return NULL;
}

account_t* processClientRequest(char *buffer, account_t* tcurrent,
		char *response, int* quit) {
	account_t *acct = NULL;
	char *temp = strstr(buffer, " ");
	char *nani = NULL;
	if (temp != NULL) {
		// no parameter -- query, end
		nani = (char*) malloc(strlen(temp));
		memcpy(nani, temp + 1, strlen(temp) - 1);
		trimNewLine(nani);
	}
	response[0] = '\0';
	if (strstr(buffer, "create ") != NULL) {
		//create new struct and add to head
		if (nani == NULL) {
			sprintf(response, "invalid account\n");
			return tcurrent;
		}
		if (buffer[0] != 'c') {
			sprintf(response, "invalid command please try another command\n");
			return tcurrent;
		}
		if (tcurrent != NULL) {
			sprintf(response,
					"Cannot create a new account while another one is in service\n");
			return tcurrent;
		}
		//double value = atof(nani);
		acct = findNode(nani);
		if (acct != NULL) {
			sprintf(response, "Cannot reuse the same account name\n");
			return tcurrent;
		}
		account_t* newAcc = (account_t*) malloc(sizeof(account_t));
		newAcc->name = nani;
		newAcc->balance = 0;
		newAcc->inSesh = 0;
		newAcc->next = NULL;
		addToList(newAcc);
		strcpy(response, "Account Made: ");
		strcat(response, newAcc->name);
		strcat(response, "\n");
	} else if (strstr(buffer, "serve ") != NULL) {
		//search for stuct and set current
		if (nani == NULL) {
			sprintf(response, "invalid account\n");
			return tcurrent;
		}

		if (buffer[0] != 's') {
			sprintf(response, "invalid command please try another command\n");
			return tcurrent;
		}
		//if one is being served atm
		if (tcurrent != NULL) {
			sprintf(response, "cannot service more than 1 account at a time\n");
			return tcurrent;
		}
		acct = findNode(nani);
		if (acct == NULL) {
			sprintf(response, "%s account doesn't exist\n", nani);
			return tcurrent;
		}
		if(acct->inSesh == 1){
			sprintf(response, "Account is already in service please pick another acocunt");
		}
		//mutexing to change the specific node's sesh value
		pthread_mutex_lock(&lock);
		if (tcurrent != NULL) {  // Check if switching account..
			tcurrent->inSesh = 0;
		}
		tcurrent = acct;
		tcurrent->inSesh = 1;
		pthread_mutex_unlock(&lock);
		strcpy(response, "Serving: ");
		strcat(response, tcurrent->name);
		strcat(response, "\n");

	} else if (strstr(buffer, "deposit ") != NULL) {
		if (nani == NULL) {
			fprintf(stderr, "invalid deposit\n");
			return tcurrent;
		}

		//none in service atm
		if (tcurrent == NULL) {
			sprintf(response,
					"please service or create an account before calling this");
			return tcurrent;
		}
		//j checking the command
		if (buffer[0] != 'd') {
			sprintf(response, "invalid command please try another command\n");
			return tcurrent;
		}
		double value = atof(nani);
		if (nani[0] == '0') {
		} else if (value == 0.0) {
			sprintf(response, "invalid amount\n");
			return tcurrent;
		}
		tcurrent->balance += value;
		strcpy(response, "Deposited: ");
		strcat(response, nani);
		strcat(response, "\n");

	} else if (strstr(buffer, "withdraw ") != NULL) {
		//no accounts made
		if (nani == NULL) {
			sprintf(response, "invalid withdraw\n");
			return tcurrent;
		}

		//none in service atm
		if (tcurrent == NULL) {
			sprintf(response,
					"please service or create an account before calling this\n");
			return tcurrent;
		}
		//j checking command
		if (buffer[0] != 'w') {
			sprintf(response, "invalid command please try another command\n");
			return tcurrent;
		}
		double value = atof(nani);
		//atof fails or they j put in 0.0
		if (nani[0] == '0') {
		} else if (value == 0.0) {
			sprintf(response, "invalid amount\n");
			return tcurrent;
		}
		//condition franny said
		if (value > tcurrent->balance) {
			sprintf(response, "invalid amount\n");
			return tcurrent;
		}
		tcurrent->balance -= value;
		strcpy(response, "Withdrew: ");
		strcat(response, nani);
		strcat(response, "\n");

	} else if (strstr(buffer, "query") != NULL) {
		//no accounts made
		//none in service atm
		if (tcurrent == NULL) {
			sprintf(response,
					"please service or create an account before calling this\n");
			return tcurrent;
		}
		//j checking command
		if (buffer[0] != 'q') {
			sprintf(response, "invalid command please try another command\n");
			return tcurrent;
		}
		printData(tcurrent, response);

	} else if (strstr(buffer, "end") != NULL) {
		//if null then error
		//no accounts made
		if (buffer[0] != 'e') {
			sprintf(response, "invalid command please try another command\n");
			return tcurrent;
		}
		pthread_mutex_lock(&lock);
		tcurrent->inSesh = 0;
		pthread_mutex_unlock(&lock);
		tcurrent = NULL;
		strcpy(response, "ended session\n");
		memset(buffer, '\0', 1024);

	} else if (strstr(buffer, "quit") != NULL) {
		if (buffer[0] != 'q') {
			sprintf(response, "invalid command please try another command\n");
			return tcurrent;
		}
		if (tcurrent != NULL) {
			pthread_mutex_lock(&lock);
			tcurrent->inSesh = 0;
			pthread_mutex_unlock(&lock);
			tcurrent = NULL;
		}
		strcpy(response, "quitting session\n");
		*quit = 1;
		//need to figure out how to handle the quit
		//exit thread to stop the client but server should continue
		//pthread_exit(NULL);
	} else {
		sprintf(response, "invalid command please try another command\n");
	}
	return tcurrent;
}
