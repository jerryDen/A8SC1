#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <linux/watchdog.h>

#include "watchdog.h"

static int wtd_fd = -1;


int openWTD(void)
{
	wtd_fd = open("/dev/watchdog", O_WRONLY);
	if(wtd_fd == -1){

		printf("open(\"/dec/watchdog\", O_WRONLY)\n");
		return -1;
	}
	return 0;
}

void keepWTDalive(int dummy)
{
	if( wtd_fd < 0 ) return;

	ioctl(wtd_fd, WDIOC_SETTIMEOUT, &dummy);
}
void closeWTD(void)
{
	if(wtd_fd > 0)
		close(wtd_fd);
}

