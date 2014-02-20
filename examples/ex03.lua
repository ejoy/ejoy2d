local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local particle = require "ejoy2d.particle"

pack.load {	pattern = fw.WorkDir..[[examples/asset/?]], "sample", }
particle.preload(fw.WorkDir.."examples/asset/particle")

local ps = particle.new("fire", function()
	print("particle has gone")
end)
ps.group:ps(160, 240)

local obj = ej.sprite("sample", "mine")
obj.label.text = "Hello World"

local game = {}

function game.update()
	ps:update(0.0333)
  ps.group.frame = ps.group.frame + 1
end

local pos = {x=160, y= 300}
local pos2 = {x=160, y = 240}

function game.drawframe()
  ps.group:draw()
	ps:draw()
  obj:draw(pos)
end

function game.touch(what, x, y)
end

ej.start(game)


