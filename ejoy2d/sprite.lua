local debug = debug
local c = require "ejoy2d.sprite.c"
local pack = require "ejoy2d.spritepack"

local method = c.method
method.mount = c.mount
local fetch

local get = c.get
local set = c.set

function sprite_property_get(spr, key)
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

function sprite_property_set(spr, key, v)
	local setter = set[key]
	if setter then
		setter(spr, v)
		return
	end
	error("Unsupport set " .. key)
end

local sprite_meta = {
	__index = sprite_property_get,
	__newindex = sprite_property_set,
}

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