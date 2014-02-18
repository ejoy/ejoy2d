local c = require "ejoy2d.matrix.c"

local matrix_meta = {
	__index = c,
	__tostring = c.tostring,
}

function matrix(srt)
	local mat = c.new(srt)
	if type(srt) == "table" then
		if srt.scale then
			c.scale(mat, srt.scale)
		elseif srt.sx and srt.sy then
			c.scale(mat,srt.sx, srt.sy)
		end
		if srt.rot then
			c.rot(mat, srt.rot)
		end
		if srt.x and srt.y then
			c.trans(mat, srt.x, srt.y)
		end
	end
	return debug.setmetatable(mat, matrix_meta)
end

return matrix