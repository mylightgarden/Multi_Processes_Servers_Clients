#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

/**
* Enc_Server code
*/

int num_connections = 0;
int buffer_size = 256;

// Encrypt plain text to ciphertext
void encrypt(char plainText[], char key[], long len, char encryptedText[]){
  int i;
  for (i=0; i<len; i++){
    // conver chars to numbers
    int curr_plain = plainText[i] == ' '? 0 : plainText[i] - 64;
    int curr_key = key[i] == ' '? 0 : key[i] - 64;

    // conver numbers to chars
    int curr_cipher = (curr_plain + curr_key) % 27;
    encryptedText[i] = curr_cipher == 0 ? ' ' : curr_cipher+64;
  }
  encryptedText[i+1] = '\0';
  //printf("Enc_server -- This is encryptedText :%s\n", encryptedText);
}

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]){
  int connectionSocket, charsRead;
  char buffer[buffer_size];  // buffer for received messages
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  int max_len  = 100000;  
  char plainText[max_len], key[max_len]; // Space for incoming plain text, key and outgoing encrypted text
  char totalText[max_len*2];
  char encryptedText[max_len]; // Space outgoing encrypted text

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }
 
     // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

  // Accept a connection, blocking if one is not available until one connects
  while(1){

    // -------------Accept the connection request which creates a connection socket for listening-----------
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }


    // -------------Create a new child process for communication-----------
    pid_t spawnPid = -5;
    int childExitMethod;

    // Fork a new process
    spawnPid = fork();
    switch(spawnPid){
        case -1:
            perror("fork() failed!");

        // Child process where the actual encryption take place
        case 0:
            
            // Reset buffer and count
            charsRead = 0;
            memset(buffer, '\0', buffer_size);
            
            // -------------Initial authorization check that only allows message from enc_client-----------
            // Read the client's message from the socket            
            charsRead = recv(connectionSocket, buffer, buffer_size-1, 0); 
            if (charsRead < 0){
              error("ERROR reading from socket");
            }

            // Check it's communicating with the enc_client
            //If not from enc-client, reject the client and exit the process
            if (strcmp(buffer, "ENC_CLIENT") != 0){
              memset(buffer, '\0', buffer_size);
              charsRead = send(connectionSocket, 
                            "Rejected by server", 18, 0); 
              exit(2);
            }

            // Send a Success message back to the client
            charsRead = send(connectionSocket, 
                            "Approved by server", 18, 0); 
            if (charsRead < 0){
              error("ERROR writing to socket");
            } 

            // Reset buffer and count
            charsRead = 0;
            memset(buffer, '\0', buffer_size);

            // Read the plain text size from client's message from the socket
            charsRead = recv(connectionSocket, buffer, buffer_size-1, 0);
            //printf("2 SERVER: I received this from the client: \"%s\"\n", buffer); 
            if (charsRead < 0){
              error("ERROR reading from socket");
            }

            int plainTextSize = atoi(buffer);    

            // --------------------Receive text from server -------------------------            

            // Read the client's message from the socket
            charsRead = 0;
            while (charsRead < plainTextSize*2){
              memset(buffer, '\0', buffer_size);
              int curr_charsRead = recv(connectionSocket, buffer, 255, 0); 
              //printf("3 SERVER: I received this from the client: \"%s\"\n", buffer); 
              if (curr_charsRead < 0){
                error("ERROR reading from socket");
              }
              charsRead += curr_charsRead;
              strcat(totalText, buffer);
            }

            // Extract plain text and key
            for(int i = 0; i< plainTextSize; i++){
              plainText[i] = totalText[i];
              key[i] = totalText[plainTextSize + i + 1];
            }
            
            // Encrypt
            encrypt(plainText, key, plainTextSize, encryptedText);

            // Send the cipertext back to client via the same socket
            charsRead = 0;
            charsRead = send(connectionSocket, 
                encryptedText, strlen(encryptedText), 0); 
                if (charsRead < 0){
                  error("ERROR writing to socket");
                }            
            exit(0);
       
        // Parent process
        default:
            // WNOHANG specified. If the child hasn't terminated, waitpid will immediately return with value 0
            //----------------not sure if need spawnPid = -------------------------???
            spawnPid = waitpid(spawnPid, &childExitMethod, WNOHANG); 
    }    

    // Close the connection socket for this client
    close(connectionSocket);     
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}


