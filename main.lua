local ej = require "ejoy2d"
local pack = require "ejoy2d.simplepackage"

pack.load {
	pattern = [[e:\res\hayday\?.sc]],
--	"animals",
	"background",
	"nature_new",
}

local obj = ej.sprite("nature_new","crop_wheat")

local game = {}

function game.update()
	obj.frame = obj.frame + 1
end

local pos = {x = 300, y= 300}

function game.drawframe()
	obj:draw(pos)
end

ej.start(game)


