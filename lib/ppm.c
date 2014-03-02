#include "ppm.h"
#include "texture.h"
#include "array.h"

#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PPM_RGBA8 0
#define PPM_RGB8 1
#define PPM_RGBA4 2
#define PPM_RGB4 3
#define PPM_ALPHA8 4
#define PPM_ALPHA4 5

struct ppm {
	int type;
	int depth;
	int step;
	int width;
	int height;
	uint8_t *buffer;
};

#define LINEMAX 128

static char *
readline(FILE *f, char *buffer) {
	for (;;) {
		char * ret = fgets(buffer, LINEMAX, f);
		if (ret == NULL) {
			return NULL;
		}
		if (ret[0] != '#') {
			return ret;
		}
	}
}

static int
ppm_header(FILE *f, struct ppm *ppm) {
	char tmp[LINEMAX];
	char *line = readline(f, tmp);
	if (line == NULL)
		return 0;
	char c = 0;
	sscanf(line, "P%c", &c);
	ppm->type = c;
	line = readline(f, tmp);
	if (line == NULL)
		return 0;
	sscanf(line, "%d %d", &(ppm->width), &(ppm->height));
	line = readline(f, tmp);
	if (line == NULL)
		return 0;
	sscanf(line, "%d", &(ppm->depth));
	return 1;
}

static int
ppm_data(struct ppm *ppm, FILE *f, int id, int skip) {
	int i;
	int n = ppm->width * ppm->height;
	uint8_t * buffer = ppm->buffer + skip;
	uint8_t * tmp;
	int step = ppm->step;
	switch(id) {
	case '3':	// RGB text
		for (i=0;i<n;i++) {
			int r,g,b;
			fscanf(f, "%d %d %d", &r,&g,&b);
			buffer[i*step+0] = (uint8_t)r;
			buffer[i*step+1] = (uint8_t)g;
			buffer[i*step+2] = (uint8_t)b;
		}
		break;
	case '2':	// ALPHA text
		for (i=0;i<n;i++) {
			int alpha;
			fscanf(f, "%d", &alpha);
			buffer[i*step] = (uint8_t)alpha;
		}
		break;
	case '6':	// RGB binary
		tmp = (uint8_t *)malloc(n * 3);
		if (fread(tmp, n*3, 1, f)==0) {
			free(tmp);
			return 0;
		}
		for (i=0;i<n;i++) {
			buffer[i*step+0] = tmp[i*3+0];
			buffer[i*step+1] = tmp[i*3+1];
			buffer[i*step+2] = tmp[i*3+2];
		}
		free(tmp);
		break;
	case '5':	// ALPHA binary
		tmp = (uint8_t *)malloc(n);
		if (fread(tmp, n, 1, f)==0) {
			free(tmp);
			return 0;
		}
		for (i=0;i<n;i++) {
			buffer[i*step] = tmp[i];
		}
		free(tmp);
		break;
	default:
		return 0;
	}
	return 1;
}

static int
loadppm_from_file(FILE *rgb, FILE *alpha, struct ppm *ppm) {
	ppm->buffer = NULL;
	ppm->step = 0;
	int rgb_id = 0;
	int alpha_id = 0;
	if (rgb) {
		if (!ppm_header(rgb, ppm)) {
			return 0;
		}
		rgb_id = ppm->type;
		ppm->step += 3;
	}
	if (alpha) {
		if (rgb == NULL) {
			if (!ppm_header(alpha, ppm)) {
				return 0;
			}
		} else {
			struct ppm pgm;
			if (!ppm_header(alpha, &pgm)) {
				return 0;
			}
			if (ppm->depth != pgm.depth || ppm->width != pgm.width || ppm->height != pgm.height) {
				return 0;
			}
			alpha_id = pgm.type;
		}
		ppm->step += 1;
	}
	ppm->buffer = (uint8_t *)malloc(ppm->height * ppm->width * ppm->step);
	if (rgb) {
		if (!ppm_data(ppm, rgb, rgb_id, 0))
			return 0;
	}
	if (alpha) {
		int skip = 0;
		if (rgb) {
			skip = 3;
		}
		if (!ppm_data(ppm, alpha, alpha_id, skip)) 
			return 0;
	}

	return 1;
}

