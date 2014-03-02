#ifndef EJOY_2D_ARRAY_H
#define EJOY_2D_ARRAY_H

#if defined(_MSC_VER)
#	include <malloc.h>
#	define ARRAY(type, name, size) type* name = (type*)_alloca((size) * sizeof(type))
#else
#	define ARRAY(type, name, size) type name[size]
#endif

#endif
