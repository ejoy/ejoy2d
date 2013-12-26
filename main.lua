local ej = require "ejoy2d"
local sprite = require "ejoy2d.sprite"
local pack = require "ejoy2d.simplepackage"
local shader = require "ejoy2d.shader"

pack.load {
	pattern = [[e:\res\hayday\?.sc]],
	"animals",
	"nature_new",
	"hud",
}

local obj2 = ej.sprite("nature_new","crop_wheat")
local obj = ej.sprite("hud","wheel_of_fortune_win")

local game = {}

--obj:fetch("Text").text = "你好"
obj.Text.text = "你好"
--obj:mount("ItemIcon", obj2)
obj.ItemIcon = obj2

function game.update()
	obj.frame = obj.frame + 1
end

local pos = {x = 300, y= 300}
--local texid = pack.texture "animals"
--local text = sprite.label { width = 100, height = 16, text = "你好" }

function game.drawframe()
--[[
	shader.draw(texid, 	{
		637, 342, 621, 342, 621, 332, 637, 332,
		281, -7, 281, 441, 1, 441, 1, -7,
	})
]]
--	text:draw(pos)

	obj:draw(pos)
end

function game.touch(...)
	print(...)
end

ej.start(game)


