#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <map>

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

	int collisionLength = 5;

	std::map<std::string, std::string> map;

	//get command line argument
	if (argc >= 2){
		collisionLength = atoi(argv[1]);
	}

	//init seed with current time
	srand(time(NULL));
	
	bool didCollide = false;
	unsigned int tries = 0;

	clock_t start = clock();
	while(!didCollide){
		unsigned char message[STRING_LENGTH];
		memset(message, 0, STRING_LENGTH);
		randomString(message, STRING_LENGTH);

    	char fullHash[33] = {0};
		md5(message, strlen((char*)message), (unsigned char*)fullHash);

    	char* shortHash = (char*)malloc(collisionLength+1);
    	memset(shortHash, 0, collisionLength+1);
    	memcpy(shortHash, fullHash, collisionLength);

    	if(!map.insert(std::make_pair(shortHash, fullHash)).second){
    	 	didCollide = true;
    	 	printf("Target Hash: %s\n", fullHash);
    	 	printf("Found SHash: %s\n", shortHash);
    	 	printf("Found FHash: %s\n", map[shortHash].c_str());
    	}

		free(shortHash);
    	tries++;
	}

	clock_t end = clock();

	printf("Found Collision with length %d in %d tries\n", collisionLength, tries);
	printf("Time: %.5f sec\n", (double)(end - start)/CLOCKS_PER_SEC);

	return 0;
}