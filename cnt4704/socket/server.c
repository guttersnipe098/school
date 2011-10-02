/*******************************************************************************
* File:       server.c
* Version:    0.2
* Purpose:    Accepts connections & implements TRANSLATE, GET, STORE, & EXIT
* Author:     Michael Altfield <maltfield@knights.ucf.edu>
* Course:     CNT4707
* Assignment: 1
* Created:    2011-10-01
* Updated:    2011-10-02
*******************************************************************************/

/*******************************************************************************
                                   SETTINGS                                    
*******************************************************************************/

#define SERVER "localhost"
#define PORT "3331"
#define BACKLOG 10 // number of connections queue size
int yes = 1;

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
                              VARIABLE DEFINITIONS                              
*******************************************************************************/

int status; // generic varaible for all function results
struct addrinfo hints, *servinfo, *p;
struct sockaddr_storage their_addr; //connector's address info
socklen_t sin_size;
struct sigaction sa;
int sockfd, new_fd;
char s[INET6_ADDRSTRLEN];

/*******************************************************************************
                                   FUNCTIONS                                    
*******************************************************************************/

void sigchld_handler( int s ){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

/*******************************************************************************
                                   MAIN BODY                                    
*******************************************************************************/

int main(){

	memset( &hints, 0, sizeof(hints) ); // make struct empty
	hints.ai_family = AF_INET;          // IPv4 only
	hints.ai_socktype = SOCK_STREAM;    // TCP
	hints.ai_flags = AI_PASSIVE;        // use my ip

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
		// getaddrinfo() results (trying to connect at each iteration), and made
		// it to the end of the results, still without a connection.

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

			status = send( new_fd, "Hellow, world!", 13, 0 );
			if( status == -1)
				perror("send");

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
