local s = require "ejoy2d.shader.c"

local PRECISION = "precision lowp float;\n"

if OS == "LINUX" or OS == "MACOSX" then
	-- some linux and macosx opengl driver can't compile the shader with precision
	PRECISION = ""
end

local sprite_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
uniform sampler2D texture0;
uniform vec3 additive;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
	gl_FragColor.xyz += additive.xyz * tmp.w;
}
]]

local sprite_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;

varying vec2 v_texcoord;
varying vec4 v_color;

void main() {
	gl_Position = position + vec4(-1,1,0,0);
	v_texcoord = texcoord;
	v_color = color;
}
]]

local text_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;

uniform sampler2D texture0;

void main() {
	float c = texture2D(texture0, v_texcoord).w;
	gl_FragColor.xyz = v_color.xyz * c;
	gl_FragColor.w = c;
	gl_FragColor *= v_color.w;
}
]]

local gray_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
uniform sampler2D texture0;
uniform vec3 additive;

void main()
{
	vec4 tmp = texture2D(texture0, v_texcoord);
	vec4 c;
	c.xyz = tmp.xyz * v_color.xyz;
	c.w = tmp.w;
	c *= v_color.w;
	c.xyz += additive.xyz * tmp.w;
	float g = dot(c.rgb , vec3(0.299, 0.587, 0.114));
	gl_FragColor = vec4(g,g,g,c.a);
}
]]

local shader = {}

local shader_name = {
	NORMAL = 0,
	TEXT = 1,
	GRAY = 2,
}

function shader.init()
	s.load(shader_name.NORMAL, PRECISION .. sprite_fs, PRECISION .. sprite_vs)
	s.load(shader_name.TEXT, PRECISION .. text_fs, PRECISION .. sprite_vs)
	s.load(shader_name.GRAY, PRECISION .. gray_fs, PRECISION .. sprite_vs)
end

shader.draw = s.draw
shader.blend = s.blend

function shader.id(name)
	local id = assert(shader_name[name] , "Invalid shader name")
	return id
end

return shader
