#include <stdlib.h>
#include <stdio.h>
#include <math.h> 
#include <time.h>

/*
 Generate a randome key
*/
int main(int argc, char* argv[]){
    //Check arguments
    if (argc != 2){
        fprintf(stderr, "Please include and only include length of the key to argument.");
    }
        
    int keyLen = atoi(argv[1]);

    if(keyLen < 1){
        fprintf(stderr, "Length of the key must be larger than 1.");
    }

    char letters[27] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *key = (char *) malloc((keyLen+1)*sizeof(char));

    srand(time(NULL));

    // Generate a randome key that has a length of keyLen
    for(int i=0; i<keyLen; i++){
        int r = rand()%27;
        key[i] = letters[r];
    }

    // Set the last char to a newline
    key[keyLen] = '\n';

    fprintf(stdout, "%s", key);
    free(key);      

    return 0;
}
