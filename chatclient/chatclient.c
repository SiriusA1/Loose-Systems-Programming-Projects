/*******************************************************************************
 * Name        : chatclient.c
 * Author      : Conor McGullam
 * Date        : 5/7/2021
 * Description : Basic Chat Client
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int main(int argc, char *argv[]) {
	if (argc < 3) {
        fprintf(stderr, "Usage: %s <server IP> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
	
	struct sockaddr_in server_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	memset(&server_addr, 0, addrlen);
	int ip_conversion = inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    if (ip_conversion == 0) {
        fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    } else if (ip_conversion < 0) {
        fprintf(stderr, "Error: Failed to convert IP address. %s.\n",
                strerror(errno));
        return EXIT_FAILURE;
    }
	
	int port;
	if(parse_int(argv[2], &port, "port number") == false) {
		return EXIT_FAILURE;
	}else if(port < 1024 || port > 65535) {
		fprintf(stderr, "Error: Port must be in range [1024, 65535].\n");
		return EXIT_FAILURE;
	}
	server_addr.sin_family = AF_INET;   // Internet address family
    server_addr.sin_port = htons(port);
	
	int not_ready = 1;
	while(not_ready == 1) {
		printf("Enter your username: ");
		fgets(username, MAX_NAME_LEN+1, stdin);
		if(strlen(username) > 1 && username[strlen(username)-1] != '\n') {
			int ch;
            while(((ch = getchar()) != EOF) && (ch != '\n')) {
			}
			printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
		} else if(strlen(username) > 1) {
			not_ready = 0;
		}
	}
	username[strlen(username)-1] = '\0';
	printf("Hello, %s. Let's try to connect to the server.\n", username);
	
	int retval = EXIT_SUCCESS;
	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }
	
	int bytes_recvd;
	if(connect(client_socket, (struct sockaddr *)&server_addr, addrlen) == -1) {
		fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
		retval = EXIT_FAILURE;
        goto EXIT;
	}
	if ((bytes_recvd = recv(client_socket, inbuf, BUFLEN, 0)) == -1) {
		fprintf(stderr, "Error: Failed to receive message from server. %s.\n", 
		        strerror(errno));
		retval = EXIT_FAILURE;
        goto EXIT;
	} else if(bytes_recvd == 0) {
		fprintf(stderr, "All connections are busy. Try again later.\n");
		retval = EXIT_FAILURE;
        goto EXIT;
	}
	inbuf[bytes_recvd] = '\0';
	printf("\n%s\n\n", inbuf);
	
	if (send(client_socket, username, strlen(username), 0) < 0) {
        fprintf(stderr, "Error: Failed to send username to server. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }
	
	fd_set set;
	while(1) {
		printf("[%s]: ", username);
		fflush(stdout); 
		FD_ZERO(&set);
		FD_SET(STDIN_FILENO, &set);
		FD_SET(client_socket, &set);
		select(client_socket+1, &set, NULL, NULL, NULL);
		if (FD_ISSET(STDIN_FILENO, &set)) {
			if(get_string(outbuf, MAX_MSG_LEN) == TOO_LONG) {
				printf("Sorry, limit your message to %d characters.\n", MAX_MSG_LEN);
			} else {
				send(client_socket, outbuf, strlen(outbuf), 0);
			}
			if(strcmp(outbuf, "bye") == 0) {
				printf("Goodbye.\n");
				break;
			}
		}
		if(FD_ISSET(client_socket, &set)) {
			if ((bytes_recvd = recv(client_socket, inbuf, BUFLEN, 0)) == -1) {
				fprintf(stderr, "Warning: Failed to receive incoming message. %s.\n", 
		        strerror(errno));
			} else if(bytes_recvd == 0) {
				fprintf(stderr, "\nConnection to server has been lost.\n");
				retval = EXIT_FAILURE;
				goto EXIT;
			}
			inbuf[bytes_recvd] = '\0';
			if(strcmp(inbuf, "bye") == 0) {
				printf("\nServer initiated shutdown.\n");
				break;
			}
			printf("\n%s\n", inbuf);
			fflush(stdout);
		}
	}
	
EXIT:
    if (fcntl(client_socket, F_GETFD) >= 0) {
        close(client_socket);
    }
    return retval;
}