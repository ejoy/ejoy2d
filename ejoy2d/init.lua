local shader = require "ejoy2d.shader"
local fw = require "ejoy2d.framework"

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
precision lowp float;

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
precision lowp float;
varying vec2 v_texcoord;
varying vec4 v_color;

uniform sampler2D texture0;

void main() {
	gl_FragColor = v_color * texture2D(texture0, v_texcoord).w;
}
]]

function fw.EJOY2D_INIT()
	shader.load(0, sprite_fs, sprite_vs)
	shader.load(1, text_fs, sprite_vs)
end

function fw.EJOY2D_UPDATE()
end

function fw.EJOY2D_DRAWFRAME()
end

local ejoy2d = {}

function ejoy2d.start(callback)
	fw.EJOY2D_UPDATE = assert(callback.update)
	fw.EJOY2D_DRAWFRAME = assert(callback.drawframe)
	fw.inject()
end

return ejoy2d
