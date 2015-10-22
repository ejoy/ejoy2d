

--------------------helpers--------------------
function string.char_len(str)
  local _, len = string.gsub(str, "[^\128-\193]", "")
  return len
end

function string.plain_format(str)
  return string.gsub(str, "(#%[%w+%])", "")
end

local function is_ASCII_DBC_punct(unicode)
	if not unicode then return false end
	-- ! to /
	if unicode >= 33 and unicode <= 47 then return true end
	-- : to @
	if unicode >= 58 and unicode <= 64 then return true end
	-- [ to `
	if unicode >= 91 and unicode <= 96 then return true end
	-- { to ~
	if unicode >= 123 and unicode <= 126 then return true end

	return false
end

local function is_ASCII_SBC_punct(unicode)
	return is_ASCII_DBC_punct(unicode-65248)
end

local function is_JP_punct(unicode)
	if not unicode then return false end
	if unicode >= 65377 and unicode <= 65381 then return true end
	if unicode == 0x3002 or unicode == 0x3001 or unicode == 0x300C or
		unicode == 0x300D or unicode == 0x30FB then
		return true
	end
	return false
end

local function is_punct(unicode)
	if not unicode then return false end
	if is_ASCII_DBC_punct(unicode) then return true end
	if is_ASCII_SBC_punct(unicode) then return true end
	if is_JP_punct(unicode) then return true end
	return false
end

local function is_ascii(unicode)
	if not unicode then return false end
	if unicode >= 33 and unicode <= 126 then return true end
	local shift = unicode-65248
	if shift >= 33 and shift <= 126 then return true end
	return false
end

local function is_alnum(unicode)
	if not unicode then return false end
	return is_ascii(unicode) and not is_punct(unicode)
end

local char_sizes = {}
local function char_width(idx)
	return char_sizes[idx]
end
local function char_height(idx)
	return char_sizes[idx+1]
end
local function char_unicode_len(idx)
	return char_sizes[idx+2]
end
local function char_unicode(idx)
	return char_sizes[idx+3]
end

----------------ends of helpers-----------------

local gap = 4

local CTL_CODE_POP=0
local CTL_CODE_COLOR=1
local CTL_CODE_LINEFEED=2

local operates = {
	yellow	={val=0xFFFFFF00,type="color"},
	red			={val=0xFFFF0000,type="color"},
	blue		={val=0xFF0000FF,type="color"},
	green		={val=0xFF00FF00,type="color"},
	stop			={type=CTL_CODE_POP},
	lf 			={type=CTL_CODE_LINEFEED},
}
local function init_operates(ops)
	for k, v in pairs(ops) do
		operates[k] = v
	end
end

local function is_rich(txt)
	local _, cnt = string.plain_format(txt)
	return cnt > 0
end

local function _locate_alnum(sizes, anchor)
	local start = anchor
	local forward_len = sizes[start]
	while sizes[start-gap+3] do
		if start-gap < 3 or not is_alnum(sizes[start-gap+3]) then
			break
		end
		start = start - gap
		forward_len = forward_len + sizes[start]
	end

	local stop = anchor
	local backward_len = 0
	while sizes[stop+gap+3] do
		if not is_alnum(sizes[stop+gap+3]) then
			break
		end
		stop = stop + gap
		backward_len = backward_len + sizes[stop]
	end
	return forward_len, start, stop, backward_len
end

local function _add_linefeed(fields, pos, offset)
	local field = {false, false, false, false}
	field[1] = pos-1  --zero base index
	field[2] = pos-1
	field[3] = CTL_CODE_LINEFEED
	field[4] = math.tointeger(offset) or 1000
	-- field[4] = 1000
	table.insert(fields, field)
end

local function _layout(label, txt, fields)
	-- print(txt)
	--{label_width, label_height, char_width_1, char_width_1, unicode_len_1, unicode_1...}
	local size_cnt = label:char_size(char_sizes, txt)

	local width = char_sizes[1]

	local line_width = 0
	local char_idx = 1
	local ignore_next, extra_len = 0, 0
	local max_width, max_height, line_max_height=0, 0, 0
	for i=3, size_cnt, gap do
		local idx = i
		if ignore_next == 0 then
			line_width = line_width + char_width(idx)
		else
			ignore_next = ignore_next-1
		end
		if extra_len > 0 then
			line_width = line_width + extra_len
			extra_len=0
		end
		--reset if \n
		if char_unicode(idx) == 10 then
			max_height = max_height + char_height(idx)
			line_width = 0
		end

		line_max_height = char_height(idx) > line_max_height and char_height(idx) or line_max_height
		char_idx = char_idx + char_unicode_len(idx)

		if line_width >= width then
			--line width equalization
			-- if not is_alnum(char_unicode(idx)) and
			-- 		line_width - width > width - line_width + char_width(idx) and
			-- 		idx - gap > 3 then
			-- 	print("equalization:", line_width, char_width(idx))
			-- 	line_width = line_width - char_width(idx)
			-- 	idx = idx - gap
			-- 	extra_len = char_width(idx)
			-- end

			max_width = line_width > max_width and line_width or max_width
			max_height = max_height + line_max_height
			line_max_height = 0

			local pos = (idx+1) / gap
			local next_unicode = char_unicode(idx+gap)
			--make sure punctation does not stand at line head
			if is_punct(next_unicode) then
				line_width = line_width + char_width(idx+gap)
				pos = pos + 1
				next_unicode = char_unicode(idx+gap+gap)
				ignore_next = ignore_next + 1
			end

			if next_unicode and next_unicode ~= 10 then
				if ignore_next > 0 or not is_alnum(char_unicode(idx)) then
					_add_linefeed(fields, pos)
				else
					local forward_len, start, stop, backward_len = _locate_alnum(char_sizes, idx)
					if stop == idx+gap and not is_punct(char_unicode(stop+gap)) then
						ignore_next = ignore_next+1
						local scale = line_width * 1000 / (line_width + char_width(stop))
						scale = scale >= 970 and scale or 1000
						line_width = line_width + char_width(stop)
						_add_linefeed(fields, pos+1, scale)
						-- print("shrink:", scale)
					else
						local scale = width * 1000 / (line_width - forward_len)
						-- local scale = line_width * 1000 / (line_width - forward_len)
						if scale <= 1250 and scale > 0 then
							extra_len = forward_len
							line_width = line_width - extra_len
							_add_linefeed(fields, ((start+1) / gap)-1, scale)
						else
							_add_linefeed(fields, pos)
						end
						-- print("extend:", scale)
					end
				end

				-- print("............delta:", line_width, line_width - width, char_width(idx), ignore_next)
			end
			line_width = 0
		end
	end
	if line_width < width and line_width > 0 then
		max_height = max_height + line_max_height
	end
	max_width = max_width == 0 and line_width or max_width
	return max_width, max_height
end

local function _post_format(label, txt, fields)
	local w, h = _layout(label, txt, fields)
	return {txt, fields, w, h}
end

local function format(label, txt)
	local fields = {}
	local pos_pieces = {}
	local tag_cnt = 0
	for match in string.gmatch(txt, "(#%[%w+%])") do
		tag_cnt = tag_cnt + 1
	end
	if tag_cnt == 0 then
		return _post_format(label, txt, fields)
	end

	local s=1
	local e=1
	local pos_cnt = 0
	for i=1, tag_cnt do
		s, e = string.find(txt, "(#%[%w+%])")
		local tag = string.sub(txt, s+2, e-1)
		if not operates[tag] then
			txt = string.gsub(txt, "(#%[%w+%])", "", 1)
		else
			local pos = string.char_len(string.sub(txt, 1, s))
			pos_cnt = pos_cnt+1
			pos_pieces[pos_cnt] = pos

			pos_cnt = pos_cnt+1
			pos_pieces[pos_cnt] = tag

			txt = string.gsub(txt, "(#%[%w+%])", "", 1)
		end
	end

	local count = pos_cnt / 2
	local last_field = nil
	for i=1, count do
		local pos = pos_pieces[2*i-1]
		local tag = pos_pieces[2*i]
		local ope = operates[tag]
		if ope.type == "color" then
			local field = {false, false, false, false}
			field[1] = pos
			field[2] = i==count and string.char_len(txt) or pos_pieces[2*(i+1)-1]-1
			field[3] = CTL_CODE_COLOR
			field[4] = ope.val
			table.insert(fields, field)

			last_field = field
		elseif ope.type == CTL_CODE_POP then
			last_field = nil
		elseif ope.type == CTL_CODE_LINEFEED then
			local field = {false, false, false, false}
			field[1] = pos
			field[2] = pos
			field[3] = ope.type
			field[4] = ope.val
			table.insert(fields, field)

			if last_field then
				local field_restore = {false, false, false}
				field_restore[1] = pos
				field_restore[2] = i==count and string.char_len(txt) or pos_pieces[2*(i+1)-1]-1
				field_restore[3] = last_field[3]
				field_restore[4] = last_field[4]
				table.insert(fields, field_restore)
				last_field = field_restore
			end
		end
	end

	return _post_format(label, txt, fields)
end

return {
	init_operates=init_operates,
	is_rich=is_rich,
	format=format,
}
