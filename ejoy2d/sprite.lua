local c = require "ejoy2d.sprite.c"
local pack = require "ejoy2d.spritepack"

local method = c.method
method.fetch = c.fetch
method.mount = c.mount

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
	error("Unsupport get " ..  key)
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

local function sprite_new(packname, name)
	local pack, id = pack.query(packname, name)
	local cobj = c.new(pack,id)
	if cobj then
		return debug.setmetatable(cobj, sprite_meta)
	end
end

return sprite_new