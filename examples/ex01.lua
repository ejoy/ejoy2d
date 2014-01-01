local ej = require "ejoy2d"
local pack = require "ejoy2d.simplepackage"

pack.load {
	pattern = [[examples/asset/?]],
	"sample",
}

local obj = ej.sprite("sample","cannon")
local turret = obj.turret

local obj2 = ej.sprite("sample","mine")
obj2.resource.frame = 70
obj2.label.text = "Hello World"

local game = {}

function game.update()
	turret.frame = turret.frame + 3
	obj2.frame = obj2.frame + 1
end

local pos = {x = 300, y= 300, scale = 0.8}
local pos2 = { x = 500, y = 300 }

function game.drawframe()
	obj:draw(pos)
	obj2:draw(pos2)
end

function game.touch(what, x, y)
end

ej.start(game)


