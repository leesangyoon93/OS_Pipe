#include "s201221092_client.c"

#define print( str ) printf( "%s\n", str )
#define MAXLINE 128
#define STDOUT_FILENO 1
#define true 1
#define false 0

void server(int, int);
_Bool ServerLogin(int, int, char[MAXLINE], char[MAXLINE]);
void ServerStudentControl(int, int, char info1[MAXLINE], char info2[MAXLINE], char info3[MAXLINE], char id[MAXLINE], char password[MAXLINE]);
void dataParsing(char data[MAXLINE], char info1[MAXLINE], char info2[MAXLINE], char info3[MAXLINE]);
char* timeToString(struct tm *t);
_Bool timeCompare(char cur[20], char last[20]);

int main(int argc, char *argv[]) {
	int pipe1[2], pipe2[2];
	pid_t childpid;

	pipe(pipe1);
	pipe(pipe2);

	if((childpid=fork())==0) /* child */
	{
		close(pipe1[0]);        // pipe[0] read end of the pipe
        close(pipe2[1]);        // pipe[1] write end of the pipe

		server(pipe2[0], pipe1[1]); 
		exit(0);
	}
/* parent */
	close(pipe1[1]);
    close(pipe2[0]);

    client(pipe1[0], pipe2[1]);
	waitpid(childpid, NULL, 0); /* wait for child to terminate */
	exit(0);
}

void server(int readfd, int writefd) {
	char data[MAXLINE];
	char info1[MAXLINE];
	char info2[MAXLINE];
	char info3[MAXLINE]; 
	
	char idSession[MAXLINE];
	char passwordSession[MAXLINE];

	_Bool isLogin = false;

	short int tryCount = 0;

	while(true) {
		if((read(readfd, data, MAXLINE)) > 0) {
			dataParsing(data, info1, info2, info3);
			// data parsing and assign variable

			if(!isLogin) {
				if(tryCount == 3) {
					print("Server Message -> Account is locked... Server Waiting...");
					wait(NULL);
				}

				if(strcmp(info1, "LOGIN") == 0 || strcmp(info1, "login") == 0) {
					// call ServerLogin function
					isLogin = ServerLogin(readfd, writefd, info2, info3);
					strcpy(idSession, info2);
					strcpy(passwordSession, info3);
					tryCount++;
				}
				else {
					write(writefd, "INVALIDATE_REQUEST", MAXLINE);
				}
			}
			else {
				// call ServerStudentControl function
				ServerStudentControl(readfd, writefd, info1, info2, info3, idSession, passwordSession);
			}
		}
	}
}

void ServerStudentControl(int readfd, int writefd, char info1[MAXLINE], char info2[MAXLINE], char info3[MAXLINE], char id[MAXLINE], char password[MAXLINE]) {
	char serverID[MAXLINE];
	char serverPassword[MAXLINE];
	char lastAccess[20];

	char response[MAXLINE];
 	char studentID[128];
 	char studentName[128];
 	_Bool isStudent = false;

 	FILE *file, *file2, *tmp;

	struct tm *t;
  	time_t timer;
  	timer = time(NULL);
 	t = localtime(&timer);

	if((file = fopen("students.txt", "r+")) == NULL) { // file open
		file = fopen("students.txt", "w+");
	}

	if(strcmp(info1, "r") == 0) { // r command
		while(!feof(file)) {
			// search matching student
			fscanf(file, "%s %s\n", studentID, studentName);
			if(strcmp(studentID, info2) == 0) {
				isStudent = true;
				break;
			}
		}
		if(isStudent) {
			// student exist
			fclose(file);
			strcpy(response, "READ_SUCCESS ");
			strcat(response, info2);
			strcat(response, " ");
			strcat(response, studentName);
			write(writefd, response, MAXLINE);
		}
		else {
			// student not exist
			fclose(file);
			strcpy(response, "NOT_FOUND");
			write(writefd, response, MAXLINE);
		}
	}
	else if(strcmp(info1, "w") == 0) { 

		while(!feof(file)) {
			// search matching student
			fscanf(file, "%s %s\n", studentID, studentName);
			if(strcmp(studentID, info2) == 0) {
				isStudent = true;
				break;
			}
		}
		if(isStudent) {
			// student exist
			tmp = fopen("tmp.txt", "w");
			file = fopen("students.txt", "r");
			while(!feof(file)) {
				// update student info
				fscanf(file, "%s %s\n", studentID, studentName);
				if(strcmp(studentID, info2) == 0) {
					fprintf(tmp, "%s %s\n", info2, info3);
				}
				else {
					fprintf(tmp, "%s %s\n", studentID, studentName);
				}
			}
			fclose(file);
			fclose(tmp);
			rename("tmp.txt", "students.txt");

			strcpy(response, "UPDATE_SUCCESS ");
			strcat(response, info2);
			strcat(response, " ");
			strcat(response, info3);
			write(writefd, response, MAXLINE);
		}
		else {
			// student not exist
			// write student info
			fprintf(file, "%s %s\n", info2, info3);
			fclose(file);
			strcpy(response, "WRITE_SUCCESS ");
			strcat(response, info2);
			strcat(response, " ");
			strcat(response, info3);
			write(writefd, response, MAXLINE);
		}
	}
	else if(strcmp(info1, "logout") == 0) { 
		// logout
		file2 = fopen("login.txt", "r+");

		if(file2 == NULL) { 
			print("\nServer Message -> Login file open error!");
			return;
		}

		while(!feof(file2)) {
			// last access time updaate
			fscanf(file2,"%s %s %s\n", serverID, serverPassword, lastAccess);
			if(strcmp(serverID, id) == 0 && strcmp(serverPassword, password) == 0) {
				fseek(file2, -20L, SEEK_CUR); 
				fprintf(file2, "%s", timeToString(t));
				break;
			}
		}
		fclose(file);
		fclose(file2);
		print("\nServer Message -> User Logout... Server Waiting...");
		write(writefd, "LOGOUT", MAXLINE);
		wait(NULL); 
	}
	else {
		write(writefd, "INPUT_ERROR", MAXLINE);
	}
}

