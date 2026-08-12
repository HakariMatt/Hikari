#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <stdarg.h>
typedef enum {
	INFO,
	WARNING,
	ERROR
} log_level;

static void vprint(log_level level, const char* fmt, va_list arg) {
	switch (level) {
		case INFO:
			printf("\x1b[1;34m[INFO]\x1b[0m ");
			break;
		case WARNING:
			printf("\x1b[1;33m[WARNING]\x1b[0m ");
			break;
		case ERROR:
			printf("\x1b[1;31m[ERROR]\x1b[0m ");
			break;
	}

	vprintf(fmt, arg);

	printf("\n");
}

static void print(log_level level, const char* fmt, ...) {
	va_list arg;

	va_start(arg, fmt);
	vprint(level, fmt, arg);
	va_end(arg);
}

#endif
