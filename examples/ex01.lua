local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"

pack.load {
	pattern = fw.WorkDir..[[examples/asset/?]],
	"sample",
}

local obj = ej.sprite("sample","cannon")
local turret = obj.turret

-- set position (-100,0) scale (0.5)
obj:ps(-100,0,0.5)

local obj2 = ej.sprite("sample","mine")
obj2.resource.frame = 70
-- set position(100,0) scale(1.2) separately
obj2:ps(100,0)
obj2:ps(1.2)

local game = {}
local screencoord = { x = 512, y = 384, scale = 1.2 }
local x1,y1,x2,y2 = obj2:aabb(screencoord)
obj2.label.text = string.format("AABB\n%d x %d", x2-x1, y2-y1)

function game.update()
	turret.frame = turret.frame + 3
	obj2.frame = obj2.frame + 1
end

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
	obj:draw(screencoord)
	obj2:draw(screencoord)
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


