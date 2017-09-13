#include "dir.h"
#include "usage.h"
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
#include <ctype.h>

const char *CONNECTED_MESSAGE = "220 Please type USER followed by your username\r\n";
const char *TRY_AGAIN_MESSAGE = "Error please log in by typing USER followed by your username\r\n";
const char *INVALID_USER = "Invalid username, please try again\r\n";
const char *QUIT_INPUT = "221 Quitting ftp server\r\n";
//TODO FIND ERROR MESSAGES
const char *SUCCESS = "200 Command worked\r\n";
const char *CWD_ERROR = "550 Invalid CWD usage\r\n";
const char *CDUP_ERROR = "550 Invalid CDUP usage\r\n";
const char *TYPE_ERROR = "504 Invalid type, please input i for image and a for ASCII\r\n";
const char *MODE_ERROR = "504 Bad mode command useage\r\n";
const char *STRU_ERROR = "504 Bad stru command useage\r\n";
const char *PASV_ERROR = "500 PASV ERROR\r\n";
const char *CONNECTION_ERROR = "425 Cannot open data connectin\r\n";
const char *FILESTRUCT_ERROR = "Bad STRU command useage\r\n";
const char *DIR_CHANGE_MESSAGE = "250 Directory changed\r\n";
const char *SUCCESSFUL_LOGIN = "230 Login Successful\r\n";
const char *INVALID_COMMAND = "500 Invalid command\r\n";
const char *REQUEST_ERROR = "451 Action error\r\n";
const char *NLST_ERROR = "501 Invalid use\r\n";
const char *NLST_ACTION_ERROR = "450 Invalid use \r\n";
const char *SEND_START_LIST = "150 Here comes the listings\r\n";
const char *SEND_END_LIST = "226 Directory listings okay\r\n";
const char *SEND_END_RETR = "250 File successfully transfered\r\n";
const char *SEND_START_RETR = "150 Ready to retrieve file\r\n";

