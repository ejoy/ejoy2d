local debug = debug
local c = require "ejoy2d.sprite.c"
local pack = require "ejoy2d.spritepack"

local method = c.method
method.mount = c.mount
local fetch

local get = c.get
local set = c.set

local sprite_meta = {}

function sprite_meta.__index(spr, key)
	if method[key] then
		return method[key]
	end
	local getter = get[key]
	if getter then
		return getter(spr)
	end
	local child = fetch(spr, key)
	if child then
		return child
	else
		error("Unsupport get " ..  key)
	end
end

function sprite_meta.__newindex(spr, key, v)
	local setter = set[key]
	if setter then
		setter(spr, v)
		return
	end
	assert(debug.getmetatable(v) == sprite_meta, "Need a sprite")
	c.mount(spr, key, v)
end

-- local function
function fetch(spr, child)
	local cobj = c.fetch(spr, child)
	if cobj then
		return debug.setmetatable(cobj, sprite_meta)
	end
end

method.fetch = fetch

local sprite = {}

function sprite.new(packname, name)
	local pack, id = pack.query(packname, name)
	local cobj = c.new(pack,id)
	if cobj then
		return debug.setmetatable(cobj, sprite_meta)
	end
end

function sprite.label(tbl)
	local size = tbl.size or tbl.height - 2
	local l = (c.label(tbl.width, tbl.height, size, tbl.color, tbl.align))
	if l then
		l = debug.setmetatable(l, sprite_meta)
		if tbl.text then
			l.name = tbl.text
		end
		return l
	end
end

return sprite