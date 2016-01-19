-- This example show how to write a flappy bird
local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"
local matrix = require "ejoy2d.matrix"

local config = require "examples.asset.bird_config"

pack.load {
	pattern = fw.WorkDir..[[examples/asset/?]],
	"birds",
}

local DEBUG = false

local screen_width = 1024
local screen_height = 768

-- all sprites
local sky1 = ej.sprite("birds", "sky_bg")
local sky2 = ej.sprite("birds", "sky_bg")
local land1 = ej.sprite("birds", "land_bg")
local land2 = ej.sprite("birds", "land_bg")

local scoreboard = ej.sprite("birds", "scoreboard")
scoreboard.text = "0"

-- util
local function _width(s, scale)
  return -s:aabb({x=0,y=0, scale=scale})
end

local function _half_height(s, scale)
  local _, h1, _, h2 = s:aabb({x=0,y=0, scale=scale})
  return (-h1 + h2)/2
end

local function _real_width(s, scale)
  local w1, _, w2, _ = s:aabb({x=0,y=0, scale=scale})
  return -w1 + w2
end

-- draw
local movingBg = {}
movingBg.__index = movingBg
function movingBg.new(obj1, obj2, y)
  local obj = {}
  obj.obj1 = obj1
  obj.obj2 = obj2
  print(obj1, obj2, x, y)

  -- init x, y
  obj.x = _width(obj1) 
  obj.y = y

  obj.diff = 0
  obj.speed = 0

  obj.width = _real_width(obj1)
  assert(obj.width > screen_width)

  return setmetatable(obj, movingBg)
end

function movingBg:set_speed(speed)
  self.speed = speed
end

function movingBg:get_speed()
  return self.speed
end

function movingBg:draw()
  self.obj1:draw({x=self.x, y=self.y})
  self.obj2:draw({x=self.x + self.width, y=self.y})
end

function movingBg:update()
  self.diff = self.diff + self.speed
  self.diff = self.diff % (2 * self.width)
  if self.diff > (2*self.width - screen_width) then
    self.obj1:ps(2*self.width - self.diff, 0)
  else
    self.obj1:ps(-self.diff, 0)
  end
  self.obj2:ps(-self.diff, 0)
end


local half_land_height = _half_height(land1)
local land_height = 2 * half_land_height
local sky_height = _half_height(sky2)

-- gen_matrix for birds_annimation
-- 顶部向上拉长4倍
-- 底部向下拉长4倍
local function gen_matrix()
  local scale = 4
  local blank_height = config.blank_height * config.pipe_scale
  local header_height = config.pipe_scale * config.header_height
  local tail_height = config.pipe_scale * config.tail_height

  local offset1 = (blank_height + tail_height)*0.5 + header_height
  local offset2 = blank_height * 0.5 + header_height + scale * 0.5 * tail_height

  local m1 = matrix{x=0, y=offset1}
  m1 = m1:mul(matrix{sx=1,sy=scale, x=0, y=-offset2})
  local m2 = matrix({x=0, y=-offset1})
  m2 = m2:mul(matrix{sx=1, sy=scale, x=0, y=offset2})
  print("----------m1:", m1:export())
  print("----------m2:", m2:export())
end
gen_matrix()

local pipe_obj = ej.sprite("birds", "pipe")

math.randomseed(os.time())
local pipe = {}
pipe.__index = pipe

pipe.blank_height = config.pipe_scale * config.blank_height
pipe.init_x = -100
pipe.min_init_y = pipe.blank_height * 0.5 + config.header_height * config.pipe_scale + 30
pipe.max_init_y = screen_height - (0.5 * pipe.blank_height + land_height + config.header_height*config.pipe_scale + 10)

function pipe.new()
  local obj = {}
  obj.sprite = ej.sprite("birds", "pipe")
  obj.x = pipe.init_x
  obj.y = math.random(pipe.min_init_y, pipe.max_init_y)

  print("--------- sprite:", obj.x, obj.y)
  --
  obj.offset = 0
  return setmetatable(obj, pipe)
end

function pipe:get_init_x()
  return self.x
end

function pipe:get_x()
  return self.x + self.offset
end

function pipe:get_y()
  return self.y
end

function pipe:set_x(x)
  self.offset = x - self.x
end

--
function pipe:update(dist)
  -- print("----------update:", dist, self.sprite, self.offset, dist, self.x, self.y)
  self.offset = self.offset + dist
  self.sprite:ps(self.offset, 0)
end

function pipe:draw()
  self.sprite:draw({x=self.x, y=self.y})
end

local pipes = {}
pipes.pool_length = 100
pipes.pool = {} -- pipe pool
pipes.choosed = {} -- choosed pipes
pipes.width = 0 -- pipe width
pipes.space = 190 -- pipe space
pipes.speed = 0
pipes.init_offset = screen_width + 200
--pipes.init_offset = screen_width
pipes.half_blank_height = pipe.blank_height / 2

function pipes:init()
  for i=1, self.pool_length do
    self.pool[i] = pipe.new()
  end
  self.width = _real_width(self.pool[1].sprite)
  self.max_pipes = math.ceil(screen_width / (self.width + self.space)) + 3
  assert(self.max_pipes < self.pool_length)
end