_Bool ServerLogin(int readfd, int writefd, char loginID[MAXLINE], char loginPassword[MAXLINE]) {
	
	FILE *file = fopen("login.txt", "r+");  // file open

	char serverID[MAXLINE]; 
	char serverPassword[MAXLINE]; 
	char lastAccess[20]; 

	_Bool loginSuccess = false;

	struct tm *t;
  	time_t timer;
  	timer = time(NULL);
 	t = localtime(&timer);
 

	if(file == NULL) { 
		print("\nServer Message -> Login file open error!");
		return false;
	}

	while(!feof(file)) {  
		// search user
		fscanf(file,"%s %s %s\n", serverID, serverPassword, lastAccess);
		if(strcmp(serverID, loginID) == 0 && strcmp(serverPassword, loginPassword) == 0 && timeCompare(lastAccess, timeToString(t))) { 
			// correct user info
			fseek(file, -20L, SEEK_CUR); 
			fprintf(file, "%s", timeToString(t)); 
			loginSuccess = true; 
			break;
		}
		else {
			// incorrect user info
			loginSuccess = false;
		}
	}
	fclose(file);
	if(loginSuccess) {
		write(writefd, "SUCCESS", MAXLINE); 
		return true;
	}
	else {
		write(writefd, "FAIL", MAXLINE); 
		return false;
	}
	
}

void dataParsing(char data[], char info1[], char info2[], char info3[]) { 

	short int token = 0;
	char *ptr;

	ptr = strtok(data, " ");
	strcpy(info1, ptr);
	while(ptr = strtok(NULL, " ")) {
		switch(token) {
			case 0:
				strcpy(info2, ptr);
				break;
			case 1:
				strcpy(info3, ptr);
				break;
			default:
				break;
		}
		token++;
	}
}

char* timeToString(struct tm *t) {
	// convert time structure to string
  	static char s[20];
  	sprintf(s, "%04d-%02d-%02d-%02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  	return s;
}

_Bool timeCompare(char cur[20], char last[20]) {
	// compare two time string
	int year, month, day, hour, min, sec;
	int year2, month2, day2, hour2, min2, sec2;
	sscanf(cur, "%04d-%02d-%02d-%02d:%02d:%02d", &year, &month, &day, &hour, &min, &sec);
	sscanf(last, "%04d-%02d-%02d-%02d:%02d:%02d", &year2, &month2, &day2, &hour2, &min2, &sec2);
	long currentAccessTimeSeq = ((((year*12L+month)*31L+day)*24L+hour)*60L+min)*60L+sec;
	long lastAccessTimeSeq = ((((year2*12L+month2)*31L+day2)*24L+hour2)*60L+min2)*60L+sec2;

	if(currentAccessTimeSeq <= lastAccessTimeSeq) { 
		return true;
	}
	else { 
		return false;
	}
}