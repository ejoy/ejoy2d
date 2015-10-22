local shader = require "ejoy2d.shader"
local core = require "ejoy2d.geometry.c"

-- define a shader
local s = shader.define {
	name = "GEOMETRY",
	vs = [[
attribute vec4 position;
attribute vec4 color;

varying vec4 v_color;

void main() {
	gl_Position = position + vec4(-1.0,1.0,0,0);
	v_color = color;
}
	]],
	fs = [[
varying vec4 v_color;

void main() {
	gl_FragColor = v_color;
}
	]],
}

core.setprogram(shader.id "GEOMETRY")

local geo = {}

geo.line = assert(core.line)
geo.box = assert(core.box)
geo.polygon = assert(core.polygon)
geo.frame = assert(core.frame)
geo.scissor = assert(core.scissor)

return geo
