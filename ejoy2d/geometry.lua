local shader = require "ejoy2d.shader"
local core = require "ejoy2d.geometry.c"

-- define a shader
local s = shader.define {
	name = "GEOMETRY",
	vs = [[
attribute vec4 position;
attribute vec4 color;

varying vec4 v_color;

uniform mat4 inv_pmv;

void main() {
	gl_Position = position + vec4(-1.0,1.0,0,0);
	gl_Position = inv_pmv * gl_Position;
	v_color = color;
}
	]],
	fs = [[
varying vec4 v_color;

void main() {
	gl_FragColor = v_color;
}
	]],
	uniform={
		{
			name="inv_pmv",
			type="matrix44"
		},
	},
}

local id = shader.id("GEOMETRY")
core:setprogram(id)
if core.__obj then
	core.material = debug.setmetatable(core, shader.material_meta(id))
	core.material:inv_pmv(1.0,0,0,0,  0,1.0,0,0, 0,0,1.0,0, 0,0,0,1.0)
else
	core.material = nil
end

local function set_mat(...)
	if core.material then
		core.material:inv_pmv(...)
	end
end

local geo = {}

geo.line = assert(core.line)
geo.box = assert(core.box)
geo.polygon = assert(core.polygon)
geo.frame = assert(core.frame)
geo.scissor = assert(core.scissor)
geo.matrix = set_mat

return geo
