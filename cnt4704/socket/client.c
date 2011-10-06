/*******************************************************************************
* File:       client.c
* Version:    0.7
* Purpose:    Connects to server.c & implements TRANSLATE, GET, STORE, & EXIT
* Author:     Michael Altfield <maltfield@knights.ucf.edu>
* Course:     CNT4707
* Assignment: 1
* Created:    2011-10-01
* Updated:    2011-10-05
* Notes:      Much of this code's base was obtained/modified using:
              http://beej.us/guide/bgnet/
*******************************************************************************/

/*******************************************************************************
                                   SETTINGS                                    
*******************************************************************************/

#define SERVER "localhost"
#define PORT "3331"
#define MAXDATASIZE 100 // max number of bytes that can be recieved at once
#define OK "200 OK"

/*******************************************************************************
                                   INCLUDES                                     
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

/*******************************************************************************
                           GLOBAL VARIABLE DEFINITIONS                      
*******************************************************************************/

// na

/*******************************************************************************
                                   FUNCTIONS                                    
*******************************************************************************/

// TODO: wrap common functions in header file

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
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
* Name:    getLines
* Purpose: Gets several lines of data from the user and store it to the given
*          location. (end of input is designated by a line only containing ".")
* Input:   s - the string to where we will store the collected input
* Output:  none (the input defines the output)
*******************************************************************************/
void getLines( char* s ){

	// DECLARE VARIABLES
	int status;
	int firstIteration = 1;

	// TODO: test overflow of input[]
	s[0] = '\0';
	do{

		// DECLARE VARIABLES
		char input[100];

		// don't print "c: " on the first iteration
		if( firstIteration )
			firstIteration = 0;
		else
			printf( "c: " );

		fgets( input, sizeof(input), stdin );

		// determine if the last line (period by itself) was entered
		status = strcmp( input, ".\n" );
		// was the last line of input entered?
		if( status != 0 && (strcmp( input, "\n" ) != 0) )
			// the user did not enter the last line of input
			// add this line to the response string to send to the server
			strcat( s, input );

	} while( status != 0 );

	// delete the last character of the string (a newline) by changing it to null
	s[strlen(s)-1] = '\0';

}

/*******************************************************************************
* Name:    printServerResp
* Purpose: Prints the server's response, prepending "s: " to each line
* Input:   s - the server's response
* Output:  none. the output is printed directly to Standard Out
*******************************************************************************/
void printServerResp( char* s ){

	printf( "s: ");

	// loop through every character of the supplied string
	for( int i =0; i<strlen(s); i++ ){

		// print each character
		printf( "%c", s[i] );

		// is this current character a newline with more data after it?
		if( s[i] == '\n' && i != (strlen(s)-1) )
			// this iteration's character is a newline AND there's more data

			// prepend this new line with "s: " to inform the user that this line
			// of data came from the server
			printf( "s: " );
	}

	// was the last line printed a newline?
	if( s[strlen(s)-1] != '\n' )
		// the last line printed was not a newline; print a newline
		printf( "\n" );

}

/*******************************************************************************
                                   MAIN BODY                                    
*******************************************************************************/

int main(){

	/***********************
	* VARIABLE DEFINITIONS *
	***********************/

	int status; // generic varaible for all function call's return status
	struct addrinfo hints, *servinfo, *p;
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	char s[INET6_ADDRSTRLEN]; // TODO to IPv4
	memset( &hints, 0, sizeof(hints) ); // make struct empty
	hints.ai_family = AF_INET;          // IPv4 only
	hints.ai_socktype = SOCK_STREAM;    // TCP

	int sentinel;     // used to exit loops

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

			perror( "client: socket" );

			// try the next iteration's addrinfo
			continue;

		}

		// If we made it this far, the socket has been successfully established.
		// Now try to establish a connection on the established socket.

		/************
		* connect() *
		************/

		status = connect( sockfd, servinfo->ai_addr, servinfo->ai_addrlen );

		// could we connect on the new socket?
		if( status  == -1 ){
			// we could not connect on the new socket, so close it & error.

			close( sockfd );
			perror( "client: connect" );

			// now try the next iteration's addrinfo
			continue;

		}

		// If we made it this far, the connection on the socket is working.

		// break free from this for loop; we don't need to try any other addrinfos
		break;

	} // end for each getaddrinfo() result

	// is p pointing to NULL?
	if( p == NULL ){
		// p is pointing to null, which means we iterated through all of the
		// getaddrinfo() results (trying to connect at each iteration), and made
		// it to the end of the results, still without a connection.

		fprintf( stderr, "client: failed to connect\n" );
		return 2;
	}

	inet_ntop(
	 p->ai_family, &( ( (struct sockaddr_in*)p->ai_addr)->sin_addr), s, sizeof(s)
	);
	printf( "client: connecting to %s\n", s );

	// get the number of bytes recieved
	numbytes = recv( sockfd, buf, MAXDATASIZE-1, 0);
	if( numbytes == -1 ){
		perror( "recv" );
		exit(1);
	}

	buf[numbytes] = '\0';

	/**********************
	* OUTPUT DATA TO USER *
	**********************/

	printf( "client: recieved '%s'\n", buf );

	/*******************
	* INTERACTIVE LOOP *
	*******************/

	// TODO: reset command value
	// TODO: handle unexpected input (type & buffer overflow)
	sentinel = 0;
	do{

		// DECLARE VARIABLES
		char command[10]; // store user inputted command
		char resp[100];   // store user inputted response

		// prompt the user for the command to send to the server
		printf( "c: " );
		scanf( "%s", (char *)&command );

		// send command to server
		sendH( sockfd, command );

		// get server's response & print to user
		recvH( sockfd, buf );
		printf( "s: %s\n", buf );

		/************
		* TRANSLATE *
		************/

		status = strcmp( command, "TRANSLATE" );
		if( status == 0 ){

			status = strcmp( buf, OK );
			// did the server respond with an OK?
			if( status != 0)
				// the server did not respond with an OK; abort TRANSLATE
				continue;

			// the server responded with an OK; get input from the user
			getLines( resp );

			// send the user's inputted data to the server
			sendH( sockfd, resp );

			recvH( sockfd, buf );   // get the TRANSLATE'd data
			printServerResp( buf ); // print the server's response

			continue;
		}

		/********
		* STORE *
		********/

		status = strcmp( command, "STORE" );
		if( status == 0 ){

			status = strcmp( buf, OK );
			// did the server respond with an OK?
			if( status != 0)
				// the server did not respond with an OK; abort TRANSLATE
				continue;

			// the server responded with an OK; get input from the user
			getLines( resp );

			// send the user's inputted data to the server
			printf( "about to send:|%s|\n", resp );
			sendH( sockfd, resp );

			recvH( sockfd, buf );   // get the server's response
			printServerResp( buf );

			continue;
		}

		/*******
		* EXIT *
		*******/

		status = strcmp( command, "EXIT" );
		if( status == 0 ){
			sendH( sockfd, command );
			recvH( sockfd, buf );
			sentinel = 1;
		}

	} while( sentinel == 0 );

	/***********
	* CLEANUP! *
	***********/

	freeaddrinfo(servinfo);
	close(sockfd);
	return 0;

}