static int
loadppm(lua_State *L) {
	size_t sz = 0;
	const char * filename = luaL_checklstring(L, 1, &sz);
	ARRAY(char, tmp, sz+5);
	sprintf(tmp, "%s.ppm", filename);
	FILE *rgb = fopen(tmp, "rb");
	sprintf(tmp, "%s.pgm", filename);
	FILE *alpha = fopen(tmp, "rb");

	if (rgb == NULL && alpha == NULL) {
		return luaL_error(L, "Can't open %s(.ppm/.pgm)", filename);
	}

	struct ppm ppm;

	int ok = loadppm_from_file(rgb, alpha, &ppm);

	if (rgb) {
		fclose(rgb);
	}
	if (alpha) {
		fclose(alpha);
	}
	if (!ok) {
		if (ppm.buffer) {
			free(ppm.buffer);
		}
		luaL_error(L, "Invalid file %s", filename);
	}

	if (ppm.depth == 255) {
		if (ppm.step == 4) {
			lua_pushliteral(L, "RGBA8");
		} else if (ppm.step == 3) {
			lua_pushliteral(L, "RGB8");
		} else {
			lua_pushliteral(L, "ALPHA8");
		}
	} else {
		if (ppm.step == 4) {
			lua_pushliteral(L, "RGBA4");
		} else if (ppm.step == 3) {
			lua_pushliteral(L, "RGB4");
		} else {
			lua_pushliteral(L, "ALPHA4");
		}
	}
	lua_pushinteger(L, ppm.width);
	lua_pushinteger(L, ppm.height);
	int n = ppm.width * ppm.height * ppm.step;
	lua_createtable(L, n, 0);
	int i;
	for (i=0;i<n;i++) {
		lua_pushinteger(L, ppm.buffer[i]);
		lua_rawseti(L, -2, i+1);
	}
	free(ppm.buffer);
	return 4;
}

static int
loadtexture(lua_State *L) {
	int id = (int)luaL_checkinteger(L,1);
	size_t sz = 0;
	const char * filename = luaL_checklstring(L, 2, &sz);
	ARRAY(char, tmp, sz + 5);
	sprintf(tmp, "%s.ppm", filename);
	FILE *rgb = fopen(tmp, "rb");
	sprintf(tmp, "%s.pgm", filename);
	FILE *alpha = fopen(tmp, "rb");
	if (rgb == NULL && alpha == NULL) {
		return luaL_error(L, "Can't open %s(.ppm/.pgm)", filename);
	}
	struct ppm ppm;

	int ok = loadppm_from_file(rgb, alpha, &ppm);

	if (rgb) {
		fclose(rgb);
	}
	if (alpha) {
		fclose(alpha);
	}
	if (!ok) {
		if (ppm.buffer) {
			free(ppm.buffer);
		}
		luaL_error(L, "Invalid file %s", filename);
	}

	int type = 0;
	if (ppm.depth == 255) {
		if (ppm.step == 4) {
			type = Texture2DPixelFormat_RGBA8888;
		} else if (ppm.step == 3) {
			type = Texture2DPixelFormat_RGB888;
		} else {
			type = Texture2DPixelFormat_A8;
		}
	} else {
		if (ppm.step == 4) {
			type = Texture2DPixelFormat_RGBA4444;
			uint16_t * tmp = (uint16_t * )malloc(ppm.width * ppm.height * sizeof(uint16_t));
			int i;
			for (i=0;i<ppm.width * ppm.height;i++) {
				uint32_t r = ppm.buffer[i*4+0];
				uint32_t g = ppm.buffer[i*4+1];
				uint32_t b = ppm.buffer[i*4+2];
				uint32_t a = ppm.buffer[i*4+3];
				tmp[i] = r << 12 | g << 8 | b << 4 | a;
			}
			free(ppm.buffer);
			ppm.buffer = (uint8_t*)tmp;
		} else if (ppm.step == 3) {
			type = Texture2DPixelFormat_RGB565;
			uint16_t * tmp = (uint16_t *)malloc(ppm.width * ppm.height * sizeof(uint16_t));
			int i;
			for (i=0;i<ppm.width * ppm.height;i++) {
				uint32_t r = ppm.buffer[i*3+0];
				if (r == 15) {
					r = 31;
				} else {
					r <<= 1;
				}
				uint32_t g = ppm.buffer[i*3+1];
				if (g == 15) {
					g = 63;
				} else {
					g <<= 2;
				}
				uint32_t b = ppm.buffer[i*3+2];
				if (b == 15) {
					b = 31;
				} else {
					b <<= 1;
				}
				tmp[i] = r << 11 | g << 5 | b;
			}
			free(ppm.buffer);
			ppm.buffer = (uint8_t*)tmp;
		} else {
			type = Texture2DPixelFormat_A8;
			int i;
			for (i=0;i<ppm.width * ppm.height;i++) {
				uint8_t c =	ppm.buffer[i];
				if (c == 15) {
					ppm.buffer[i] = 255;
				} else {
					ppm.buffer[i] *= 16;
				}
			}
		}
	}

	const char * err = texture_load(id, type, ppm.width, ppm.height, ppm.buffer);
	free(ppm.buffer);
	if (err) {
		return luaL_error(L, "%s", err);
	}

	return 0;
}

