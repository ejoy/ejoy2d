local debug = debug
local ej = require "ejoy2d"
local c = require "ejoy2d.particle.c"
local shader = require "ejoy2d.shader"
local pack = require "ejoy2d.simplepackage"

local particle_configs = {}
local particle_group_configs = {}

local particle = {}

local particle_meta = {__index = {mat = {}, col = {}}}

function particle_meta.__index:update(dt)
	if not self.is_active then
		if self.end_callback ~= nil then
			self.end_callback()
			self.end_callback = nil
		end
		return false
	end

	self:do_update(dt)

	if not self.is_active and self.end_callback ~= nil then
		self.end_callback()
		self.end_callback = nil
	end
	return self.is_active
end

function particle_meta.__index:do_update(dt)
	self.is_active = false
	for _, v in ipairs(self.particles) do
		local active = c.update(v.particle, dt)
		self.is_active = active or self.is_active
	end
end

function particle_meta.__index:data(ptc)
	return c.data(ptc.particle, self.mat, self.col)
end

function particle_meta.__index:draw()
	local cnt = 0
	for _, v in ipairs(self.particles) do
		if self.group:child_visible(v.anchor.name) then
			cnt = self:data(v)
			if cnt > 0 then
				shader.blend(v.src_blend,v.dst_blend)
				v.sprite:matrix_multi_draw(v.anchor.world_matrix, cnt, self.mat, self.col)
				shader.blend()
			end
		end
	end
end

function particle_meta.__index:reset()
	self.is_active = true
	for _, v in ipairs(self.particles) do
		c.reset(v.particle)
	end
end

function particle.preload(config_path)
	-- TODO pack particle data to c
	local meta = dofile(config_path..".lua")
	particle_configs = dofile(config_path.."_particle_config.lua")
	for _, v in ipairs(meta) do
		if v.type == "animation" and v.export ~= nil then
			comp = {}
			for _, c in ipairs(v.component) do
				rawset(comp, #comp+1, c.name)
			end
			rawset(particle_group_configs, v.export, comp)
		end
	end
end

local function new_single(name, anchor)
	local config = rawget(particle_configs, name)
	assert(config ~= nil, "particle not exists:"..name)
	local texid = config.texId
	local cobj = c.new(config)
	anchor.visible = true

	if cobj then
		return {particle = cobj,
			sprite = ej.sprite("particle", texid),
			src_blend = config.blendFuncSource,
			dst_blend = config.blendFuncDestination,
			anchor = anchor,
			is_loop = config.duration < 0,
			name = name
		}
	end
end

function particle.new(name, callback)
	local config = rawget(particle_group_configs, name)
	assert(config ~= nil, "particle group not exists:"..name)

	local group = ej.sprite("particle", name)
	local particles = {}
	local loop = false
	for _, v in ipairs(config) do
		local anchor = group:fetch(v)
		local spr = new_single(v, anchor)
		rawset(particles, #particles+1, spr)
		-- group:mount(v, spr.sprite)
		loop = loop or spr.is_loop
	end
	return debug.setmetatable({group=group,
		is_active = true,
		particles = particles,
		end_callback = callback,
		is_loop = loop,
		}, particle_meta)
end

return particle
