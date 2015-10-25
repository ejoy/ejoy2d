local layout = {}
local dialog = {} ; dialog.__index = dialog

local DEFAULT_FONT = 20
local BUTTON_MARGIN = 6
local LABEL_MARGIN = 4

local function collect_id(root)
	local id = {}
	local function collect(obj)
		local name = obj.style.id
		if name then
			if id[name] then
				error ("Duplicate id : " .. name)
			end
			id[name] = obj
		end
		if obj.children then
			for _,v in ipairs(obj.children) do
				collect(v)
			end
		end
	end
	collect(root)
	return id
end

function layout.dialog(C)
	local root = layout.panel(C)
	return setmetatable ({
		__id = collect_id(root),
		__dialog = root,
	}, dialog)
end

local function copy_children(from, to)
	local children = to.children
	for k,v in ipairs(from) do
		assert(v.type)
		children[k] = v
		v.parent = to
	end
end

local function copy_attrib(from, to , attrib, ...)
	if attrib then
		to["__" .. attrib] = from[attrib]
		return copy_attrib(from, to, ...)
	end
end

local function attribs()
	return "font", "margin", "gap", "size", "expand"
end

local function copy_style(from, to , s, ...)
	if s then
		to[s] = from[s]
		return copy_style(from, to, ...)
	else
		return to
	end
end

local function copy_styles(C, ...)
	return copy_style(C, {}, ...)
end

function layout.panel(C)
	assert(C[1].type)
	local obj = { type = "panel" , children = { C[1] } , widget = true, container = true, style = copy_styles(C, "id") }
	C[1].parent = obj
	copy_attrib(C, obj, attribs())
	return obj
end

function layout.button(C)
	local obj = { type = "button" , widget = true , style = copy_styles(C, "id", "title") }
	copy_attrib(C, obj, "font", "size", "expand")
	return obj
end

function layout.label(C)
	local obj = { type = "label" , widget = true , style = copy_styles(C, "id", "title") }
	copy_attrib(C, obj, "font", "size", "expand")
	return obj
end

function layout.fill(C)
	return { type = "fill" , container = true, style = {}}
end

function layout.hbox(C)
	local obj = { type = "hbox" , children = {} , container = true, style = {}}
	copy_children(C, obj)
	copy_attrib(C, obj, attribs())
	return obj
end

function layout.vbox(C)
	local obj = { type = "vbox" , children = {} , container = true, style = {}}
	copy_children(C, obj)
	copy_attrib(C, obj, attribs())
	return obj
end

------------

function dialog:dump()
	print("ID:")
	local temp = {}
	for k in pairs(self.__id) do
		table.insert(temp, k)
	end
	print(table.concat(temp, ","))
	local function dump_tree(root, level)
		local name
		local indent = string.rep("  ",level)
		local attrib = {}
		for k, v in pairs(root) do
			if k:sub(1,2) == "__" then
				table.insert(attrib, string.format("%s:%s", k:sub(3), v))
			end
		end
		local function attribs()
			return table.concat(attrib,", "),
				root.x,root.y,
				root.style.width,
				root.style.height
		end
		if root.style.id then
			name = string.format("%s%s(%s) %s (%d:%d-%dx%d)", indent,root.type, root.style.id, attribs())
		else
			name = string.format("%s%s %s (%d:%d-%dx%d)", indent,root.type, attribs())
		end
		print(name)
		if root.children then
			level = level + 1
			for _, v in ipairs(root.children) do
				dump_tree(v, level)
			end
		end
	end
	dump_tree(self.__dialog,0)
end

local function get_size(size)
	if size then
		local x,y = tostring(size):match "(%d*)x?(%d*)"
		if not x then
			error( "Invalid size " .. size)
		end
		if x == "" then
			x = 0
		else
			x = tonumber(x)
		end
		if y == "" then
			y = 0
		else
			y = tonumber(y)
		end
		return x,y
	end
	return 0,0
end

local function get_attrib(root, attrib)
	if root then
		if root[attrib] ~= nil then
			return root[attrib]
		else
			return get_attrib(root.parent, attrib)
		end
	end
end

local nsize = {}	-- natural size functions

local function calc_nsize(root)
	local w,h = nsize[root.type](root)
	local style = root.style
	style.width = w
	style.height = h
	return w,h
end

function nsize.panel(root)
	local ux,uy = get_size(root.__size)
	local margin = get_attrib(root, "__margin") or 0
	margin = margin * 2
	local nx,ny = calc_nsize(root.children[1])
	nx = nx + margin
	ny = ny + margin
	ux = (ux > nx) and ux or nx
	uy = (uy > ny) and uy or ny
	return ux , uy
end

function nsize.fill(root)
	return 0,0
end

