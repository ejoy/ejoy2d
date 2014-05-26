local pack = require "ejoy2d.spritepack.c"

--	table or number , texture
--	table data

--	return { texture = maxtex, max = maxid, size = capsize, data = block, export = {} }

-- don't release pack_pool
local pack_pool = {}

local spritepack = {}

local TYPE_PICTURE = assert(pack.TYPE_PICTURE)
local TYPE_ANIMATION = assert(pack.TYPE_ANIMATION)
local TYPE_POLYGON = assert(pack.TYPE_POLYGON)
local TYPE_LABEL = assert(pack.TYPE_LABEL)
local TYPE_PANNEL = assert(pack.TYPE_PANNEL)

local function pack_picture(src, ret)
	table.insert(ret , pack.byte(TYPE_PICTURE))
	local n = #src
	table.insert(ret, pack.byte(n))
	local maxid = 0
	for i=1,n do
		local data = src[i]
		local texid = (data.tex or 1) - 1
		if texid > maxid then
			maxid = texid
		end
		table.insert(ret, pack.byte(texid))
		for j=1,8 do
			table.insert(ret, pack.word(data.src[j]))
		end
		for j=1,8 do
			table.insert(ret, pack.int32(data.screen[j]))
		end
	end

	return pack.picture_size(n) , maxid
end

