#ifndef ejoy2d_material_h
#define ejoy2d_material_h

// the implemention is in shader.c

struct material;
struct render;

int material_size(int prog);
struct material * material_init(void *self, int size, int prog);
void material_apply(int prog, struct material *);
int material_setuniform(struct material *, int index, int n, const float *v);
int material_settexture(struct material *, int channel, int texture);
// todo: change alpha blender mode, change attrib layout, etc.

#endif
