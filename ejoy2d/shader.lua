local s = require "ejoy2d.shader.c"

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
	gl_FragColor = v_color * texture2D(texture0, v_texcoord).w;
}
]]

local shader = {}

function shader.init()
	s.load(0, sprite_fs, sprite_vs)
	s.load(1, text_fs, sprite_vs)
end

shader.draw = s.draw
shader.blend = s.blend

return shader