local function pack_polygon(src, ret)
	table.insert(ret , pack.byte(TYPE_POLYGON))
	local n = #src
	table.insert(ret, pack.byte(n))
	local maxid = 0
	local total_point = 0
	for i=1,n do
		local data = src[i]
		local texid = (data.tex or 1) - 1
		if texid > maxid then
			maxid = texid
		end
		table.insert(ret, pack.byte(texid))
		local pn = #data.src
		total_point = total_point + pn
		assert(pn == #data.screen)
		table.insert(ret, pack.byte(pn/2))
		for j=1,pn do
			table.insert(ret, pack.word(data.src[j]))
		end
		for j=1,pn do
			table.insert(ret, pack.int32(data.screen[j]))
		end
	end

	return pack.polygon_size(n, total_point) , maxid
end

local function is_identity( mat )
	if mat == nil then
		return false
	end
	return mat[1] == 1024 and mat[2] == 0 and mat[3] == 0
			and mat[4] == 1024 and mat[5] == 0 and mat[6] == 0
end

local function pack_part(data, ret)
	if type(data) == "number" then
		table.insert(ret, pack.frametag "i")
		table.insert(ret, pack.word(data))

		return pack.part_size()
	else
		local tag = "i"
		assert(data.index, "frame need an index")
		local mat = data.mat
		if is_identity(mat) then
			mat = nil
		end
		if mat then
			tag = tag .. "m"
		end
		if data.color and data.color ~= 0xffffffff then
			tag = tag .. "c"
		end
		if data.add and data.add ~= 0 then
			tag = tag .. "a"
		end
		if data.touch then
			tag = tag .. "t"
		end
		table.insert(ret, pack.frametag(tag))

		table.insert(ret, pack.word(data.index))
		if mat then
			for i=1,6 do
				table.insert(ret, pack.int32(mat[i]))
			end
		end
		if data.color and data.color ~= 0xffffffff then
			table.insert(ret, pack.color(data.color))
		end
		if data.add and data.add ~= 0 then
			table.insert(ret, pack.color(data.add))
		end
		if data.touch then
			table.insert(ret, pack.word(1))
		end
		return pack.part_size(mat)
	end
end

local function pack_frame(data, ret)
	local size = 0
	table.insert(ret, pack.word(#data))
	for _,v in ipairs(data) do
		local psize = pack_part(v, ret)
		size = size + psize
	end
	return size
end

local function pack_label(data, ret)
	table.insert(ret, pack.byte(TYPE_LABEL))
	table.insert(ret, pack.byte(data.align))
	table.insert(ret, pack.color(data.color))
	table.insert(ret, pack.word(data.size))
	table.insert(ret, pack.word(data.width))
	table.insert(ret, pack.word(data.height))
    table.insert(ret, pack.byte(data.noedge and 0 or 1))
    table.insert(ret, pack.byte(data.space_w or 0))
    table.insert(ret, pack.byte(data.space_h or 0))
    table.insert(ret, pack.byte(data.auto_size or 0))
	return pack.label_size()
end

local function pack_pannel(data, ret)
	table.insert(ret, pack.byte(TYPE_PANNEL))
	table.insert(ret, pack.int32(data.width))
	table.insert(ret, pack.int32(data.height))
	table.insert(ret, pack.byte(data.scissor and 1 or 0))
	return pack.pannel_size()
end

local function pack_animation(data, ret)
	local size = 0
	local max_id = 0
	table.insert(ret , pack.byte(TYPE_ANIMATION))
	local component = assert(data.component)
	table.insert(ret , pack.word(#component))
	for _, v in ipairs(component) do
		if v.id and v.id > max_id then
			max_id = v.id
		end
		if v.id == nil then
			assert(v.name, "Anchor need a name")
			v.id = 0xffff	-- Anchor use id 0xffff
		end
		table.insert(ret, pack.word(v.id))
		table.insert(ret, pack.string(v.name))
		size = size + pack.string_size(v.name)
	end
	table.insert(ret, pack.word(#data))	-- action number
	local frame = 0
	for _, v in ipairs(data) do
		table.insert(ret, pack.string(v.action))
		table.insert(ret, pack.word(#v))
		size = size + pack.string_size(v.action)
		frame = frame + #v
	end
	table.insert(ret, pack.word(frame))	-- total frame
	size = size + pack.animation_size(frame, #component, #data)
	for _, v in ipairs(data) do
		for _, f in ipairs(v) do
			local fsz = pack_frame(f, ret)
			size = size + fsz
		end
	end

	return size, max_id
end

function spritepack.pack( data )
	local ret = { texture = 0, maxid = 0, size = 0 , data = {}, export = {} }
	local ani_maxid = 0

	for _,v in ipairs(data) do
		if v.type ~= "particle" then
			local id = assert(tonumber(v.id))
			if id > ret.maxid then
				ret.maxid = id
			end
			local exportname = v.export
			if exportname then
				assert(ret.export[exportname] == nil, "Duplicate export name"..exportname)
				ret.export[exportname] = id
			end
			table.insert(ret.data, pack.word(id))
			if v.type == "picture" then
				local sz, texid = pack_picture(v, ret.data)
				ret.size = ret.size + sz
				if texid > ret.texture then
					ret.texture = texid
				end
			elseif v.type == "animation" then
				local sz , maxid = pack_animation(v, ret.data)
				ret.size = ret.size + sz
				if maxid > ani_maxid then
					ani_maxid = maxid
				end
			elseif v.type == "polygon" then
				local sz, texid = pack_polygon(v, ret.data)
				ret.size = ret.size + sz
				if texid > ret.texture then
					ret.texture = texid
				end
			elseif v.type == "label" then
				local sz = pack_label(v, ret.data)
				ret.size = ret.size + sz
			elseif v.type == "pannel" then
				local sz = pack_pannel(v, ret.data)
				ret.size = ret.size + sz
			else
				error ("Unknown type " .. tostring(v.type))
			end
		end
	end

	if ani_maxid > ret.maxid then
		error ("Invalid id in animation ".. ani_maxid)
	end

	ret.texture = ret.texture + 1
	ret.data = table.concat(ret.data)
	ret.size = ret.size + pack.pack_size(ret.maxid, ret.texture)
	return ret
end

function spritepack.export(meta)
	local result = { true }
	table.insert(result, pack.word(meta.maxid))
	table.insert(result, pack.word(meta.texture))
	table.insert(result, pack.int32(meta.size))
	table.insert(result, pack.int32(#meta.data))
	local s = 0
	for k,v in pairs(meta.export) do
		table.insert(result, pack.word(v))
		table.insert(result, pack.string(k))
		s = s + 1
	end
	result[1] = pack.word(s)
	table.insert(result, meta.data)
	return table.concat(result)
end

function spritepack.import(data)
	local meta = { export = {} }
	local export_n, off = pack.import_value(data, 1, 'w')
	meta.maxid , off = pack.import_value(data, off, 'w')
	meta.texture , off = pack.import_value(data, off, 'w')
	meta.size , off = pack.import_value(data, off, 'i')
	meta.data_sz , off = pack.import_value(data, off, 'i')
	for i=1, export_n do
		local id, name
		id, off = pack.import_value(data, off, 'w')
		name, off = pack.import_value(data, off, 's')
		meta.export[name] = id
	end
	meta.data = pack.import_value(data, off, 'p')

	return meta
end

function spritepack.init( name, texture, meta )
	assert(pack_pool[name] == nil , string.format("sprite package [%s] is exist", name))
	if type(texture) == "number" then
		assert(meta.texture == 1)
	else
		assert(meta.texture == #texture)
	end
	pack_pool[name] = {
		cobj = pack.import(texture,meta.maxid,meta.size,meta.data, meta.data_sz),
		export = meta.export,
	}
	meta.data = nil

	return pack_pool[name]
end

function spritepack.query( packname, name )
	local p = assert(pack_pool[packname], "Load package first")
	local id
	if type(name) == "number" then
		id = name
	else
		id = p.export[name]
	end
	if not id then
		error(string.format("'%s' is not exist in package %s", name, packname))
	end
	return p.cobj, id
end

return spritepack
