local ej = require "ejoy2d"
local geo = require "ejoy2d.geometry"

local game = {}

local x = 0
local y = 768

function game.update()
	x = x + 1
	y = y - 1
end

local hexagon = {}

for i = 0, 5 do
	local r = math.pi * 2 * i / 6
	table.insert(hexagon, math.sin(r) * 100 + 300)
	table.insert(hexagon, math.cos(r) * 100 + 300)
end


function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
	geo.line(0,0,x,y,0xffffffff)
	geo.box(100,100,200,300, 0x80ff0000)
	geo.polygon(hexagon, 0x40ffff00)
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

