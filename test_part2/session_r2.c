
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define PATH 	    "./pro.txt"


int  main(){
	
	int fd,ret;
	char *str = "CIAO";
	fd = open(PATH,O_RDWR);
	printf ("This is the fd %d \n",fd);
	getchar();
	ret = write(fd,str,strlen(str));
	printf ("This is the ret %d \n",ret);
	return 0;
	
}
