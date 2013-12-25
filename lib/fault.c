#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

void
fault(const char * format, ...) {
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	exit(1);
}

