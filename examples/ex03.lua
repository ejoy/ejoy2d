local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local particle = require "ejoy2d.particle"

fw.AnimationFramePerFrame = 1

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
  ej.clear()	-- default clear color is black (0,0,0,1)
  ps.group:draw()
  obj:draw(pos)
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


