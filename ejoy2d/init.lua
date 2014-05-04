local shader = require "ejoy2d.shader"
local fw = require "ejoy2d.framework"

function fw.EJOY2D_INIT()
	shader.init()
end

local ejoy2d = {}

local touch = {
	"BEGIN",
	"END",
	"MOVE",
	"CANCEL"
}

local gesture = {
	"PAN",
	"TAP",
	"PINCH",
    "PRESS",
    "DOUBLE_TAP",
}

function ejoy2d.start(callback)
	fw.EJOY2D_UPDATE = assert(callback.update)
	fw.EJOY2D_DRAWFRAME = assert(callback.drawframe)

	fw.EJOY2D_TOUCH = function(x,y,what,id)
		return callback.touch(touch[what],x,y,id)
	end
    fw.EJOY2D_GESTURE = function(what, x1, y1, x2, y2, state)
		return callback.gesture(gesture[what], x1, y1, x2, y2, state)
	end
	fw.EJOY2D_MESSAGE = assert(callback.message)
  	fw.EJOY2D_HANDLE_ERROR = assert(callback.handle_error)
  	fw.EJOY2D_RESUME = assert(callback.resume)
	fw.inject()
end

function ejoy2d.clear(color)
	return shader.clear(color)
end

return ejoy2d
