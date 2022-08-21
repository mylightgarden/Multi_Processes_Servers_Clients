#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

/**
* Dec_Server code
*/

int buffer_size = 256;

// Decrypt cipher text to decrypted Text
void decrypt(char cipherText[], char key[], long len, char decryptedText[]){
  int i;
  for (i=0; i<len; i++){
    // conver chars to numbers
    int currCipher = cipherText[i] == ' '? 0 : cipherText[i] - 64;
    int currKey = key[i] == ' '? 0 : key[i] - 64;

    // conver numbers to chars
    int diff = currCipher - currKey;
    if (diff < 0){
      diff+= 27;
    }
    int deCipher = diff % 27;
    decryptedText[i] = deCipher == 0 ? ' ' : deCipher+64;
  }
  decryptedText[i+1] = '\0';
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
  char cipherText[max_len], key[max_len]; // Space for incoming cipher text, key and outgoing decrypted text
  char totalText[max_len*2];
  char decryptedText[max_len]; // Space outgoing decrypted text

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

        // Child process where the actual decryption take place
        case 0:
            
            // Reset buffer and count
            charsRead = 0;
            memset(buffer, '\0', buffer_size);
            
            // -------------Initial authorization check that only allows message from dec_client-----------
            // Read the client's message from the socket            
            charsRead = recv(connectionSocket, buffer, buffer_size-1, 0); 
            if (charsRead < 0){
              error("ERROR reading from socket");
            }

            // Check it's communicating with the dec_client
            //If not from dec-client, reject the client and exit the process
            if (strcmp(buffer, "DEC_CLIENT") != 0){
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

            // Read the cipher text size from client's message from the socket
            charsRead = recv(connectionSocket, buffer, buffer_size-1, 0);
            //printf("2 Dec SERVER: I received this from the client: \"%s\"\n", buffer); 
            if (charsRead < 0){
              error("ERROR reading from socket");
            }

            int cipherTextSize = atoi(buffer);    

            // --------------------Receive text from server -------------------------            

            // Read the client's message from the socket
            charsRead = 0;
            while (charsRead < cipherTextSize*2){
              memset(buffer, '\0', buffer_size);
              int curr_charsRead = recv(connectionSocket, buffer, buffer_size-1, 0); 
              //printf("3 Dec SERVER: I received this from the client: \"%s\"\n", buffer); 
              if (curr_charsRead < 0){
                error("ERROR reading from socket");
              }
              charsRead += curr_charsRead;
              strcat(totalText, buffer);
            }

            // Extract cipher text and key
            for(int i = 0; i< cipherTextSize; i++){
              cipherText[i] = totalText[i];
              key[i] = totalText[cipherTextSize + i + 1];
            }
            
            // Decrypt
            decrypt(cipherText, key, cipherTextSize, decryptedText);

            // Send the cipertext back to client via the same socket
            charsRead = 0;
            charsRead = send(connectionSocket, 
                decryptedText, strlen(decryptedText), 0); 
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


