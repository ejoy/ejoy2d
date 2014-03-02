#include "label.h"
#include "array.h"
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

void
font_create(int font_size, struct font_context *ctx) {
	TEXTMETRIC tm;
	HFONT f = CreateFontW(
		font_size,0,
		0, 0, 
		FW_SEMIBOLD,
		FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, 
		OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		DEFAULT_PITCH, 
		NULL
	);

	HDC dc = CreateCompatibleDC(NULL);
	SelectObject(dc, f);
	ctx->font = (void *)f;
	ctx->dc = (void *)dc;
	GetTextMetrics(dc,&tm);
	ctx->h=tm.tmHeight + 1;
	ctx->ascent=tm.tmAscent;
}

void
font_release(struct font_context *ctx) {
	DeleteObject((HFONT)ctx->font);
	DeleteDC((HDC)ctx->dc);
}

static MAT2 mat2={{0,1},{0,0},{0,0},{0,1}};

void 
font_size(const char *str, int unicode, struct font_context *ctx) {
	GLYPHMETRICS gm;

	GetGlyphOutlineW(
		(HDC)ctx->dc,
		unicode,
		GGO_GRAY8_BITMAP,
		&gm,
		0,
		NULL,
		&mat2
	);
	
	ctx->w = gm.gmCellIncX + 1;
}

void 
font_glyph(const char * str, int unicode, void * buffer, struct font_context *ctx) {
	GLYPHMETRICS gm;
	memset(&gm,0,sizeof(gm));

	ARRAY(uint8_t, tmp, ctx->w * ctx->h);
	memset(tmp,0, ctx->w * ctx->h);

	GetGlyphOutlineW(
		(HDC)ctx->dc,
		unicode,
		GGO_GRAY8_BITMAP,
		&gm,
		ctx->w * ctx->h,
		tmp,
		&mat2
	);

	int w = (gm.gmBlackBoxX + 3) & ~3;
	int h = gm.gmBlackBoxY;

	uint8_t * buf = (uint8_t *)buffer;
	int offx = gm.gmptGlyphOrigin.x;
	int offy = ctx->ascent - gm.gmptGlyphOrigin.y;
	assert(offx >= 0);
	assert(offy >= 0);
	assert(offx + gm.gmBlackBoxX <= ctx->w);
	assert(offy + h <= ctx->h);

	int i,j;

	for (i=0;i<h;i++) {
		for (j=0;j<gm.gmBlackBoxX;j++) {
			int src = tmp[i*w+j];
			buf[(i + offy)*ctx->w + j + offx] = src * 255 / 64;
		}
	}
}


