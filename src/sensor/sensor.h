#include <stdint.h>
#include <pthread.h>

struct sensor_handle {
	uint64_t count_input;
	uint64_t count_output;
	pthread_t com_thr;
	int log_pri;
	char hostname[256];
};


int sensor_log_init(struct sensor_handle *sh);