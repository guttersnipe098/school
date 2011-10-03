/*******************************************************************************
* File:       server.c
* Version:    0.4
* Purpose:    Accepts connections & implements TRANSLATE, GET, STORE, & EXIT
* Author:     Michael Altfield <maltfield@knights.ucf.edu>
* Course:     CNT4707
* Assignment: 1
* Created:    2011-10-01
* Updated:    2011-10-02
* Notes:      Much of this code's base was obtained/modified using:
              http://beej.us/guide/bgnet/
*******************************************************************************/

/*******************************************************************************
                                   SETTINGS                                    
*******************************************************************************/

#define SERVER "localhost"
#define PORT "3331"
#define BACKLOG 10 // number of connections queue size
int yes = 1;
#define MAXDATASIZE 100 // max number of bytes that can be recieved at once

/*******************************************************************************
                                   INCLUDES                                     
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/*******************************************************************************
                           GLOBAL VARIABLE DEFINITIONS                      
*******************************************************************************/

int status; // generic varaible for all function results
struct addrinfo hints, *servinfo, *p;
struct sockaddr_storage their_addr; //connector's address info
socklen_t sin_size;
struct sigaction sa;
int sockfd, new_fd;
char s[INET6_ADDRSTRLEN];
char store_buffer[] = "We ain't in Joe-Ja no mo!";

/*******************************************************************************
                                   FUNCTIONS                                    
*******************************************************************************/

void sigchld_handler( int s ){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

/*******************************************************************************
* Name:    sendH
* Purpose: Helper function to send string data to server
* Input:   sockfd - socket file descriptor
*          s      - string to send
* Output:  none (perror() & exit() program on fail)
*******************************************************************************/
void sendH( int sockfd, char* s ){

	// VARIABLE DEFINITIONS
	int status;

	status = send( sockfd, s, strlen(s), 0 );

	// did the send() succeed?
	if( status == -1){
		// there was an error sending; error & exit.
		perror("send");
		exit(1);
	}

}

/*******************************************************************************
* Name:    recvH
* Purpose: Helper function to recieve string data from server
* Input:   sockfd - socket file descriptor
*          buf    - buffer to store the string data recieved from the server
* Output:  none (perror() & exit() program on fail)
*******************************************************************************/
void recvH( int sockfd, char* buf ){

	// VARIABLE DEFINITIONS
	int numbytes;

	numbytes = recv( sockfd, buf, MAXDATASIZE-1, 0);

	// did the recv() succeed?
	if( numbytes == -1 ){
		perror( "recv" );
		exit(1);
	}

	buf[numbytes] = '\0';

}

/*******************************************************************************
                                   MAIN BODY                                    
*******************************************************************************/

int main(){

	/***********************
	* VARIABLE DEFINITIONS *
	***********************/

	memset( &hints, 0, sizeof(hints) ); // make struct empty
	hints.ai_family = AF_INET;          // IPv4 only
	hints.ai_socktype = SOCK_STREAM;    // TCP
	hints.ai_flags = AI_PASSIVE;        // use my ip

	int numbytes; // number of bytes returned by send() or recv()
	char buf[MAXDATASIZE];

	/****************
	* getaddrinfo() *
	****************/

	status = getaddrinfo(SERVER, PORT, &hints, &servinfo);

	// were there errors?
	if( status != 0 ){
		// there were errors; alert & exit.
		fprintf( stderr, "getaddrinfo error: %s\n", gai_strerror(status) );
		exit(1);
	}

	// loop through getaddrinfo()'s results = servinfo
	for( p = servinfo; p != NULL; p = p->ai_next ){
		// try to establish a connection with this iteration's addrinfo

		/***********
		* socket() *
		***********/

		sockfd = socket(
		 servinfo-> ai_family, servinfo->ai_socktype, servinfo->ai_protocol
		);

		// could we establish a socket?
		if( sockfd == -1 ){
			// socket creation failed

			perror( "server: socket" );

			// try the next iteration's addrinfo
			continue;

		}

		// fix "Address Already in Use" Error
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			perror("setsockopt");
			exit(1);
		}

		// If we made it this far, the socket has been successfully established.
		// Now try to bind on the established socket.

		/*********
		* bind() *
		*********/

		status = bind( sockfd, p->ai_addr, p->ai_addrlen );

		// could we bind() on the new socket?
		if( status == -1 ){
			// we could not bind() on the new socket, so close it & error.

			close(sockfd);
			perror( "server: bind" );

			// now try the next iteration's addrinfo
			continue;

		}

		// If we made it this far, the bind on the socket is working.

		// break free from this for loop; we don't need to try any other addrinfos
		break;

	} // end for each getaddrinfo() result

	// is p pointing to NULL?
	if( p == NULL ){
		// p is pointing to null, which means we iterated through all of the
		// getaddrinfo() results (trying to bind() at each iteration), and made
		// it to the end of the results, still without a working bind().

		fprintf( stderr, "server: failed to bind\n" );
		return 2;
	}

	/***********
	* listen() *
	***********/

	status = listen( sockfd, BACKLOG );

	// could we listen() on the connect()ion?
	if( status == -1 ){
		// we could not listen(); error & exit.

		perror( "listen" );
		exit(1);

	}

	// REAP ALL DEAD PROCESSES
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf( "server: waiting for connections...\n" );

	while(1) {  // main accept() loop

		sin_size = sizeof( their_addr );
		new_fd = accept( sockfd, (struct sockaddr *)&their_addr, &sin_size );

		if( new_fd == -1 ){
			perror("accept");
			continue;
		}

		inet_ntop(
		 their_addr.ss_family, &( ( (struct sockaddr_in*)&their_addr)->sin_addr ),
		 s, sizeof(s)
		);

		printf( "server: got connection from %s\n", s );

		if( !fork() ){ // this is the child process

			close(sockfd); // child doesn't need the listener

			numbytes = send( new_fd, "Server is ready...", 23, 0 );
			if( status == -1)
				perror("send");

			// RECIEVE MORE DATA
			do{ 

				// DEFINE VARIABLES
				char *resp; // string to build our response to the client

				// get the number of bytes recieved

				// if we don't print a newline here, the server will hang on the
				// following line. TODO: find out why!
				printf( "\n" );
				recvH( new_fd, buf );
				printf( "buffer:|%s|", buf );

				// TRANSLATE
				status = strcmp( buf, "TRANSLATE" );
				if( status == 0 ){
					sendH( new_fd, "200 OK" );
					// TODO

					continue;
				}

				// GET
				status = strcmp( buf, "GET" );
				if( status == 0 ){
					strcpy( resp, "200 OK\n" );
					strcat( resp, store_buffer );
					sendH( new_fd, resp );
					// TODO

					continue;
				}

				// STORE
				status = strcmp( buf, "STORE" );
				if( status == 0 ){
					// TODO
					numbytes = send( new_fd, "200 OK", 6, 0 );
					if( status == -1)
						perror("send");

					continue;
				}

				// EXIT
				status = strcmp( buf, "EXIT" );
				if( status == 0 ){
					// TODO
					numbytes = send( new_fd, "200 OK", 6, 0 );
					if( status == -1)
						perror("send");

					continue;
				}

				// if we made it this far, the command is not recognized.
				numbytes = send( new_fd, "400 Command not valid.", 23, 0 );
				if( status == -1)
					perror("send");

			} while( (strcmp(buf,"EXIT") != 0) );

			close(new_fd);
			exit(0);

		}

		close(new_fd);  // parent doesn't need this

	}

	/***********
	* CLEANUP! *
	***********/

	freeaddrinfo(servinfo);
	return 0;

}
