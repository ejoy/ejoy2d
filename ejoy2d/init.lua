local shader = require "ejoy2d.shader"
local fw = require "ejoy2d.framework"

function fw.EJOY2D_INIT()
	shader.init()
end

function fw.EJOY2D_UPDATE()
end

function fw.EJOY2D_DRAWFRAME()
end

local ejoy2d = {}

function ejoy2d.start(callback)
	fw.EJOY2D_UPDATE = assert(callback.update)
	fw.EJOY2D_DRAWFRAME = assert(callback.drawframe)
	fw.inject()
end

return ejoy2d
