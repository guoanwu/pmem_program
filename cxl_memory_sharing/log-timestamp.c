#include "log-timestamp.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#ifdef DOTC
#export= LOG
#endif

void LOG(const char *fmt, ...) {
	char date[20];
	struct timeval tv;
	va_list args;

	/* print the progname, version, and timestamp */
	gettimeofday(&tv, NULL);
	strftime(date, sizeof(date) / sizeof(*date), "%Y-%m-%dT%H:%M:%S", gmtime(&tv.tv_sec));
	printf("[%s.%03dZ] ", date, tv.tv_usec / 1000);

	/* printf like normal */
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
