#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

int buffer_size = 256;
/**
* Enc_Client code
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

long findFileLength(char *filename){
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }
    //seek to the end of the file and then ask for the position
    fseek(fp, 0L, SEEK_END);
    long int res = ftell(fp);
    rewind(fp);
    fclose(fp);  
    return res-1;
}

int checkBadChar(char *plainTextFileName, char *keyFileName, long plainTextLen){
    //open plaintext and key file
    FILE* fpPlainTxt = fopen(plainTextFileName, "r");
    FILE* fpKey = fopen(keyFileName, "r");

    //go through the two files to check each character
    for (int i=0; i<plainTextLen; i++){
        char c_plainTxt = fgetc(fpPlainTxt);
        char c_key = fgetc(fpKey);

        if(!(isspace(c_plainTxt) || isupper(c_plainTxt))){
            return 1;
        }

        if(!(isspace(c_key) || isupper(c_key))){
            return 1;
        }        
    }
    rewind(fpPlainTxt);
    rewind(fpKey);
    fclose(fpPlainTxt);
    fclose(fpKey);
    return 0;
}

int main(int argc, char *argv[]) {
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char buffer[buffer_size];

  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr,"USAGE: enc_client $plaintext $key $port\n"); 
    exit(0); 
  } 

  // Find plane text length
  long plainTextLen = findFileLength(argv[1]);
  long keyLen = findFileLength(argv[2]);
  
  // Error out if input plain text and key have different length
  if (plainTextLen > keyLen){
    fprintf(stderr, "Error: key %s is too short\n", argv[2]); 
    exit(1);
  }

  // Check if any bad character
   char checkResult = checkBadChar(argv[1], argv[2], plainTextLen);
   //printf("%d", checkResult);
   if (checkResult != 0){
    fprintf(stderr, "enc_client error: input contains bad characters\n");
    exit(1);
   }

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

  // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    fprintf(stderr, "Error: could not contact enc_server on port %s \n", argv[3]); 
    exit(2);
  }

  // To make sure server knows it's from enc_client, send "ENC_CLIENT" to server
  charsWritten = send(socketFD, "ENC_CLIENT", 10 , 0);

  // Check if get approved by server
  memset(buffer, '\0', sizeof(buffer));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }

  // Exit if not approved by server
  if (strcmp(buffer, "Rejected by server") == 0){
    fprintf(stderr, "CLIENT: Rejected by server. Failed to connect\n");
    exit(2);
  }

  // -------------Start sending real data---------------------------------
   memset(buffer, '\0', sizeof(buffer));
  // Send file length to server
  char str_len[buffer_size];
  sprintf(str_len, "%d", plainTextLen);
  charsWritten = 0;
  charsWritten = send(socketFD, str_len, sizeof(str_len), 0);   

  FILE* fp_plainTxt = fopen(argv[1], "r");
  FILE* fp_key = fopen(argv[2], "r");

  int readNumPlainTxt = 0;
  int readNumKey = 0;
  memset(buffer, '\0', sizeof(buffer));

  //read and send plain text
  while(readNumPlainTxt < plainTextLen){
    memset(buffer, '\0', sizeof(buffer));
    size_t curr = fread(buffer, sizeof(*buffer), sizeof buffer / sizeof *buffer, fp_plainTxt);
    readNumPlainTxt += curr * sizeof *buffer;

    charsWritten = send(socketFD, buffer, strlen(buffer), 0);    
    if (charsWritten < 0){
      error("CLIENT_PlainTxt: ERROR writing to socket");
    }
    if (charsWritten < strlen(buffer)){
      printf("CLIENT_PlainTxt: WARNING: Not all data written to socket!\n");
    }
  }

  //read and send key
  while(readNumKey < plainTextLen){
    memset(buffer, '\0', sizeof(buffer));
    size_t curr= fread(buffer, sizeof(*buffer), sizeof buffer / sizeof *buffer, fp_key);
    readNumKey += curr * sizeof *buffer;

    //send
    charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
    if (charsWritten < 0){
      error("CLIENT_Key: ERROR writing to socket");
    }
    if (charsWritten < strlen(buffer)){
      printf("CLIENT_Key: WARNING: Not all data written to socket!\n");
    }
  }

  // -------------Start receiving --------------------------------
  charsRead = 0;

  while (charsRead < plainTextLen){
    memset(buffer, '\0', sizeof(buffer));
    int curr_read = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0){
    error("Enc-CLIENT: ERROR reading from socket");
    }
    // Count number of chars received
    charsRead += curr_read;

    printf("%s", buffer);
  }
  // Close the socket
  close(socketFD);
  printf("\n");

  return 0;
}