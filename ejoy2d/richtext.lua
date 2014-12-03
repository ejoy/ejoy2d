

--------------------helpers--------------------
function string.char_len(str)
  local _, len = string.gsub(str, "[^\128-\193]", "")
  return len
end

function string.plain_format(str)
  return string.gsub(str, "(#%[%w+%])", "")
end

local function is_ASCII_DBC_punct(unicode)
	-- ! to /
	if unicode >= 33 and unicode <= 47 then return true end
	-- : to @
	if unicode >= 58 and unicode <= 64 then return true end
	-- [ to `
	if unicode >= 91 and unicode <= 96 then return true end
	-- { to ~
	if unicode >= 123 and unicode <= 126 then return true end

	-- if unicode >= 33 and unicode <= 126 then return true end

	return false
end

local function is_ASCII_SBC_punct(unicode)
	return is_ASCII_DBC_punct(unicode-65248)
end

local function is_JP_punct(unicode)
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
----------------ends of helpers-----------------

local M = {}

local CTL_CODE_POP=0
local CTL_CODE_COLOR=1
local CTL_CODE_LINEFEED=2

M.operates = {
	yellow	={val=0xFFFFFF00,type="color"},
	red			={val=0xFFFF0000,type="color"},
	blue		={val=0xFF000003,type="color"},
	green		={val=0xFF00FF00,type="color"},
	stop			={type=CTL_CODE_POP},
	lf 			={type=CTL_CODE_LINEFEED},
}


function M:init_operates(operates)
	for k, v in pairs(operates) do
		self.operates[k] = v
	end
end

local pos_pieces = {}
function M.is_rich(txt)
	local _, cnt = string.plain_format(txt)
	return cnt > 0
end

function M:format(label, txt)
	local fields = {}
	local tag_cnt = 0
	for match in string.gmatch(txt, "(#%[%w+%])") do
		tag_cnt = tag_cnt + 1
	end
	if tag_cnt == 0 then
		return self:_post_format(label, txt, fields)
	end

	local s=1
	local e=1
	local pos_cnt = 0
	for i=1, tag_cnt do
		s, e = string.find(txt, "(#%[%w+%])")
		local tag = string.sub(txt, s+2, e-1)
		if not self.operates[tag] then
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
		local ope = self.operates[tag]
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

	return self:_post_format(label, txt, fields)
end

function M:_post_format(label, txt, fields)
	local w, h = self:layout(label, txt, fields)
	return {txt, fields, w, h}
end

local char_sizes = {}
function M:layout(label, txt, fields)
	--{label_width, label_height, char_width_1, char_width_1, unicode_len_1, unicode_1...}
	local size_cnt = label:char_size(char_sizes, txt)

	local sizes = char_sizes
	local width = sizes[1]

	local line_width = 0
	local char_idx = 1
	local gap = 4
	local ignore_next = false
	local max_width, max_height, line_max_height=0, 0, 0
	for i=3, size_cnt, gap do
		if not ignore_next then
			line_width = line_width + sizes[i]
		else
			ignore_next = false
		end
		--reset if \n
		if sizes[i+3] == 10 then
			max_height = max_height + sizes[i+1]
			line_width = 0
		end
		line_max_height = sizes[i+1] > line_max_height and sizes[i+1] or line_max_height
		char_idx = char_idx + sizes[i+2]
		if line_width >= width then
			max_width = line_width > max_width and line_width or max_width
			max_height = max_height + line_max_height
			line_max_height = 0

			local pos = (i+1) / gap
			local next_unicode = sizes[i+gap+3]
			if is_punct(sizes[i+gap+3]) then
				pos = pos + 1
				next_unicode = sizes[i+gap+gap+3]
				ignore_next = true
			end

			line_width = 0
			if next_unicode and next_unicode ~= 10 then
				local field = {false, false, false, false}
				field[1] = pos-1  --zero base index
				field[2] = pos-1
				field[3] = CTL_CODE_LINEFEED
				field[4] = 0
				table.insert(fields, field)
			end
		end
	end
	if line_width < width and line_width > 0 then
		max_height = max_height + line_max_height
	end
	max_width = max_width == 0 and line_width or max_width
	return max_width, max_height
end

return M
