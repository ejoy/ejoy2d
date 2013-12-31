local debug = debug
local ej = require "ejoy2d"
local c = require "ejoy2d.particle.c"
local shader = require "ejoy2d.shader"
local pack = require "ejoy2d.simplepackage"

local particle_configs = {}

local particle = {}

local particle_meta = {__index = {mat = {}, col = {}}}

function particle_meta.__index:update(dt, x, y)
	c.update(self.particle, dt, x, y)
end

function particle_meta.__index:data()
	return c.data(self.particle, self.mat, self.col)
end

function particle_meta.__index:draw(pos)
	local cnt = self:data()

	shader.blend(self.src_blend,self.dst_blend)
	self.sprite:multi_draw(pos, cnt, self.mat, self.col)
	shader.blend()
end

function particle.preload(config_path)
	local meta = dofile(config_path..".lua")
	for _, v in ipairs(meta) do
		if v.type == "particle" and v.export ~= nil then
			rawset(particle_configs, v.export, v)
		end
	end
end

function particle.new(name)
	local config = rawget(particle_configs, name)
	assert(config ~= null, "Cannot found particle:"..name)
	assert(config.data ~= null, "Invalid particle config:"..name)
	local texid = config.component[1]["id"]
	local cobj = c.new(config.data)

	if cobj then
		return debug.setmetatable({particle = cobj,
			sprite = ej.sprite("particle", texid),
			src_blend = config.data.blendFuncSource,
			dst_blend = config.data.blendFuncDestination},
			particle_meta)
	end
end

return particle
