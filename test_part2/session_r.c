
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define O_SESSION 	0x4000000
#define PATH 	    "/home/rob/Scrivania/output_session/session_file.txt"
#define PATH2 	    "./pro.txt"


int  main(){
	
	int fd,ret;
	char *str = "CIAO ho scritto dopo di te";
	fd = open(PATH,O_SESSION|O_RDWR|O_TRUNC);
	printf ("This is the fd %d \n",fd);
	getchar();
	ret = write(fd,str,strlen(str));
	printf ("This is the ret of write %d \n",ret);
	getchar();
	ret = close(fd);
	printf ("This is the ret of close%d \n",ret);
	return 0;
	
}