function pipes:choose_pipe()
  local i = math.random(#self.pool)
  local p = table.remove(self.pool, i)
  if #self.choosed == 0 then
    p:set_x(self.init_offset)
  else
    local offset = self.choosed[#self.choosed]:get_x() + self.space + self.width
    p:set_x(offset)
  end
  table.insert(self.choosed, p)
  print("---------------pipe:", #self.choosed, self.choosed[#self.choosed]:get_x(), self.choosed[#self.choosed]:get_y())
end

function pipes:reset()
  self.offset = self.init_offset

  while #self.choosed > 0 do
    local p = table.remove(self.choosed)
    table.insert(self.pool, p)
  end
  assert(#self.pool == self.pool_length)

  for i=1, self.max_pipes do
    self:choose_pipe()
  end
end

function pipes:set_speed(speed)
  self.speed = speed
end

function pipes:update()
  if #self.choosed == 0 then
    return
  end

  for _, p in ipairs(self.choosed) do
    p:update(-self.speed)
  end

  local p1 = self.choosed[1]
  local x = p1:get_x()
  if x + self.width / 2 < 0 then
    -- move out screen
    -- choose new one
    self:choose_pipe()
    -- put back to pool
    table.remove(self.choosed, 1)
    table.insert(self.pool, p1)
  end
end

function pipes:draw()
  for _, p in ipairs(self.choosed) do
    p:draw()
  end
end

function pipes:find_clamp(x)
  local ret
  for i, p in ipairs(self.choosed) do
    if p:get_x() >= x then
      print("++++++ find clamp:", i, x, p:get_x(), p:get_y())
      return p, ret
    end
    ret = p
  end
end

pipes:init()

local bg = {}
bg.land = movingBg.new(land1, land2, screen_height - half_land_height)
bg.sky = movingBg.new(sky1, sky2, screen_height - land_height - sky_height)
bg.pipes = pipes

function bg:stop()
  bg.sky:set_speed(0)
  bg.pipes:set_speed(0)
  bg.land:set_speed(0)
end

function bg:begin()
  pipes:reset()
  bg.sky:set_speed(2)
  bg.pipes:set_speed(8)
  bg.land:set_speed(8)
end

function bg:is_moving()
  return bg.land:get_speed() > 0
end

function bg:draw()
  bg.sky:draw()
  bg.pipes:draw()
  bg.land:draw()
end

function bg:update()
  bg.sky:update()
  bg.pipes:update()
  bg.land:update()
end

local bird = {}
bird.sprite = ej.sprite("birds", "bird")
bird.x = 350
bird.half_width = _width(bird.sprite)
bird.half_height = _half_height(bird.sprite)
bird.y = screen_height - land_height - bird.half_height

-- const
bird.touch_speed = 13.5
bird.g = 1.5 -- 重力加速度

-- variable
bird.speed = 0
bird.dt = bird.g

bird.altitude = 0

-- 死亡后保护一段时间才能开始游戏
bird.guard_time = 0

-- pipe
bird.behind = nil
bird.score = 0

-- debug
bird.stop = false

function bird:draw()
  self.sprite:draw({x=self.x, y=self.y})
end

function bird:reset()
  -- init height
  self.altitude = 50
  self.dt = self.g

  self.guard_time = 0

  self.behind = nil
  self.score = 0
end

function bird:update_altitude()
  -- print("jump:", self.speed, self.altitude)
  if self.speed > 0 then
    self.sprite:sr(360 - (self.speed/self.touch_speed) * 30)
  elseif self.speed == 0 then
    self.sprite:sr(0)
  else
    self.sprite:sr((-self.speed /(self.touch_speed * 5))*75 % 75)
  end

  self.altitude = self.altitude + self.speed
  if self.altitude > 0 then
    self.speed = self.speed - self.dt
  else
    self.altitude = 0
    self.speed = 0
    bg:stop()
  end
  self.sprite:ps(0, -self.altitude)
end

function bird:crash_with(p)
  if not p then
    return false
  end

  -- local offset_x = pipes.width + self.width
  -- local offset_y = pipes.half_blank_height - self.height
  local offset_x = pipes.width/2
  local offset_y = pipes.half_blank_height
  local x = p:get_x()
  local y = p:get_y()

  local bird_x = self.x
  local bird_y = self.y - self.altitude

  if x-offset_x <= bird_x and bird_x <= x + offset_x then
    if y-offset_y <= bird_y and bird_y <= y+offset_y then
      return false
    end
    print("-------------- crash -------------")
    print(string.format("bird x:%s, y:%s, altitude:%s, half height:%s", bird_x, bird_y, self.altitude, self.half_height))
    print(string.format("pipe x:%d, y:%d, offset_x:%d, offset_y:%d", x, y, offset_x, offset_y))
    print("-------------- crash end-------------")
    return true
  end
  return false
end

function bird:crash()
  local front, behind = pipes:find_clamp(self.x)
  if self:crash_with(front) or self:crash_with(behind) then
    return true
  end

  if self.behind ~= behind then
    self.behind = behind
    self.score = self.score + 1
  end

  return false
end

function bird:update()
  if self.stop then
    return
  end

  if self.guard_time > 0 then
    self.guard_time = self.guard_time - 1
  end

  if bg:is_moving() and self:crash() then
    if DEBUG then
      self.stop = true
    else
      self.dt = 10 * self.g
      self.guard_time = 15
    end
    bg:stop()
  end

  self:update_altitude()
  scoreboard.text = tostring(self.score)
end

function bird:touch()
  if self.stop or self.guard_time > 0 then
    return
  end

  if not bg:is_moving() then
    self:reset()
    bg:begin()
  else
    self.speed = self.touch_speed
  end
end

local game = {}
function game.update()
  bg:update()
  bird:update()
end

function game.drawframe()
	ej.clear(0xff51c0c9)	-- clear blue

  bg:draw()
  bird:draw()
  scoreboard:draw{x=screen_width/2, y=150}
end

function game.touch(what, x, y)
  if what == "BEGIN" then
    bird:touch()
  end
end

function game.message(...)
end

function game.handle_error(...)
end

function game.on_resume()
end

function game.on_pause()
end

ej.start(game)

