local ej = require "ejoy2d"
local gui = require "ejoy2d.gui"

local game = {}

function game.update()
end

local layout = gui.layout
local dialog = gui.dialog {
	font = 20, margin = 4, size = 233, gap = 3,
	layout.vbox {
		layout.label { expand = true , title = "我是一行字" },
		layout.hbox {
			layout.fill {},
			layout.button { id = "ok", size = 100, title = "OK" },
			layout.button { id = "cancel", size = 100 , title = "CANCEL" },
			layout.fill {},
		},
	}
}

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
	dialog:draw(100,100)
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

