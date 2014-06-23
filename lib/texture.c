#include "texture.h"
#include "shader.h"

#define MAX_TEXTURE 128
#define IS_POT(x) (((x) & ((x) -1)) == 0)

struct texture {
	int width;
	int height;
	float invw;
	float invh;
	GLuint id;
	GLuint fb; /// rt ¶ÔÓ¦µÄframe buffer
};

struct texture_pool {
	int count;
	struct texture tex[MAX_TEXTURE];
};

static struct texture_pool POOL;

const char * 
texture_load(int id, int pixel_format, int pixel_width, int pixel_height, void *data) {
	if (id >= MAX_TEXTURE) {
		return "Too many texture";
	}
	struct texture * tex = &POOL.tex[id];
	if (id >= POOL.count) {
		POOL.count = id + 1;
	} 
	tex->fb = 0;
	tex->width = pixel_width;
	tex->height = pixel_height;
	tex->invw = 1.0f / (float)pixel_width;
	tex->invh = 1.0f / (float)pixel_height;
	if (tex->id == 0) {
		glGenTextures(1, &tex->id);
	}
	if (data == NULL) {
		// empty texture
		return NULL;
	}
	if ((pixel_format == Texture2DPixelFormat_RGBA8888) || 
		( IS_POT(pixel_width) && IS_POT(pixel_height))) {
		glPixelStorei(GL_UNPACK_ALIGNMENT,4);
	} else {
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	}
	glActiveTexture(GL_TEXTURE0);
	shader_texture(tex->id);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	switch(pixel_format) {
		case Texture2DPixelFormat_RGBA8888:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)pixel_width, (GLsizei)pixel_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			break;
		case Texture2DPixelFormat_RGB888:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)pixel_width, (GLsizei)pixel_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			break;
		case Texture2DPixelFormat_RGBA4444:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)pixel_width, (GLsizei)pixel_height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
			break;
		case Texture2DPixelFormat_RGB565:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)pixel_width, (GLsizei)pixel_height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
			break;
		case Texture2DPixelFormat_A8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, (GLsizei)pixel_width, (GLsizei)pixel_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
			break;
#if GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
		case Texture2DPixelFormat_PVRTC4:
			{
				int size = pixel_width * pixel_height * 4 / 8;
				uint8_t* p = data+4;
				glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,
																			(GLsizei)pixel_width, (GLsizei)pixel_height, 0, size, p);
			
			}
			break;
#endif
		default:
			glDeleteTextures(1,&tex->id);
			tex->id = 0;
			return "Invalid pixel format";
	}

	return NULL;
}

const char*
texture_new_rt(int id, int w, int h){

	if (id >= MAX_TEXTURE) {
		return "Too many texture";
	}

	struct texture * tex = &POOL.tex[id];
	if (id >= POOL.count) {
		POOL.count = id + 1;
	}

	tex->width = w;
	tex->height = h;
	tex->invw = 1.0f / (float) w;
	tex->invh = 1.0f / (float) h;
	if (tex->id == 0) {
		glGenTextures(1, &tex->id);
		glGenFramebuffers(1, &tex->fb);
	}

	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) w, (GLsizei) h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);


	glBindFramebuffer(GL_FRAMEBUFFER, tex->fb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->id, 0);


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return "frame buffer is not complete";
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return NULL;
}

const char*
texture_active_rt(int id) {
	if (id < 0 || id >= POOL.count)
		return "Invalid rt id";
	struct texture *tex = &POOL.tex[id];

	glBindFramebuffer(GL_FRAMEBUFFER, tex->fb);

	return NULL;
}



void 
texture_coord(int id, float *x, float *y) {
	if (id < 0 || id >= POOL.count) {
		*x = *y = 0;
		return;
	}
	struct texture *tex = &POOL.tex[id];
//	*x = (*x+0.5f) * tex->invw;
//	*y = (*y+0.5f) * tex->invh;
	*x *= tex->invw;
	*y *= tex->invh;
}

void 
texture_unload(int id) {
	if (id < 0 || id >= POOL.count)
		return;
	struct texture *tex = &POOL.tex[id];
	if (tex->id == 0)
		return;
	glDeleteTextures(1,&tex->id);
	if (tex->fb != 0)
		glDeleteFramebuffers(1, &tex->fb);
	tex->id = 0;
    tex->fb = 0;
}

GLuint 
texture_glid(int id) {
	if (id < 0 || id >= POOL.count)
		return 0;
	struct texture *tex = &POOL.tex[id];
	return tex->id;
}

void 
texture_clearall() {
	int i;
	for (i=0;i<POOL.count;i++) {
		texture_unload(i);
	}
}

void 
texture_exit() {
	texture_clearall();
	POOL.count = 0;
}

void
texture_set_inv(int id, float invw, float invh) {
   if (id < 0 || id >= POOL.count)
       return ;
    
    struct texture *tex = &POOL.tex[id];
    tex->invw = invw;
    tex->invh = invh;
}

void
texture_swap(int ida, int idb) {
    if (ida < 0 || idb < 0 || ida >= POOL.count || idb >= POOL.count)
        return ;
    
    struct texture tex = POOL.tex[ida];
    POOL.tex[ida] = POOL.tex[idb];
    POOL.tex[idb] = tex;
}

void
texture_size(int id, int *width, int *height) {
    if (id < 0 || id >= POOL.count) {
        *width = *height = 0;
        return ;
    }
    
    struct texture *tex = &POOL.tex[id];
    *width = tex->width;
    *height = tex->height;
}

void
texture_delete_framebuffer(int id) {
    if (id < 0 || id >= POOL.count) {
        return;
    }
    
    struct texture *tex = &POOL.tex[id];
    if (tex->fb != 0) {
        glDeleteFramebuffers(1, &tex->fb);
        tex->fb = 0;
    }
}
