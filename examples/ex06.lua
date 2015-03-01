local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local sprite = require "ejoy2d.sprite"

pack.load {
	pattern = fw.WorkDir..[[examples/asset/?]],
	"sample",
}

local obj = ej.sprite("sample","cannon")
local obj2 = ej.sprite("sample","cannon")
obj2:ps(100,0)

local proxy = sprite.proxy()
local turret = obj.turret
proxy.proxy = turret -- proxy:mount("proxy", turret)

-- multi mount proxy
obj.turret = proxy
obj2.turret = proxy

assert(proxy.parent == nil)
assert(proxy.name == nil)

local game = {}
local screencoord = { x = 512, y = 384, scale = 1.2 }

function game.update()
	turret.frame = turret.frame + 1
end

function game.drawframe()
	ej.clear()
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


