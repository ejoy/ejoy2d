#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

void
fault(const char * format, ...) {
	if (format[0] == '!') {
		va_list ap;
		va_start(ap, format);
		vprintf(format+1, ap);
		va_end(ap);
	} else {
		va_list ap;
		va_start(ap, format);
		vprintf(format, ap);
		va_end(ap);
		exit(1);
	}
}

