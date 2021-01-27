#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include "md5.h"

#define STRING_LENGTH 256

void randomString(unsigned char *dest, unsigned int length){
	    char charset[] = "0123456789"
                     	 "abcdefghijklmnopqrstuvwxyz"
                     	 "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	while (length-- > 1) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';             	
}

int main(int argc, char** argv){

	int collisionLength = 4;

	//get command line argument
	if (argc >= 2){
		collisionLength = atoi(argv[1]);
	}

	//init seed with current time
	srand(time(NULL));
	
	unsigned char message[STRING_LENGTH];
	memset(message, 0, STRING_LENGTH);
	randomString(message, STRING_LENGTH);

	unsigned char mdTargetString[33];
	memset(mdTargetString, 0, 33);
	md5(message, STRING_LENGTH, mdTargetString);

    //roll the dice
	unsigned char anotherMessage[STRING_LENGTH];
	memset(anotherMessage, 0, STRING_LENGTH);
	
	unsigned char mdString[33];
	memset(mdString, 0, 33);

	unsigned int tries = 0;
        
	clock_t start = clock();
    while(strncmp((char*)mdString, (char*)mdTargetString, collisionLength)){
    	randomString(anotherMessage, STRING_LENGTH);
   		md5(anotherMessage, STRING_LENGTH, mdString);
        tries++;
    }
	clock_t end = clock();

	printf("Target Hash: %s\n", mdTargetString);
	printf("Found  Hash: %s\n", mdString);
	
	printf("Found Collision with length %d in %d tries\n", collisionLength, tries);
	printf("Time: %.5f sec\n", (double)(end - start)/CLOCKS_PER_SEC);

	return 0;
}
