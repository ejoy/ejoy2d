#pragma once

#if !defined(EJOY2D_LUA_PATH)
#  define EJOY2D_PP_CAT(a, b) EJOY2D_PP_CAT_I(a, b)
#  define EJOY2D_PP_CAT_I(a, b) a ## b
#  define EJOY2D_PP_STRINGIZE(text)   EJOY2D_PP_STRINGIZE_A((text))
#  define EJOY2D_PP_STRINGIZE_A(arg)  EJOY2D_PP_STRINGIZE_I arg
#  define EJOY2D_PP_STRINGIZE_I(text) #text
#  define EJOY2D_LUA_PATH(file)   EJOY2D_PP_STRINGIZE(EJOY2D_PP_CAT(EJOY2D_PP_CAT(EJOY2D_LUA, \), file))
#endif
