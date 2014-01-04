local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local particle = require "ejoy2d.particle"

pack.load {	pattern = fw.WorkDir..[[examples/asset/?]], "sample", }
particle.preload(fw.WorkDir.."examples/asset/particle")

local ps = particle.new("ps")
local ps2 = particle.new("ps2", function()
	print("particle has gone")
end)

local obj = ej.sprite("sample", "mine")
obj.label.text = "Hello World"

local game = {}

function game.update()
	ps:update(0.0333)
	ps2:update(0.0333)
end

local pos = {x = 160, y = 130}
local pos2 = {x=160, y= 270}

function game.drawframe()
	ps:draw(pos)
	ps2:draw(pos2)
  obj:draw(pos2)
end

function game.touch(what, x, y)
end

ej.start(game)


