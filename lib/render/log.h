#ifndef ejoy3d_log_h
#define ejoy3d_log_h

#include <stdio.h>

struct log {
	FILE *f;
};

void log_init(struct log *log, FILE *f);
void log_printf(struct log *log, const char * format, ...);

#endif
