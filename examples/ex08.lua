-- This example show how user defined shader works

local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"

pack.load {
	pattern = fw.WorkDir..[[examples/asset/?]],
	"sample",
}


-- define a shader
ej.define_shader {
	name = "EXAMPLE",
	fs = [[
uniform vec4 color;

void main() {
	gl_FragColor = color;
}
	]],
	uniform = {
		{
			name = "color",
			type = "float4",
		}
	}
}


local obj = ej.sprite("sample","cannon")
obj:ps(100,100)

obj.program = "EXAMPLE"
local material = obj.material
material:color(1,0,0,1)

local obj2 = ej.sprite("sample","cannon")
obj2:ps(200,100)

obj2.turret.program = "EXAMPLE"
obj2.turret.material:color(0,1,0,1)

local game = {}

function game.update()
end

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
	obj:draw()
	obj2:draw()
end

function game.touch(what, x, y)
end

function game.message(...)
end

function game.handle_error(...)
end

function game.on_resume()
end

function game.on_pause()
end

ej.start(game)

