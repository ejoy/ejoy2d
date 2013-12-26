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
}

function ejoy2d.start(callback)
	fw.EJOY2D_UPDATE = assert(callback.update)
	fw.EJOY2D_DRAWFRAME = assert(callback.drawframe)
	fw.EJOY2D_TOUCH = function(x,y,what,id)
		return callback.touch(x,y,touch[what],id)
	end
	fw.inject()
end

return ejoy2d
