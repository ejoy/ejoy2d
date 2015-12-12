local ej = require "ejoy2d"
local geo = require "ejoy2d.geometry"
local spr = require "ejoy2d.sprite.c"	-- for internal use

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


local style_button = {
	frame = 0xffffffff,
	bgcolor = 0x80404040,
	margin = 6,
	border = 1,
}

local function button(x,y,w,h,text,s)
	local default = style_button
	s = s or default
	local margin = s.margin or default.margin
	local color = s.frame or default.frame
	local bgcolor = s.bgcolor or default.bgcolor
	local charsize = h - margin * 2
	local border = s.border or default.border
	geo.frame(x,y,w,h,color, border)
	geo.box(x+border,y+border,w-border*2, h-border*2, bgcolor)
	geo.scissor(x+border,y+border,w-border*2, h-border*2)
	spr.drawtext(text,x,y+margin,w,charsize,color)
	geo.scissor()
end

local function button_hover(x,y,w,h,text,s)
	local default = style_button
	s = s or default
	local margin = s.margin or default.margin
	local color = s.frame or default.frame
	local bgcolor = s.bgcolor or default.bgcolor
	local charsize = h - margin * 2
	local border = s.border or default.border
	geo.box(x,y,w, h, color)
	geo.frame(x+border,y+border,w-border*2,h-border*2,bgcolor, border)
	spr.drawtext(text,x,y+margin,w,charsize,bgcolor)
end

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
	-- geo.matrix(1.0,0,0,0,  0,1.0,0,0, 0,0,1.0,0, 0.1,0,0,1.0)
	geo.line(0,0,x,y,0xffffffff)
	geo.box(100,100,200,300, 0x80ff0000)
	geo.polygon(hexagon, 0x40ffff00)
	button(400,400,80, 32, "我是一个按钮")
	button_hover(400,440,80, 32, "按钮")
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

