#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* sleep */

#include <syslog.h>

#include "sensor.h"

/*thread for push log to syslog.*/
static void *sensor_log_thr(void *user) {
	static time_t last_time, now;
	struct sensor_handle *sh;

	sh = (struct sensor_handle *) user;
	for(;;) {
		time(&now);
		/*send statistic to server every 5 min*/
		if((now - last_time >= 300) && !(now % 300)) {
			time(&last_time);
			syslog(sh->log_pri, "%s: input=%ld, output=%ld", 
					sh->hostname, sh->count_input, sh->count_output);
			sh->count_input = 0;
			sh->count_output = 0;
		} else
			sleep(1);
	}

	return NULL;
}

int sensor_log_init(struct sensor_handle *sh)
{
	if(pthread_create(&sh->com_thr, NULL, sensor_log_thr, sh) == 0) {
		printf("Create com thread success.\n");
		return 0;
	} else 
		return -1;
}