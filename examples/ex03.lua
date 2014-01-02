local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local particle = require "ejoy2d.particle"

pack.load {	pattern = fw.WorkDir..[[examples/asset/?]] }
particle.preload(fw.WorkDir.."examples/asset/particle")

local ps = particle.new("ps")
local ps2 = particle.new("ps2", function()
	print("---------------")
end)

local game = {}

function game.update()
	-- ps:update(0.0333)
	ps2:update(0.0333)
end

local pos = {x = 160, y = 130}
local pos2 = {x=100, y= 30}

function game.drawframe()
	-- ps:draw(pos)
	ps2:draw(pos)
end

function game.touch(what, x, y)
end

ej.start(game)


