

function string.char_len(str)
  local _, len = string.gsub(str, "[^\128-\193]", "")
  return len
end

function string.plain_format(str)
  return string.gsub(str, "(#%[%w+%])", "")
end

local M = {}

M.operates = {
	yellow	={val=0xFFFFFF00,type="color"},
	red			={val=0xFFFF0000,type="color"},
	blue		={val=0xFF0000FF,type="color"},
	green		={val=0xFF00FF00,type="color"},
	stop			={type="ctl"},
}

local pos_pieces = {}
function M.is_rich(txt)
	local _, cnt = plain_format(txt)
	return cnt > 0
end

function M:format(txt)
	local tag_cnt = 0
	for match in string.gmatch(txt, "(#%[%w+%])") do
		tag_cnt = tag_cnt + 1
		-- print(match)
	end
	if tag_cnt == 0 then
		return txt
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
			print(s,e,pos, tag)
			pos_cnt = pos_cnt+1
			pos_pieces[pos_cnt] = pos

			pos_cnt = pos_cnt+1
			pos_pieces[pos_cnt] = tag

			txt = string.gsub(txt, "(#%[%w+%])", "", 1)
		end
	end

	local count = pos_cnt / 2
	local fields = {}
	for i=1, count do
		local pos = pos_pieces[2*i-1]
		local tag = pos_pieces[2*i]
		local ope = self.operates[tag]
		if ope.type == "color" then
			local field = {false, false, false}
			field[1] = pos
			field[2] = i==count and string.char_len(txt) or pos_pieces[2*(i+1)-1]-1
			field[3] = ope.val
			table.insert(fields, field)
		end
	end
	return #fields > 0 and {txt, fields} or txt
end

-- M:format("一二三#[red]四五六12七八ab九十#[stop]一二三四五d六七八九十#[red]#[stop]一二三四五六七八九十一二三#[yellow]四五六七八九十")

return M
