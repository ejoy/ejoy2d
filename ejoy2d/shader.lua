local s = require "ejoy2d.shader.c"

local PRECISION = ""
local PRECISION_HIGH = ""

if s.version() == 2 then
	-- Opengl ES 2.0 need float precision specifiers
	PRECISION = "precision lowp float;\n"
	PRECISION_HIGH = "precision highp float;\n"
end

local sprite_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;
uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
	gl_FragColor.xyz += v_additive.xyz * tmp.w;
}
]]

local sprite_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;
attribute vec4 additive;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

void main() {
	gl_Position = position + vec4(-1.0,1.0,0,0);
	v_texcoord = texcoord;
	v_color = color;
	v_additive = additive;
}
]]

local text_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main() {
	float c = texture2D(texture0, v_texcoord).w;
	float alpha = clamp(c, 0.0, 0.5) * 2.0;

	gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * alpha;
	gl_FragColor.w = alpha;
	gl_FragColor *= v_color.w;
}
]]

local text_edge_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main() {
	float c = texture2D(texture0, v_texcoord).w;
	float alpha = clamp(c, 0.0, 0.5) * 2.0;
	float color = (clamp(c, 0.5, 1.0) - 0.5) * 2.0;

	gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * color;
	gl_FragColor.w = alpha;
	gl_FragColor *= v_color.w;
}
]]

local gray_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main()
{
	vec4 tmp = texture2D(texture0, v_texcoord);
	vec4 c;
	c.xyz = tmp.xyz * v_color.xyz;
	c.w = tmp.w;
	c *= v_color.w;
	c.xyz += v_additive.xyz * tmp.w;
	float g = dot(c.rgb , vec3(0.299, 0.587, 0.114));
	gl_FragColor = vec4(g,g,g,c.a);
}
]]

local color_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main()
{
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = v_color.xyz * tmp.w;
	gl_FragColor.w = tmp.w;
}
]]


local blend_fs = [[
varying vec2 v_texcoord;
varying vec2 v_mask_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
	gl_FragColor.xyz += v_additive.xyz * tmp.w;

	vec4 m = texture2D(texture0, v_mask_texcoord);
	gl_FragColor.xyz *= m.xyz;
//	gl_FragColor *= m.w;
}
]]


local blend_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;
attribute vec4 additive;

varying vec2 v_texcoord;
varying vec2 v_mask_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform vec2 mask;

void main() {
	gl_Position = position + vec4(-1,1,0,0);
	v_texcoord = texcoord;
	v_mask_texcoord = texcoord + mask;
	v_color = color;
    v_additive = additive;
}
]]

local renderbuffer_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
}
]]

local renderbuffer_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;

varying vec2 v_texcoord;
varying vec4 v_color;

uniform vec4 st;

void main() {
	gl_Position.x = position.x * st.x + st.z -1.0;
	gl_Position.y = position.y * st.y + st.w +1.0;
	gl_Position.z = position.z;
	gl_Position.w = position.w;
	v_texcoord = texcoord;
	v_color = color;
}
]]

local shader = {}

local shader_name = {
	NORMAL = 0,
	RENDERBUFFER = 1,
	TEXT = 2,
	EDGE = 3,
	GRAY = 4,
	COLOR = 5,
	BLEND = 6,
}

function shader.init()
	s.load(shader_name.NORMAL, PRECISION .. sprite_fs, PRECISION .. sprite_vs)
	s.load(shader_name.TEXT, PRECISION .. text_fs, PRECISION .. sprite_vs)
	s.load(shader_name.EDGE, PRECISION .. text_edge_fs, PRECISION .. sprite_vs)
	s.load(shader_name.GRAY, PRECISION .. gray_fs, PRECISION .. sprite_vs)
	s.load(shader_name.COLOR, PRECISION .. color_fs, PRECISION .. sprite_vs)
	s.load(shader_name.BLEND, PRECISION .. blend_fs, PRECISION .. blend_vs)
	s.load(shader_name.RENDERBUFFER, PRECISION .. renderbuffer_fs, PRECISION_HIGH .. renderbuffer_vs)
end

shader.draw = s.draw
shader.blend = s.blend
shader.clear = s.clear

function shader.id(name)
	local id = assert(shader_name[name] , "Invalid shader name " .. name)
	return id
end

-- user defined shader (or replace default shader)

local MAX_PROGRAM = 16
local USER_PROGRAM = 7

local uniform_format = {
	float = 1,
	float2 = 2,
	float3 = 3,
	float4 = 4,
	matrix33 = 5,
	matrix44 = 6,
}

local uniform_set = s.uniform_set

local function create_shader(id, uniform)
	if uniform then
		local s = {}
		for index , u in ipairs(uniform) do
			local loc = index-1
			local format = u.type
			s[u.name] = function(...)
				uniform_set(id, loc, format, ...)
			end
		end
		return s
	end
end

function shader.define( arg )
	local name = assert(arg.name)
	local id = shader_name[name]
	if id == nil then
		assert(USER_PROGRAM < MAX_PROGRAM)
		id = USER_PROGRAM
		USER_PROGRAM = id + 1
	end

	local vs = PRECISION .. (arg.vs or sprite_vs)
	local fs = PRECISION_HIGH .. (arg.fs or sprite_fs)

	s.load(id, fs, vs)

	local uniform = arg.uniform
	if uniform then
		for _,v in ipairs(uniform) do
			v.type = assert(uniform_format[v.type])
		end
		s.uniform_bind(id, uniform)
	end

	local r = create_shader(id, uniform)
	shader_name[name] = id
	return r
end

return shader
