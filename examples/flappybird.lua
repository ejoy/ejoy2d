-- This example show how to write a flappy bird
local ej = require "ejoy2d"
local fw = require "ejoy2d.framework"
local pack = require "ejoy2d.simplepackage"

pack.load {
	pattern = fw.WorkDir..[[examples/asset/?]],
	"birds",
}

-- all sprites
local pipeUp    = ej.sprite("birds", "PipeUp.png")
local pipeDown  = ej.sprite("birds", "PipeDown.png")
local spaceship = ej.sprite("birds", "Spaceship.png")
local sky1 = ej.sprite("birds", "sky_bg")
local sky2 = ej.sprite("birds", "sky_bg")
local land1 = ej.sprite("birds", "land_bg")
local land2 = ej.sprite("birds", "land_bg")

local screen_width = 1024
local screen_height = 768

-- util
local function _width(s, scale)
  return -s:aabb({x=0,y=0, scale=scale})
end

local function _height(s, scale)
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

local land_height = _height(land1)
local sky_height = _height(sky2)

local bg = {}
bg.land = movingBg.new(land1, land2, screen_height - land_height)
bg.sky = movingBg.new(sky1, sky2, screen_height - 2*land_height - sky_height)

function bg:stop()
  bg.land:set_speed(0)
  bg.sky:set_speed(0)
end

function bg:begin()
  bg.land:set_speed(8)
  bg.sky:set_speed(2)
end

function bg:is_moving()
  return bg.land:get_speed() > 0
end

function bg:draw()
  bg.land:draw()
  bg.sky:draw()
end

function bg:update()
  bg.land:update()
  bg.sky:update()
end

local bird = {}
bird.sprite = ej.sprite("birds", "bird")
bird.scale = 5
bird.x = 200
bird.y = screen_height - 2 * land_height - _height(bird.sprite, bird.scale)

-- const
bird.touch_speed = 3.5
bird.g = 0.09 -- 重力加速度

-- variable
bird.speed = 0
bird.dt = 0

bird.height = 0
function bird:draw()
  self.sprite:draw({x=self.x, y=self.y, scale = self.scale})
end

function bird:update()
  print("jump:", self.speed, self.dt, self.height)
  if self.speed > 0 then
    self.sprite:sr(360 - (self.speed/self.touch_speed) * 30)
  elseif self.speed == 0 then
    self.sprite:sr(0)
  else
    self.sprite:sr((-self.speed /(self.touch_speed * 5))*75 % 75)
  end

  self.height = self.height + self.speed
  if self.height > 0 then
    self.dt = self.dt + self.g
    self.speed = self.speed - self.dt
  else
    self.height = 0
    self.dt = 0
    self.speed = 0
    bg:stop()
  end
  self.sprite:ps(0, -self.height)
end

function bird:touch()
  if not bg:is_moving() then
    self.height = 50
    bg:begin()
  else
    self.speed = self.touch_speed
  end
  self.dt = 0
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