//Function to send entire file incase partial file is sent
//Code is taken from Beej 
int sendall(int s, FILE *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int generateSocket(int port){
    int newSocket;
    newSocket = socket(AF_INET, SOCK_STREAM, 0);

    //TODO set timeout
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    int returnSocket = bind(newSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if(returnSocket < 0){
        printf("Error with binding");
        return -1;
    }

    return newSocket;
}

// Parses according to strtok from http://www.cplusplus.com/reference/cstring/strtok/
int parseInput(char buff[], char *commands[]){
    char *pch;
    int count = 0;

    pch = strtok(buff, " \r\n");
    while (pch != 0 ) {
        commands[count++] = pch;
        pch = strtok(NULL," \r\n");
    }
    return count;
}

// Parses according to strtok from http://www.cplusplus.com/reference/cstring/strtok/
int parseIp(char buff[], char *commands[]){
    char *pch;
    int count = 0;

    pch = strtok(buff, ".");
    while (pch != 0 ) {
        commands[count++] = pch;
        pch = strtok(NULL,".");
    }

    return count;
}

// Quits the program
void quitCommand(int fd){
    if (send(fd, QUIT_INPUT, strlen(QUIT_INPUT), 0) == -1)
        perror("send");
    shutdown(fd,2);
    return;
}


void cwdCommand(int fd, char *commands){
    // Error handling, checks if input is correct
    if(strstr(commands, "./") != NULL) {
        if (send(fd, CWD_ERROR, strlen(CWD_ERROR), 0) == -1)
            perror("send");
        return;
    }

    // More error handling
    if(strstr(commands, "../") != NULL) {
        printf("all good");
        if (send(fd, CWD_ERROR, strlen(CWD_ERROR), 0) == -1)
            perror("send");
        return; 
    }

    // changes the directory using the chdir method
    if(chdir(commands) == -1) {
        if (send(fd, CWD_ERROR, strlen(CWD_ERROR), 0) == -1)
            perror("send");
        return;
    }

    if (send(fd, DIR_CHANGE_MESSAGE, strlen(DIR_CHANGE_MESSAGE), 0) == -1){
        if (send(fd, CWD_ERROR, strlen(CWD_ERROR), 0) == -1)
            perror("send");
        return;
    }
    
}

void cdupCommand(int fd, char *commands,char *initialDir){

    // Creates buffer to store path
    char currentDir[2048];

    if(getcwd(currentDir, 2048) == NULL) {
        if (send(fd, CDUP_ERROR, strlen(CDUP_ERROR), 0) == -1)
            perror("send");
    }

    if (strcmp(currentDir, initialDir) == 0) {
        if (send(fd, CDUP_ERROR, strlen(CDUP_ERROR), 0) == -1)
            perror("send");
    } else {
        if(chdir("..") == -1) {
            if (send(fd, CDUP_ERROR, strlen(CDUP_ERROR), 0) == -1)
            perror("send");
        }
    }

    //Testing to see current directory 
    printf("now we are in directory: ");
    char testDir[2048];
    getcwd(testDir, 2048);
    printf("%s\n", testDir);

}

void typeCommand(int *imgType, char *commands, int fd){
    // Changes to ascii if a
    if(strcasecmp(commands, "a") == 0){
        *imgType = 1;
        if (send(fd, SUCCESS, strlen(SUCCESS), 0) == -1)
            perror("send");
        return;
    }else if(strcasecmp(commands, "i") == 0){
        // Changes to i
        if (send(fd, SUCCESS, strlen(SUCCESS), 0) == -1)
            perror("send");
        *imgType = 0;
    }else{
        // Error handling
        if (send(fd, TYPE_ERROR, strlen(TYPE_ERROR), 0) == -1)
            perror("send");
        return; 
    }

    return;
}

void modeCommand(int *isStream, char *commands, int fd){
    // Changes to stream mode, only stream mode is accepted
    if(strcasecmp(commands, "s") == 0){
        *isStream = 1;
        if (send(fd, SUCCESS, strlen(SUCCESS), 0) == -1)
            perror("send");
    }else{
        // Error handling for mode command
        if (send(fd, MODE_ERROR, strlen(MODE_ERROR), 0) == -1)
            perror("send");
        return;
    }

}

void struCommand(int *fileStruct, char *commands, int fd){
    // Changes the stru
    if(strcasecmp(commands, "f") == 0){
        *fileStruct = 1;
        if (send(fd, SUCCESS, strlen(SUCCESS), 0) == -1)
            perror("send");
    }else{
        // Error handling for stru
        if (send(fd, STRU_ERROR, strlen(STRU_ERROR), 0) == -1)
            perror("send");
        return;
    }
}

void retrCommand(int fd,int dataSocket, char *path, int pasv){
    
    if(pasv != 1){
        if (send(fd, CONNECTION_ERROR, strlen(CONNECTION_ERROR), 0) == -1)
            perror("send");
        return;
    }
    //open the file to be sent 
    if (send(fd, SEND_START_RETR, strlen(SEND_START_LIST), 0) == -1){
        // TODO add error handling here
        return;
    }
    FILE * fp;
    fp = fopen (path, "r");
    if(fp == NULL) {
        //TODO: put correct send message 
        printf("File opened was null\n");
    }
    
    //Get size of the file 
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    //function to send the entirety of the file 
    int sendAllResult = sendall(dataSocket, fp, &size);

    
    if(sendAllResult == -1) {
        if (send(fd, REQUEST_ERROR, strlen(REQUEST_ERROR), 0) == -1)
            perror("send");
    }
    
    fclose(fp);
    
    if (send(fd, SEND_END_RETR, strlen(SEND_END_RETR), 0) == -1){
        // TODO add error handling here
        if (send(fd, REQUEST_ERROR, strlen(REQUEST_ERROR), 0) == -1)
            perror("send");
        return;
    }

    close(dataSocket);\
    

}

void pasvCommand(int fd, int *pasv, int *dataSock){
    //Get the host ip address 
    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        if (send(fd, REQUEST_ERROR, strlen(REQUEST_ERROR), 0) == -1)
            perror("send");
        return;
    }

    //Code snippets from Beej to get the ip address of the host
    struct hostent *he;
    if ((he = gethostbyname(hostname)) == NULL) {  // get the host info
        if (send(fd, REQUEST_ERROR, strlen(REQUEST_ERROR), 0) == -1)
            perror("send");
        return;
    }
    struct in_addr **addr_list;
    addr_list = (struct in_addr **)he->h_addr_list;
    char *ipaddress = inet_ntoa(*addr_list[0]);
    char *commands[4];
    int pieces = parseIp(ipaddress, commands);

    //TODO: Convert ip and port into pasv message to sent to client 
    char ipbuff[512];
    int port;
    int dataSocket, new_fd;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;


    for(port = 1025; port <= 65535; port++){
        dataSocket = generateSocket(port);
        if(dataSocket > 0)
            break;
    }

    int fbyte = port/256;
    int lbyte = port%256;
    sprintf(ipbuff, "227 Entering passive mode (%s,%s,%s,%s,%i,%i)\r\n",commands[0],commands[1],commands[2],commands[3],fbyte,lbyte);

    if (send(fd, ipbuff, strlen(ipbuff), 0) == -1){
        if (send(fd, REQUEST_ERROR, strlen(REQUEST_ERROR), 0) == -1)
            perror("send");        
        return;
    }

    if (listen(dataSocket, 1) == -1) {
        if (send(fd, CONNECTION_ERROR, strlen(CONNECTION_ERROR), 0) == -1)
            perror("send");
        perror("listen");
        exit(1);
    }

    sin_size = sizeof their_addr;
    *dataSock = accept(dataSocket, (struct sockaddr *)&their_addr, &sin_size);
    if (dataSocket == -1) {
        if (send(fd, CONNECTION_ERROR, strlen(CONNECTION_ERROR), 0) == -1)
            perror("send");
        perror("accept");
        return;
    }
    *pasv = 1;

}

void nlstCommand(int fd, int pasv, int dataSocket){
    //Get current directory name 
    if(pasv != 1){
        if (send(fd, CONNECTION_ERROR, strlen(CONNECTION_ERROR), 0) == -1)
            perror("send");
        return;
    }
    char currentDir[2048];
    if(getcwd(currentDir, 2048) == NULL) {
        if (send(fd, CWD_ERROR, strlen(CWD_ERROR), 0) == -1)
            perror("send");
    }
    if (send(fd, SEND_START_LIST, strlen(SEND_START_LIST), 0) == -1){
        // TODO add error handling here
        return;
    }
    int listFilesReturn = listFiles(dataSocket,currentDir);
    if (send(fd, SEND_END_LIST, strlen(SEND_END_LIST), 0) == -1){
        // TODO add error handling here
        return;
    }
    // listFiles returns -1 the named directory does not exist or you don't have permission to read it.
    if(listFilesReturn == -1) {
        if (send(fd, REQUEST_ERROR, strlen(REQUEST_ERROR), 0) == -1)
            perror("send");
        return;
    }
    //listFiles returns -2 insufficient resources to perform request
    if (listFilesReturn == -2) {
        if (send(fd, REQUEST_ERROR, strlen(REQUEST_ERROR), 0) == -1)
            perror("send");
        return;
    }

    close(dataSocket);
    return;
}

void handleRequests(int fd){
    char buff[100];
    int pieces;
    char initialDir[2048];
    int loggedIn = 0;
    int imgType = 0;
    int isStream = 0;
    int fileStruct = 0;
    int pasv = 0;
    int dataSocket;

    if(getcwd(initialDir, 2048) == NULL) {
        if (send(dataSocket, CWD_ERROR, strlen(CWD_ERROR), 0) == -1){
        // TODO add error handling here
        return;
        }
    }


    // Makes sure users logs in
    while(loggedIn == 0){
        int length = recv(fd, buff, sizeof(buff), 0);
        if(length == -1){
            if (send(dataSocket, CWD_ERROR, strlen(CWD_ERROR), 0) == -1){
                return;
            }
            return;
        }
        buff[length] = '\0';
        char *commands[4];
        pieces = parseInput(buff, commands);
        //TODO Add lowercase function
        if(strcmp(commands[0], "USER") == 0 && pieces == 2){
            if(strcmp(commands[1], "cs317") == 0) {
                if(send(fd, SUCCESSFUL_LOGIN, strlen(SUCCESSFUL_LOGIN), 0) == -1)
                    break;
                loggedIn = 1;
                break;
            } else {
                //print invalid user
                if (send(fd, INVALID_USER, strlen(INVALID_USER), 0) == -1){
                    return;
                }
            }
        }else{
            //print doesnt understand command type user 
            if (send(fd, TRY_AGAIN_MESSAGE, strlen(TRY_AGAIN_MESSAGE), 0) == -1){
                return;
            }
        }

    }

    // Loop for handling commands
    while(1){
        //cleanBuffer(commands);
        int length = recv(fd, buff, sizeof(buff), 0);
        if(length == -1){
            perror("ERROR: CAN'T READ FROM SOCKET");
            return;
        }
        buff[length] = '\0';
        char *commands[4];
        pieces = parseInput(buff, commands);


        if(strcmp(commands[0], "QUIT") == 0 && pieces == 1){
            quitCommand(fd);
            break;
        }else if(strcmp(commands[0], "CWD") == 0 && pieces == 2){
            printf("%s\n", initialDir);
            cwdCommand(fd, commands[1]);
            continue;
        }else if(strcmp(commands[0], "CDUP") == 0 && pieces == 1){
            cdupCommand(fd,commands[1], initialDir);
            continue;
        }else if(strcmp(commands[0], "TYPE") == 0 && pieces == 2){
            typeCommand(&imgType, commands[1], fd);
            continue;
        }else if(strcmp(commands[0], "MODE") == 0 && pieces == 2){
            modeCommand(&isStream, commands[1], fd);
            continue;
        }else if(strcmp(commands[0], "STRU") == 0 && pieces == 1){
            struCommand(&fileStruct, commands[1], fd);
            continue;
        }else if(strcmp(commands[0], "RETR") == 0 && pieces == 2){
            retrCommand(fd, dataSocket, commands[1], pasv);
            continue;
        }else if(strcmp(commands[0], "PASV") == 0 && pieces == 1){
            pasvCommand(fd, &pasv, &dataSocket);
            continue;
        }else if(strcmp(commands[0], "NLST") == 0 && pieces == 1){
            nlstCommand(fd,pasv, dataSocket);
            continue;
        }else{
            if (send(fd, INVALID_COMMAND, strlen(INVALID_COMMAND), 0) == -1){
                    return;
            }
        }

    }
}


// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.
int main(int argc, char **argv) {
    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    // Initiators
    int new_fd, newSocket;
    int i;
    int port = atoi(argv[1]);
    struct sockaddr_storage their_addr;
    socklen_t sin_size;

    // Generates a new socket
    newSocket = generateSocket(port);

    if (listen(newSocket, 1) == -1) {
        //TODO Add a proper error
        perror("listen");
        exit(1);
    }

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(newSocket, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        if (send(new_fd, CONNECTED_MESSAGE, strlen(CONNECTED_MESSAGE), 0) == -1){
            break;
        }

        // Handles the requests
        handleRequests(new_fd);
    }

    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor
    // returned for the ftp server's data connection

    //printf("Printed %d directory entries\n", listFiles(1, "."));
    // Closes sockets
    close(new_fd);
    close(newSocket);
    return 0;
}
