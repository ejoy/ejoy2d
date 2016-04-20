#include <string.h>
#include <stdint.h>
#include "render/render.h"

#define NK_IMPLEMENTATION
#include "nuklear_ejoy2d.h"
#include "nuklear.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

struct nk_ejoy2d_device {
	struct render *R;
	struct nk_buffer cmds;
	struct nk_draw_null_texture null;
	RID prog;
	int uniform_proj;
	RID layout;
	RID vbo, ebo;
	RID font_tex;
	void *vertex_buffer;
	void *element_buffer;
	int max_vertex_memory;
	int max_element_memory;
};

static struct nk_ejoy2d {
	int width, height;
	struct nk_ejoy2d_device ogl;
	struct nk_context ctx;
	struct nk_font_atlas atlas;
} ejoy2d;

#ifdef __APPLE__
  #define NK_SHADER_VERSION "#version 150\n"
#else
  #define NK_SHADER_VERSION "#version 300 es\n"
#endif

NK_API void
nk_ejoy2d_device_create(struct render *R, int max_vertex_memory, int max_element_memory) {
	struct shader_init_args sia;
	sia.vs = 
		NK_SHADER_VERSION
		"uniform mat4 ProjMtx;\n"
		"in vec2 Position;\n"
		"in vec2 TexCoord;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main() {\n"
		"   Frag_UV = TexCoord;\n"
		"   Frag_Color = Color;\n"
		"   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
		"}\n";
	sia.fs = 
		NK_SHADER_VERSION
		"precision mediump float;\n"
		"uniform sampler2D Texture;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main(){\n"
		"   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
		"}\n";
	sia.texture = 1;

	const char *texture_uniform[] = {
		"Texture",
	};

	sia.texture_uniform = texture_uniform;

	struct nk_ejoy2d_device *dev = &ejoy2d.ogl;
	dev->R = R;
	nk_buffer_init_default(&dev->cmds);

#define VB_OFFSET(f) ((intptr_t)&(((struct nk_draw_vertex *)NULL)->f))

	struct vertex_attrib va[3] = {
		{ "Position", 0, 2, sizeof(float), VB_OFFSET(position) },
		{ "TexCoord", 0, 2, sizeof(float), VB_OFFSET(uv) },
		{ "Color", 0, 4, sizeof(nk_uint)/4, VB_OFFSET(col) },
	};

	dev->layout = render_register_vertexlayout(R, sizeof(va)/sizeof(va[0]), va);
	render_set(R, VERTEXLAYOUT, dev->layout, 0);

	dev->prog = render_shader_create(R, &sia);
	render_shader_bind(R, dev->prog);
	dev->uniform_proj = render_shader_locuniform(R, "ProjMtx");

	dev->ebo = render_buffer_create(R, INDEXBUFFER, NULL, 0, sizeof(uint16_t));
	dev->vbo = render_buffer_create(R, VERTEXBUFFER, NULL, 0, sizeof(struct nk_draw_vertex));
	dev->font_tex = 0;
	dev->vertex_buffer = calloc((size_t)max_vertex_memory, 1);
	dev->element_buffer = calloc((size_t)max_element_memory, 1);
	dev->max_vertex_memory = max_vertex_memory;
	dev->max_element_memory = max_element_memory;
}

NK_INTERN void
nk_ejoy2d_device_upload_atlas(const void *image, int width, int height) {
	struct nk_ejoy2d_device *dev = &ejoy2d.ogl;
	dev->font_tex = render_texture_create(dev->R, width, height, TEXTURE_RGBA8, TEXTURE_2D, 0);
	render_texture_update(dev->R, dev->font_tex, width, height, image, 0, 0);
}

NK_API void
nk_ejoy2d_device_destroy(void) {
	struct nk_ejoy2d_device *dev = &ejoy2d.ogl;
	render_release(dev->R, SHADER, dev->prog);
	render_release(dev->R, VERTEXBUFFER, dev->vbo);
	render_release(dev->R, INDEXBUFFER, dev->ebo);
	render_release(dev->R, TEXTURE, dev->font_tex);
	nk_buffer_free(&dev->cmds);
}

