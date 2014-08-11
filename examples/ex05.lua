local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local sprite = require "ejoy2d.sprite"

pack.load {
	pattern = fw.WorkDir..[[examples/asset/?]],
	"sample",
}

local obj = ej.sprite("sample","cannon")
local turret = obj.turret

local game = {}
local screencoord = { x = 512, y = 384, scale = 1.2 }

local label = sprite.label { width = 100, height = 100, size = 30, text = "Hello" , color = 0xffffffff }

-- you can also mount the label to turret.anchor
-- turret.anchor = label
local anchor = turret.anchor
anchor.visible = true

function game.update()
	turret.frame = turret.frame + 1
end

function game.drawframe()
	ej.clear()
	obj:draw(screencoord)
	-- If anchor is visible, obj:draw will update anchor's world_matrix
	label.matrix = anchor.world_matrix
	label:draw()
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