function nsize.button(root)
	local ux,uy = get_size(root.__size)
	local font = get_attrib(root, "__font") or DEFAULT_FONT
	root.style.font = font
	if uy == 0 then
		uy = font
		uy = uy + BUTTON_MARGIN
	end
	return ux,uy
end

function nsize.label(root)
	local ux,uy = get_size(root.__size)
	local font = get_attrib(root, "__font") or DEFAULT_FONT
	root.style.font = font
	if uy == 0 then
		uy = font
		uy = uy + LABEL_MARGIN
	end
	return ux,uy
end

function nsize.hbox(root)
	local ux, uy = get_size(root.__size)
	local c = root.children
	local n = #c
	local nx = 0
	local ny = 0
	for i=1,n do
		local w,h = calc_nsize(c[i])
		nx = nx + w
		if h > ny then
			ny = h
		end
	end
	if n > 1 then
		nx = nx + (get_attrib(root, "__gap") or 0) * (n-1)
	end
	ux = (ux > nx) and ux or nx
	uy = (uy > ny) and uy or ny
	return ux,uy
end

function nsize.vbox(root)
	local ux, uy = get_size(root.__size)
	local c = root.children
	local n = #c
	local nx = 0
	local ny = 0
	for i=1,n do
		local w,h = calc_nsize(c[i])
		ny = ny + h
		if w > nx then
			nx = w
		end
	end
	if n > 1 then
		ny = ny + (get_attrib(root, "__gap") or 0) * (n-1)
	end
	ux = (ux > nx) and ux or nx
	uy = (uy > ny) and uy or ny
	return ux,uy
end

local spacing = {}	-- spacing functions

local function calc_spacing(root)
	if root.children then
		return spacing[root.type](root)
	end
end

local function expand(root)
	if root.container then
		return get_attrib(root, "__expand") or true
	else
		return root.__expand
	end
end

function spacing.panel(root)
	local c = root.children[1]
	local margin = get_attrib(root, "__margin") or 0
	local style = c.style
	local root_style = root.style
	if expand(c) then
		style.width = root_style.width - margin * 2
		style.height = root_style.height - margin * 2
	end
	c.x = margin
	c.y = margin
	return calc_spacing(c)
end

local function child_expand(c, e)
	if c.container then
		return e
	else
		return c.__expand
	end
end

function spacing.hbox(root)
	local c = root.children
	local n = #c
	local expand_n = 0
	local e = expand(root)
	local w = 0
	local height = root.style.height
	for i=1,n do
		local child = c[i]
		if child_expand(child, e) then
			expand_n = expand_n + 1
		end
		w = w + child.style.width
	end
	local gap = get_attrib(root, "__gap") or 0
	if expand_n > 0 then
		local spacing = root.style.width - w - gap * (n-1)
		for i=1,n do
			local child = c[i]
			if child_expand(child, e) then
				local expand_spacing = spacing // expand_n
				spacing = spacing - expand_spacing
				expand_n = expand_n - 1
				local style = child.style
				style.width = style.width + expand_spacing
				style.height = height
			end
		end
	end
	w = 0
	for i=1, n do
		local child = c[i]
		local style = child.style
		child.x = w
		child.y = 0
		calc_spacing(child)
		w = w + style.width + gap
	end
end

function spacing.vbox(root)
	local c = root.children
	local n = #c
	local expand_n = 0
	local e = expand(root)
	local h = 0
	local width = root.style.width
	for i=1,n do
		local child = c[i]
		if child_expand(child, e) then
			expand_n = expand_n + 1
		end
		h = h + child.style.height
	end
	local gap = get_attrib(root, "__gap") or 0
	if expand_n > 0 then
		local spacing = root.style.height - h - gap * (n-1)
		for i=1,n do
			local child = c[i]
			if child_expand(child, e) then
				local expand_spacing = spacing // expand_n
				spacing = spacing - expand_spacing
				expand_n = expand_n - 1
				local style = child.style
				style.height = style.height + expand_spacing
				style.width = width
			end
		end
	end
	h = 0
	for i=1, n do
		local child = c[i]
		local style = child.style
		child.x = 0
		child.y = h
		calc_spacing(child)
		h = h + style.height + gap
	end
end

function dialog:refresh()
	local root = self.__dialog
	calc_nsize(root)
	calc_spacing(root)
	root.x = 0
	root.y = 0
end

local function iter(root, x, y)
	local style = root.style
	x = x + root.x
	y = y + root.y
	if root.widget then
		coroutine.yield(root.type,x,y,style)
	end
	if root.children then
		for _,v in ipairs(root.children) do
			iter(v, x, y)
		end
	end
end

function dialog:draw()
	return coroutine.wrap(function()
		return iter(self.__dialog, 0,0)
	end)
end

return layout
