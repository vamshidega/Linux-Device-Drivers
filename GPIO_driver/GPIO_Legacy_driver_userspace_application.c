/*
 * This code is the user space test application for GPIO_legacy_driver.c
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

int main()
{
	int fd, i;
	ssize_t ret;
	char value = 3;
	
	fd = open("/dev/gpio_dev", O_RDWR );
	if(fd<0)
		printf("failed acquiring file descriptor return status %d\n",fd);

	//last two digits pay the role. 001 - LED_18 will be high, 010 - LED_19 will be high, 
	//011 - means both will be high.
	ret = write( fd, &value, 1);
	if(ret<0) printf("write operation failed with return status %d\n",ret);
    
    value = 0;
	ret = read( fd, &value, 1);
	if(ret<0) printf("write operation failed with return status %d\n",ret);
	else printf("recieved value = %d\n",value); 

	close(fd);
}