NK_API void
nk_ejoy2d_render(enum nk_anti_aliasing AA) {
	struct nk_ejoy2d_device *dev = &ejoy2d.ogl;
	struct nk_context *ctx = &ejoy2d.ctx;
	float ortho[4][4] = {
		{2.0f, 0.0f, 0.0f, 0.0f},
		{0.0f,-2.0f, 0.0f, 0.0f},
		{0.0f, 0.0f,-1.0f, 0.0f},
		{-1.0f,1.0f, 0.0f, 1.0f},
	};
	ortho[0][0] /= (float)ejoy2d.width;
	ortho[1][1] /= (float)ejoy2d.height;

	/* setup global state */
	render_setblend(dev->R, BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);

// default, so don't care
//	render_setcull(dev->R, CULL_DISABLE);
//	render_setdepth(dev->R, DEPTH_DISABLE);

	/* setup program */
	render_set(dev->R, VERTEXLAYOUT, dev->layout, 0);
	render_set(dev->R, INDEXBUFFER, dev->ebo, 0);
	render_set(dev->R, VERTEXBUFFER, dev->vbo, 0);
	render_shader_bind(dev->R, dev->prog);
	render_shader_setuniform(dev->R, dev->uniform_proj, UNIFORM_FLOAT44, &ortho[0][0]);
	render_enablescissor(dev->R, 1);
	{
		/* convert from command queue into draw list and draw to screen */
		const struct nk_draw_command *cmd;
		int offset = 0;

		/* allocate vertex and element buffer */

		{
			/* fill converting configuration */
			struct nk_convert_config config;
			memset(&config, 0, sizeof(config));
			config.global_alpha = 1.0f;
			config.shape_AA = AA;
			config.line_AA = AA;
			config.circle_segment_count = 22;
			config.curve_segment_count = 22;
			config.arc_segment_count = 22;
			config.null = dev->null;

			{struct nk_buffer vbuf, ebuf;
			nk_buffer_init_fixed(&vbuf, dev->vertex_buffer, (size_t)dev->max_vertex_memory);
			nk_buffer_init_fixed(&ebuf, dev->element_buffer, (size_t)dev->max_element_memory);
			nk_convert(ctx, &dev->cmds, &vbuf, &ebuf, &config);}
		}
		render_buffer_update(dev->R, dev->vbo, dev->vertex_buffer, ctx->draw_list.vertex_count);
		render_buffer_update(dev->R, dev->ebo, dev->element_buffer, ctx->draw_list.element_count);

		/* iterate over and execute each draw command */
		nk_draw_foreach(cmd, ctx, &dev->cmds) {
			if (!cmd->elem_count) continue;
			render_set(dev->R, TEXTURE, cmd->texture.id, 0);
			render_setscissor(dev->R,cmd->clip_rect.x, ejoy2d.height - (cmd->clip_rect.y + cmd->clip_rect.h),
				cmd->clip_rect.w, cmd->clip_rect.h);
			render_draw(dev->R, DRAW_TRIANGLE, offset, cmd->elem_count);
			offset += cmd->elem_count;
		}
		nk_clear(ctx);
	}
	render_enablescissor(dev->R, 0);
}

NK_API struct nk_context*
nk_ejoy2d_init(struct render *R, int w, int h) {
	ejoy2d.width = w;
	ejoy2d.height = h;
	nk_init_default(&ejoy2d.ctx, 0);
	nk_ejoy2d_device_create(R, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
	return &ejoy2d.ctx;
}

NK_API void
nk_ejoy2d_font_stash_begin(struct nk_font_atlas **atlas) {
	nk_font_atlas_init_default(&ejoy2d.atlas);
	nk_font_atlas_begin(&ejoy2d.atlas);
	*atlas = &ejoy2d.atlas;
}

NK_API void
nk_ejoy2d_font_stash_end(void) {
	const void *image; int w, h;
	image = nk_font_atlas_bake(&ejoy2d.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	nk_ejoy2d_device_upload_atlas(image, w, h);
	nk_font_atlas_end(&ejoy2d.atlas, nk_handle_id((int)ejoy2d.ogl.font_tex), &ejoy2d.ogl.null);
	if (ejoy2d.atlas.default_font)
		nk_style_set_font(&ejoy2d.ctx, &ejoy2d.atlas.default_font->handle);
}

NK_API
void nk_ejoy2d_shutdown(void) {
	nk_font_atlas_clear(&ejoy2d.atlas);
	nk_free(&ejoy2d.ctx);
	nk_ejoy2d_device_destroy();
}
