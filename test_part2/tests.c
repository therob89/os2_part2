#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define O_SESSION 0x4000000

int main (){
	
	/*

	char **buf_array = malloc(4*sizeof(char*));
	loff_t *offset = malloc(sizeof(offset));
	int *array  = malloc(4*sizeof(int));
	int i;
	for (i=0;i<4;i++){
		buf_array[i] = malloc(2048*sizeof(char));
	}
	*offset = 10;
	char src [128];
	strcpy(src,"Hello is all ok?");
	char add [64];
	char dest[128];
	strcpy(add,"Yes");
	memcpy(dest,src,sizeof(src));
	//memcpy(dest+*offset,add,sizeof(add));
	
	//strcat(dest,"..Yes");
	printf("%s \n",dest);
	memcpy(buf_array[0],dest,sizeof(dest));
	printf("buf array[0] %s \n",buf_array[0]);
	//char * t1 = buf_array[0];	
	//char temp2[2048];
	//memcpy(temp2,buf_array[0],2048);
	//printf("REEEES %s \n",temp2);
	
	
	printf(" AA -> %s \n",buf_array[0]+8);
	char *temp7 = malloc(2048);
	memcpy(temp7,buf_array[0],2048);
	memcpy(temp7+*offset,add,sizeof(add));
	printf("%s \n",temp7);
	
	
	
	
	free(buf_array);
	free(array);
	free(offset);
	free(temp7);
	*/
	
	char buff[48];
	strcpy(buff,"CIAO");
	printf ("%li %li \n",sizeof(buff),strlen(buff));
	return 0;

}
