
local ej = require "ejoy2d"

local ESC_CODE = 27
local ESC = string.char(ESC_CODE)

--------------------helpers--------------------
function string.char_len(str)
  local _, len = string.gsub(str, "[^\128-\193]", "")
  return len
end

function string.plain_format(str)
  return string.gsub(str, "(#%[%w+%])", "")
end

function is_esc(unicode)
    return unicode == ESC_CODE
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
----------------ends of helpers-----------------

local M = {}

local gap = 4

local CTL_CODE_POP=0
local CTL_CODE_COLOR=1
local CTL_CODE_LINEFEED=2
local CLT_CODE_ANIM=3

M.operates = {
	yellow      =   {val=0xFFFFFF00,type="color"},
	red         =   {val=0xFFFF0000,type="color"},
	blue        =   {val=0xFF0000FF,type="color"},
	green       =   {val=0xFF00FF00,type="color"},
	stop        =   {type=CTL_CODE_POP},
	lf          =   {type=CTL_CODE_LINEFEED},
    anim        =   {type=CLT_CODE_ANIM},
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
    local sprite_fields = {}
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
    local i = 1
    while i <= tag_cnt do
		s, e = string.find(txt, "(#%[%w+%])")
		local tag = string.sub(txt, s+2, e-1)
        local operate = self.operates[tag]
		if not operate then
			txt = string.gsub(txt, "(#%[%w+%])", "", 1)
		else
            if operate.type == CLT_CODE_ANIM and i < tag_cnt then
                local tmp_txt = string.gsub(txt, "(#%[%w+%])", "", 1)
                local ts, te = string.find(tmp_txt, "(#%[%w+%])")

                local anim_txt = string.sub(tmp_txt, s, ts - 1)
                local sprite_field = {}
                string.gsub(anim_txt, "%w+", function(pattern)
                    table.insert(sprite_field, pattern) end)

                local len = #sprite_field
                local space = 0
                local esc_string = ""

                if len >= 2 then
                    local pack = sprite_field[1]
                    for index = 2, len do
                        local name = sprite_field[index]
                        local sprite = ej.sprite(pack, name)
                        local field= {
                            s - 1 + (index - 2), 0,
                            CLT_CODE_ANIM,
                            sprite,
                        }
                        local aabb = {sprite:aabb()}
                        local w = math.abs(aabb[3] - aabb[1])
                        local h = math.abs(aabb[4] - aabb[2])
                        space = math.max(space, h)

                        table.insert(fields, field)
                        table.insert(sprite_fields, {field, w, h})
                    end

                    esc_string = string.rep(ESC, len - 1)
                end

                txt = string.format("%s%s%s", string.sub(tmp_txt, 1, s - 1), esc_string, string.sub(tmp_txt, ts))
            else
                local pos = string.char_len(string.sub(txt, 1, s))
                pos_cnt = pos_cnt+1
                pos_pieces[pos_cnt] = pos

                pos_cnt = pos_cnt+1
                pos_pieces[pos_cnt] = tag

                txt = string.gsub(txt, "(#%[%w+%])", "", 1)
            end
		end
        i = i + 1
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

	return self:_post_format(label, txt, fields, sprite_fields)
end

function M:_post_format(label, txt, fields, sprite_fields)
	local w, h = self:layout(label, txt, fields, sprite_fields)
    local sprite_count = sprite_fields and #sprite_fields or 0
	return {txt, fields, w, h, sprite_count}
end

function M:_locate_alnum(sizes, anchor)
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

local function add_linefeed(fields, pos, offset)
	local field = {false, false, false, false}
	field[1] = pos-1  --zero base index
	field[2] = pos-1
	field[3] = CTL_CODE_LINEFEED
	field[4] = offset or 1000
	-- field[4] = 1000
	table.insert(fields, field)
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

function M:layout(label, txt, fields, sprite_fields)
	-- print(txt)
	--{label_width, label_height, char_width_1, char_width_1, unicode_len_1, unicode_1...}
	local size_cnt = label:char_size(char_sizes, txt)

	local width = char_sizes[1]

	local line_width = 0
	local char_idx = 1
    local pre_char_idx = 0
	local ignore_next, extra_len = 0, 0
	local max_width, max_height, line_max_height=0, 0, 0
    local sprite_field_index = 1

    local i = 3
    for i = 3, size_cnt, gap do
		local idx = i
        local anim_height = 0
        local is_char_esc = is_esc(char_unicode(idx))

        pre_char_idx = char_idx
        char_idx = char_idx + char_unicode_len(idx)

        if sprite_fields then
            if sprite_field_index <= #sprite_fields then
                local sprite_field = sprite_fields[sprite_field_index]

                local field = sprite_field[1]
                local w = sprite_field[2]
                local h = sprite_field[3]

                if field[1] == pre_char_idx - 1 then
                    line_width = line_width + w
                    anim_height = math.max(anim_height, h)
                    sprite_field_index = sprite_field_index + 1
                end
            end
        end

		if ignore_next == 0 then
            if not is_char_esc then
                line_width = line_width + char_width(idx)
            end
		else
			ignore_next = ignore_next-1
		end
		if extra_len > 0 then
			line_width = line_width + extra_len
			extra_len=0
		end

        local cur_height
        if is_char_esc then
            cur_height = 0
        else
            cur_height = math.max(anim_height, char_height(idx))
        end

		--reset if \n
		if char_unicode(idx) == 10 then
			max_height = max_height + cur_height
			line_width = 0
		end

		line_max_height = cur_height > line_max_height and cur_height or line_max_height

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
					add_linefeed(fields, pos)
				else
					local forward_len, start, stop, backward_len = self:_locate_alnum(char_sizes, idx)
					if not is_char_esc and stop == idx+gap and not is_punct(char_unicode(stop+gap)) then
						ignore_next = ignore_next+1
						local scale = line_width * 1000 / (line_width + char_width(stop))
						scale = scale >= 970 and scale or 1000
						line_width = line_width + char_width(stop)
						add_linefeed(fields, pos+1, scale)
					else
                        -- NOTE:
                        -- local scale = width * 1000 / (line_width - forward_len)
                        -- -- local scale = line_width * 1000 / (line_width - forward_len)
                        -- if scale <= 1250 and scale > 0 then
                        --     extra_len = forward_len
                        --     line_width = line_width - extra_len
                        --     add_linefeed(fields, ((start+1) / gap)-1, scale)
                        -- else
                        --     add_linefeed(fields, pos)
                        -- end
                        add_linefeed(fields, pos)
					end
				end
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

return M
