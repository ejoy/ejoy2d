local geo = require "ejoy2d.geometry"
local spr = require "ejoy2d.sprite.c"
local layout = require "ejoy2d.layout"

local gui = { layout = layout }

local widget = {}
local dialog = {} ; dialog.__index = dialog

function dialog:draw(x,y)
	for t,dx,dy,style in self.__layout:draw() do
		widget[t](x+dx,y+dy,style)
	end
end

function widget.panel(x,y,style)
	geo.box(x,y,style.width,style.height,0xe0404040)	-- todo: color
end

function widget.button(x,y,style)
	local w,h = style.width, style.height
	local color = 0xffe0e0e0	-- todo: color
	local bgcolor = 0x80808080
	local border = 1
	local margin = (h - style.font) // 2
	geo.frame(x,y,w,h, color, border)
	geo.box(x+border,y+border,w-border*2, h-border*2, bgcolor)
	geo.scissor(x+border,y+border,w-border*2, h-border*2)
	spr.drawtext(style.title,x,y+margin,w,style.font,color)
	geo.scissor()
end

function widget.label(x,y,style)
	geo.scissor(x,y,style.width, style.height)
	spr.drawtext(style.title,x,y,style.width,style.font,0xffffffff,false,"l")	-- todo: alignment
	geo.scissor()
end

function gui.dialog(desc)
	local tree = layout.dialog(desc)
	tree:refresh()
	return setmetatable({ __layout = tree}, dialog)
end

return gui
