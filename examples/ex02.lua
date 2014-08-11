-- This example show how low level api works

local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local shader = require "ejoy2d.shader"
local ppm = require "ejoy2d.ppm"

local TEXID = 0

-- load ppm/pgm file into texture slot TEXID
ppm.texture(TEXID,fw.WorkDir.."examples/asset/sample.1")

local game = {}

function game.update()
end

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray

	-- use shader.draw to draw a polygon to screen (for debug use)
	shader.draw(TEXID, {
		88, 0, 88, 45, 147, 45, 147, 0,	-- texture coord
		-958, -580, -958, 860, 918, 860, 918, -580, -- screen coord, 16x pixel, (0,0) is the center of screen
	})
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


