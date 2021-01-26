#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>

#include "md5.h"

//this could be usefull
void randomString(unsigned char *dest, unsigned int length){
	
	// do not forget to init seed (once) before calling this function
	// srand(time(NULL));

	char charset[] = "0123456789"
                     	 "abcdefghijklmnopqrstuvwxyz"
                     	 "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';             	
}

int main(int argc, char** argv){

	unsigned char* message = "Test";

	unsigned char hash[33];
	memset(hash, 0, 33);
	
	md5(message, strlen(message), hash);

	printf("Message: %s\n", message);
	printf("   Hash: %s\n", hash);

	return 0;
}