static void
ppm_type(lua_State *L, const char * format, struct ppm *ppm) {
	if (strcmp(format, "RGBA8")==0) {
		ppm->type = PPM_RGBA8;
		ppm->depth = 255;
		ppm->step = 4;
	} else if (strcmp(format, "RGB8")==0) {
		ppm->type = PPM_RGB8;
		ppm->depth = 255;
		ppm->step = 3;
	} else if (strcmp(format, "RGBA4")==0) {
		ppm->type = PPM_RGBA4;
		ppm->depth = 15;
		ppm->step = 4;
	} else if (strcmp(format, "RGB4")==0) {
		ppm->type = PPM_RGB4;
		ppm->depth = 15;
		ppm->step = 3;
	} else if (strcmp(format, "ALPHA8")==0) {
		ppm->type = PPM_ALPHA8;
		ppm->depth = 255;
		ppm->step = 1;
	} else if (strcmp(format, "ALPHA4")==0) {
		ppm->type = PPM_ALPHA4;
		ppm->depth = 15;
		ppm->step = 1;
	} else {
		luaL_error(L, "Invalid ppm format %s, only support RGBA8 RGBA4 RGB8 RGB4 ALPHA8 ALPHA4", format);
	}
}

/*
	string filename (without ext)
	type : RGBA8 RGB8 RGBA4 RGBA4 ALPHA8 ALPHA4
	integer width
	integer height
	table data
*/

static void
save_rgb(lua_State *L, int step, int depth) {
	size_t sz = 0;
	const char * filename = lua_tolstring(L, 1, &sz);

	ARRAY(char, tmp, sz + 5);

	int width = (int)lua_tointeger(L, 3);
	int height = (int)lua_tointeger(L, 4);
	sprintf(tmp, "%s.ppm", filename);
	FILE *f = fopen(tmp,"wb");
	if (f == NULL) {
		luaL_error(L, "Can't write to %s", tmp);
	}
	fprintf(f, 
		"P6\n"
		"%d %d\n"
		"%d\n"
		, width, height, depth);
	int i;
	uint8_t *buffer = (uint8_t *)malloc(width * height * 3);
	for (i=0;i<width * height;i++) {
		lua_rawgeti(L, 5, i*step+1);
		lua_rawgeti(L, 5, i*step+2);
		lua_rawgeti(L, 5, i*step+3);
		buffer[i*3+0] = (uint8_t)lua_tointeger(L, -3);
		buffer[i*3+1] =(uint8_t)lua_tointeger(L, -2);
		buffer[i*3+2] =(uint8_t)lua_tointeger(L, -1);
		lua_pop(L,3);
	}
	int wn = fwrite(buffer, width * height * 3,1,f);
	free(buffer);
	fclose(f);
	if (wn != 1) {
		luaL_error(L, "Write to %s failed", tmp);
	}
}

static void
save_alpha(lua_State *L, int step, int depth, int offset) {
	size_t sz = 0;
	const char * filename = lua_tolstring(L, 1, &sz);

	ARRAY(char, tmp, sz + 5);

	int width = (int)lua_tointeger(L, 3);
	int height = (int)lua_tointeger(L, 4);
	sprintf(tmp, "%s.pgm", filename);
	FILE *f = fopen(tmp,"wb");
	if (f == NULL) {
		luaL_error(L, "Can't write to %s", tmp);
	}
	fprintf(f, 
		"P5\n"
		"%d %d\n"
		"%d\n"
		, width, height, depth);
	int i;
	uint8_t *buffer = (uint8_t *)malloc(width * height);
	for (i=0;i<width * height;i++) {
		lua_rawgeti(L, 5, i*step+1+offset);
		buffer[i] = (uint8_t)lua_tointeger(L, -1);
		lua_pop(L,1);
	}
	int wn = fwrite(buffer, width * height,1,f);
	free(buffer);
	fclose(f);
	if (wn != 1) {
		luaL_error(L, "Write to %s failed", tmp);
	}
}

static int
saveppm(lua_State *L) {
	struct ppm ppm;
	luaL_checktype(L,1,LUA_TSTRING);
	ppm_type(L, luaL_checkstring(L, 2), &ppm);
	ppm.width = (int)luaL_checkinteger(L, 3);
	ppm.height = (int)luaL_checkinteger(L, 4);
	luaL_checktype(L, 5, LUA_TTABLE);
	int n = (int)lua_rawlen(L,5);
	if (n != ppm.width * ppm.height * ppm.step) {
		return luaL_error(L, "Data number %d invalid , should be %d * %d * %d = %d", n, ppm.width, ppm.height, ppm.step, ppm.width * ppm.height * ppm.step);
	}
	if (ppm.type != PPM_ALPHA8 && ppm.type != PPM_ALPHA4) {
		save_rgb(L, ppm.step, ppm.depth);
	}
	if (ppm.type != PPM_RGB8 && ppm.type != PPM_RGB4) {
		int offset = 3;
		if (ppm.type ==  PPM_ALPHA8 || ppm.type == PPM_ALPHA4) {
			offset = 0;
		}
		save_alpha(L, ppm.step, ppm.depth, offset);
	}

	return 0;
}

int 
ejoy2d_ppm(lua_State *L) {
	luaL_Reg l[] = {
		{ "texture", loadtexture },
		{ "load", loadppm },
		{ "save", saveppm },
		{ NULL, NULL },
	};

	luaL_newlib(L,l);

	return 1;
}
