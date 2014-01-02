local ej = require "ejoy2d"
local pack = require "ejoy2d.simplepackage"

pack.load {
	pattern = [[examples/asset/?]],
	"sample",
}

local scissor = false
local obj = ej.sprite("sample","mine")
obj.resource.frame = 70
obj.label.text = "Hello World"

local game = {}

function game.update()
	obj.frame = obj.frame + 1
end

local pos = { x = 500, y = 300 }

function game.drawframe()
	obj:draw(pos)
end

function game.touch(what, x, y)
	if what == "END" then
		if obj:test(pos,x,y) then
			scissor = not scissor
			obj.pannel.scissor = scissor
			obj.label.text = scissor and "Set scissor" or "Clear scissor"
		else
			obj.label.text = "Not Hit"
		end
	end
end

ej.start(game)


