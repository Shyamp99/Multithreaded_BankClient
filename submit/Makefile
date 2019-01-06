CC = gcc

CFLAGS = -Wall -Werror -g -fsanitize=address -lm

all: bankingServer bankingClient

bankingServer: server.c server.h
	$(CC) $(CFLAGS) server.c -o bankingServer -pthread

bankingClient: bankingClient.c
	$(CC) $(CFLAGS) bankingClient.c -o bankingClient
clean:
	rm -rf bankingServer bankingClient
