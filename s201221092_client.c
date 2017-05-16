#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
 
#define print( str ) printf( "%s\n", str )
 
#define MAXLINE 128
#define STDOUT_FILENO 1
#define true 1
#define false 0

void ClientLogin(int, int);
void ClientStudentControl(int, int);

void client(int readfd, int writefd) {
	char data[MAXLINE];
	short int tryCount = 0;
	_Bool isLogin = false;

	while(tryCount < 3) { 
		ClientLogin(readfd, writefd);
		// Call ClientLogin
		if((read(readfd, data, MAXLINE)) > 0) {
			if(strcmp(data, "SUCCESS") == 0) {
				// login success
				print("\nClient Message -> Login Success!");
				isLogin = true;
				break;
			}
			else if(strcmp(data, "INVALIDATE_REQUEST") == 0) {
				// invalidate request
				print("\nClient Message -> Please validate command!\n");
			}
			else { 
				// login fail
				tryCount++;
				printf("\nClient Message -> Login Fail! Your Chance remain %d\n\n", 3-tryCount);
				if(tryCount == 3) {
					print("\nClient Message -> Account is locked ... bye");
					return;
				}
			}
		}
	}
	
	while(isLogin) {
		ClientStudentControl(readfd, writefd);
		if((read(readfd, data, MAXLINE)) > 0) {
			if(strcmp(data, "LOGOUT") == 0) { // logout
				print("Client Message -> User Logout ... bye");
				return;
			}
			else if(strcmp(data, "INPUT_ERROR") == 0) { // input error
				print("\nClient Message -> Please validate command!");
			}
			else if(strstr(data, "WRITE_SUCCESS") != NULL) { // student file write success
				strtok(data, " ");
				printf("\nClient Message -> Student information write success! [ %s %s ]\n", strtok(NULL, " "), strtok(NULL, " "));
			}
			else if(strstr(data, "UPDATE_SUCCESS") != NULL) { // student file update success
				strtok(data, " ");
				printf("\nClient Message -> Student information update success! [ %s %s ]\n", strtok(NULL, " "), strtok(NULL, " "));
			}
			else if(strstr(data, "READ_SUCCESS") != NULL) { // student file read success
				strtok(data, " ");
				strtok(NULL, " ");
				printf("\nClient Message -> Matching Student Name is %s\n", strtok(NULL, " "));
			}
			else if(strcmp(data, "NOT_FOUND") == 0) { // student file read fail
				print("\nClient Message -> Student not found, Try again ...");
			}
		}
	}
	
}

void ClientStudentControl(int readfd, int writefd) {
	char command[MAXLINE];

	printf("\nClient Message -> Input Command\n1. r studentID\n2. w studentID studentName\n3. logout\n=> ");
	fgets(command, MAXLINE, stdin);
	command[strlen(command)-1] = '\0';
	// input command

	write(writefd, command, MAXLINE);
	// send data to server
	memset(command, 0, MAXLINE);
}

void ClientLogin(int readfd, int writefd) {
	char command[MAXLINE];
	char *event;
	
	printf("Client Message -> Request Login [ login ID XXXX ]\n=> ");
	fgets(command, MAXLINE, stdin);
	command[strlen(command)-1] = '\0';
	// input command

	write(writefd, command, MAXLINE);
	// send data to server
	memset(command, 0, MAXLINE);
}