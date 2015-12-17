
- **[package](#package)**
- **[simplepackage](#simplepackage)**
- **[texture](#texture)**
- **[matrix](#matrix)**
- **[sprite](#sprite)**
- **[particle](#particle)**

## <span id="package">package</span>

这是一个内部模块，你可以用它实现出适合你的项目的资源包。

```Lua
local spritepack = require "ejoy2d.spritepackage"
```

> spritepack.pack(data)

data 是一个 lua table ，它描述了一组图片以及动画。范例见 [examples/asset/sample.lua](https://github.com/cloudwu/ejoy2d/blob/master/examples/asset/sample.lua) 。

这个 api 能将这个 table 打包成一个 2 进制数据块供引擎使用，同时还生成供引擎使用的元信息，如用于检索图元的字典、引用的贴图数量等等。

返回值为一个包含有这些元信息的 table 。

> spritepack.init( name, texture, meta )

将 spritepack.pack 生成的 meta 表，以 name (字符串) 命名，构造出一个供引擎使用的图元包。当这个包只使用一张贴图时，texture 应该传入贴图 id ；如果包可能引用多张贴图，texture 应为一个 table ，里面有所有被引用的贴图 id 。


> spritepack.export( meta )
> spritepack.import( data )

这一对 api 可用于 meta 数据的打包与解包。

spritepack.pack 打包一个很大的表时，可能会临时消耗大量的内存，并消耗很多的时间解析这张表。在移动设备上，很可能没有这么多的内存可供使用，过长的解析时间也对用户体验造成坏影响。可以用 spritepack.export 在开发期预处理 spritepack.pack 生成的结果。spritepack.export 返回一个字符串，可将这个字符串持久化到文件中。

spritepack.import 接受 spritepack.export 生成的字符串，返回和 spritepack.pack 相同的结构，可供 spritepack.init 使用。

> spritepack.query( packname, name )

从 packname 指代的图元包中查询一个命名为 name 的对象。对于匿名对象，name 可以为一个数字 id 。如果找到指定的对象，返回这个对象以及它的 id 。

这是一个内部 api ，供 sprite 构造使用。

## <span id="simplepackage">simplepackage</span>

```Lua
local simplepackage = require "ejoy2d.simplepackage"
```
simplepackage 是对 package 的一个简单封装，它按一定命名规则，从文件系统中加载一个图元描述表，通过 ejoy2d.spritepackage 解析后加载到内存中。

在开发期，simplepackage 是一个简单易用的选择；但通常不建议最终项目直接使用它。最好能根据你的项目情况，设计一个合适的资源包文件格式，使用 package 的低层 api 封装出项目相关的包支持模块。

```Lua
simplepackage.load { pattern = "path/?" , packagename1, packagename2, ... }
```

参数中的 pattern 指资源包所在文件系统中的位置。? 将被后续的 packagename 字符串替代。

例如：
````Lua
simplepackage.load { pattern = "path/?" , "sample" }
```
会加载 path/sample.lua 作为图元的描述文件，以及将 path/sample.1.ppm 作为这个包所用到的第一张贴图。如果存在 path/sample.1.pgm 文件，还会将它作为贴图的 alpha 通道。如果包的描述文件中提到了多张贴图，会继续尝试加载 path/sample.2.ppm 等图片。

```Lua
simplepackage.sprite( packname, name )
```
从 packname 指代的包中构造名为 name 的对象并返回。

## <span id="texture">texture</span>

ejoy2d 的开源版本暂时只提供 ppm/pgm 文件格式的支持，但你可以根据项目需要很容易的扩充其它文件格式，或自定义文件格式（如引入 libpng 库支持 .png 文件，或支持 PowerVR 压缩贴图等）。

ejoy2d 用 id 管理贴图，最多支持 128 张贴图（上限 **MAX_TEXTURE** 定义在 [lib/texture.c](https://github.com/cloudwu/ejoy2d/blob/master/lib/texture.c) 中），合法的 id 从 0 到 127 。

使用 ppm/pgm 格式的贴图文件，需要加载 ejoy2d.ppm 模块。

```Lua
local ppm = require "ejoy2d.ppm"
```

> ppm.texture(id, filename)

将一个 ppm/pgm 文件加载到 id 指定的贴图中。重复加载不同的贴图到同一个 id ，后加载的将覆盖先加载的贴图。id 的管理工作交给使用者，如果使用 simplepackage 模块的话，它会简单的为所有加载的包分配贴图 id 。所以不建议联合使用 simplepackage 和 ppm.texture ，除非你知道如何确切的管理贴图 id 。

filename 不能有后缀 ppm 或 pgm 。ppm.texture 会尝试打开 filename.ppm 文件和 filename.pgm 文件。如果两个文件都存在，贴图将被创建为 RGBA 格式，带有 alpha 通道；如果只存在 filename.ppm ，贴图被创建为 RGB 格式，没有 alpha 通道；如果只存在 filename.pgm 文件，贴图则被创建为 alpha 单通道。

你可以在 ppm 文件中描述颜色深度，一般的图形处理软件给出的默认深度为 255 ，对应的贴图为 8 bit 的。但你也可以生成深度为 15 的图片文件，ppm.texture 将为你生成 RGBA4444 或 RGB565 格式的贴图（比 8 位贴图更节省内存及显存）。

> ppm.load(filename)

这是一个方便调试用的 ppm 文件加载器。加载一个 ppm/pgm 文件，filename 不包括后缀。和 ppm.texture 一样，会同时尝试打开 filename.ppm 和 filename.pgm 来决定图片的类型。

如果加载成功，会返回四个值：

 1. 图片类型，可能是下列类型中的一种：RGBA8 RGB8 ALPHA8 RGBA4 RGB4 ALPHA4

 2. 图片宽度（像素单位）

 3. 图片高度（像素单位）

 4. 一个 table ，内含所有像素的整数值。

> ppm.save(filename, type, width, height, data)

和 ppm.load 对应，可生成一个 ppm/pgm 文件。

### 生成 ppm/pgm 文件

如果你知道如何通过 lua 把常见格式如 png 加载到内存，那么你就可以用 ejoy2d 自带的 ppm.save 来生成 ppm/pgm 文件。

ppm 的文件格式非常简单，可以很容易的用任何编程语言操作。参见：http://en.wikipedia.org/wiki/Netpbm_format 

你也可以使用 http://www.imagemagick.org/ 这个工具把带通道的 png 文件转换为一组 ppm/pgm 文件：

> convert image.png image.ppm

提取 Alpha 通道：

> convert image.png -channel A -separate image.pgm

把 ppm/pgm 合成一张带 Alpha 通道的 png 文件可以：

> convert image.ppm image.pgm -compose copy-opacity -composite image.png

## <span id="matrix">matrix</span>

ejoy2d 使用一个 3*2 的 2D 变换矩阵，进行 2D 图像的各种变换操作。这个矩阵使用定点运算，所以矩阵即是一个 6 个整数构成的整数数组。

```
| m[0] m[1] 0 |
| m[2] m[3] 0 |
| m[4] m[5] 1 |
```

当我们给它添加默认的一列 (0,0,1) 后就构成了一个 3*3 的二维变换矩阵。任何一个二维坐标 |x y 1| 都可以通过和这个 3*3 矩阵做乘法做二维空间变换。

ejoy2d.matrix 是一个方便开发者使用的针对矩阵数据类型处理的数学库。所有相关 API 在接受一个 matrix 参数时，可以是一个由 ejoy2d.matrix 构造程序来 lua userdata ，也可以是其它 C 模块传递过来的 lightuserdata （C 指针）。如果它是一个 lightuserdata ，需要调用者保证它指向的的确是一个长度为 6 的整数数组。
```Lua
{ 1024, 0, 0, 1024, 0, 0 }  -- 单位矩阵
```
由于是定点数，1024 表示 1.0 ，上面展示了一个单位矩阵。对于矩阵的最后两个数字，**SCREEN_SCALE** : 16 （定义在 [lib/spritepack.h](https://github.com/cloudwu/ejoy2d/blob/master/lib/spritepack.h) 中） 表示 1.0 。

```Lua
local matrix = require "ejoy2d.matrix"
```

这里 matrix 就是一个矩阵构造函数，可以用下列方法构造出一个矩阵。
```Lua
local m = matrix()
```
构造一个单位矩阵。
```Lua
local m = matrix { 1024, 0, 0, 1024, 0, 0 }
```
用一个 table （6 个整数）构造出一个矩阵。
```Lua
local m = matrix (mat)
```
这里 mat 可以是另一个 matrix 对象或是一个 C 的 matrix 结构指针（一个 lightuserdata），这可以复制出一个新的 matrix 对象出来。
```Lua
local m = matrix { scale = 2.0, rot = 90.0, x = 10.0, y = 20.0 }
```
这里构造了一个先放大 2.0 倍，再顺时针旋转 90 度，最后平移 (10.0, 20.0) 单位的变换矩阵。你也可以在 X 轴和 Y 轴上单独做缩放，例如：
```Lua
matrix { sx = -1, sy = 1 }
```
可以构造出一个左右镜像的变换。
> matrix:mul(m)

将矩阵 m 乘在 self 的右侧。
> matrix:inverse()

对矩阵求逆。

> matrix:identity()

把矩阵设置为单位矩阵。

> matrix:scale(sx,sy)

做一个缩放变换。如果不传入 sy ，则默认 sy 和 sx 相同。

> matrix:rot(deg)

做一个旋转变换，deg 的单独是角度（360 度为一圈）。

> matrix:trans(x,y)

做一个平移变换。

## <span id="sprite">sprite</span>

sprite 是 ejoy2d 可以处理的基本图形对象。每个 sprite 都是若干图元以树状组合起来的。大部分情况下，不可以在运行期用 ejoy2d api 动态构建一棵 sprite 图元树，而只能从外部资源包中加载它。加载 ejoy2d.sprite 模块即可生成 sprite 对象。

```Lua
local sprite = require "ejoy2d.sprite"

-- 从资源包中构造一个 sprite 对象
local obj = sprite.new("packname", "objectname")
```

这里，"packname" 是资源包的名字，"objectname" 是在资源包中对象的导出名。每个资源包中的对象都有一个数字 id ，字符串名是可选的。这个 api 同样支持传入数字 id ，但并不推荐这样用。

构造出来的 sprite 对象都可以对其调用一系列方法，而下列方法仅仅是被记入文档的一部分。未被文档化的方法可以在源代码中找到，但它们更可能在未来有变动，需要酌情使用。

### sprite 方法

```Lua
sprite:draw(srt)
```
这个 api 用于把 sprite 对象绘制到屏幕。注意：ejoy2d 是一个 2d engine ，它并没有层次或 Z 排序的概念。使用 ejoy2d 绘制 sprite 必须遵从画家算法。在同一个渲染帧内，先调用 sprite:draw 方法的对象会先被绘制到屏幕上，即在后绘制的对象的下方。如果 srt 这个参数为 nil ，那么对象被绘制到屏幕坐标原点，即屏幕的左上角。ejoy2d 的坐标系统是 X 轴向右，y 轴向下。

srt 参数可以是一张变换表。位移变换放在 .x 和 .y 中，单位为像素，默认值为 0 ，可以是小数。缩放变换在 .sx 和 .sy 中，默认值为 1 即没有缩放。旋转变换在 .rot 中，单位为度，360 度一圈，默认值为 0 。

srt 参数不建议用来给 sprite 对象定位，通常它用于画布的定位。这样你可以为你的画布构造一个 srt 表，然后给循环调用画布上所有 sprite 对象时都传这同一张表。记住：lua 中临时构造表是有一定开销的，应该尽量避免。

```Lua
sprite:ps(x,y,scale)
```
即使你要向屏幕上画多个同样的东西，也不建议只创建一个 sprite 对象，而多次调用它的 draw 方法。更好的选择是创建多个 sprite 对象，这个每个对象都可以有自己的局部矩阵。这个局部空间矩阵可以单独记录对象的位置等信息。

sprite:ps 这个方法就是用来设置对象的位置和缩放信息的。前两个参数是 x 和 y 坐标值，第三个参数是缩放量，这里只提供等比缩放。第三个参数可以不填，这样这个 api 不会改变原有的缩放比。

这个方法也可以只接受一个参数，当只有一个参数时，认为它是 scale 值。而 x,y 都会被保留，不会被改变。

```Lua
sprite:sr(sx,sy,rot)
```
sprite:sr 可以用来做不等比缩放， x 轴和 y 轴的缩放比是分别由前两个参数传入的。比如，如果你想将 sprite 对象做左右镜像，只需要调用 `sprite:sr(-1,1)` 。

第三个参数是可选的，表示旋转这个对象。单位是度，360 度为一圈。当只传入一个参数时，这个参数被解释为旋转，而原有的缩放量不会被改变。

注：缩放、旋转、位移是按次序发生作用的。所以无论你如何缩放（镜像）都不会影响旋转的结果。而位移矢量是在做完缩放和旋转之后才生效的，所以放大对象不会使它偏离原点更远。

```Lua
sprite:test(x,y,srt)
```
test 方法和 draw 有点类似，但是它不会绘制这个对象，而只会用屏幕坐标 (x,y) 测试它。这主要用来实现对象的点选，给 UI 模块提供底层支持。

通常你应该用画家算法的逆序，对绘制在屏幕上，可能被点选的对象依次调用 test 方法，并传入触摸点的屏幕坐标。这个 api 会告诉你这个对象是否在 (x,y) 的位置被触摸到。如果这个对象是一个树状复合对象，那么 test 返回值可能是它的一个子对象。

所以，我们可以把一个复杂的 UI 组件组合成一棵 sprite 树，把可能接受消息的节点在资源文件中标注出名字，并设置上 touch 标记。这样只需要对整个组件做一次测试就够了。

```Lua
sprite:aabb(srt)
```
aabb 和 draw 类似，但它不绘制对象，仅仅返回四个整数表示这个对象的轴对齐包围盒。这四个整数分别是左上角和右下角的屏幕像素坐标。

```Lua
sprite:fetch(name)
```
一个 sprite 对象以树结构组织，若它的子节点在资源文件中标明了名字，那么可以用 fetch 方法获得这个子节点对象。同时，sprite.name 是 `sprite:fetch("name")` 的语法糖，两种写法是等价的。

```Lua
sprite:mount(name, child)
```
在运行时，可以动态换掉一个 sprite 对象的具名子节点。这里 name 是一个字符串，child 是另一个 sprite 对象。sprite.name = child 是 `sprite:mount("name", child)` 的语法糖，两种写法是等价的。

注：不能将同一个 sprite 对象 mount 到两个不同的 sprite 对象上。如果你需要这样做，请用同一份资源构造两个 sprite 对象。

### sprite 属性
* `sprite.frame` 可读写
对象当前帧号
* `sprite.matrix` 可读写
对象的变换矩阵。注：不同于 sprite 在资源文件里标注的变换矩阵，这个矩阵是运行期的。在渲染时，运行期的变换矩阵和资源文件中描述的变换矩阵都会影响对象最终的显示效果（运行期矩阵最后起效果）。默认的运行期变换矩阵为单位矩阵。
* `sprite.visible` 可读写
对象是否显示。这个标记如果被设置为 false，整个子树都不会显示。
* `sprite.text` 可读写
只有 label 类型的对象才有这个属性。它是 label 的文字。
* `sprite.color` 可读写
对象的混合颜色，为一个 32bit ARGB 整数。这个对象及子树在渲染时都会乘上这个颜色，默认值为 0xffffffff 。最常用的做法是用于半透明效果，当 color 为 0x80ffffff 时，就是 50% 的半透明混合。
* `sprite.additive` 可读写
给对象叠加一个颜色，为一个 24bit RGB 整数，默认值为 0 。在渲染这个对象及子树时会叠加这个颜色。
* `sprite.message` 可读写
对象是否截获 test 调用。多用于 UI 控制。
* `sprite.frame_count` 只读
对象的动画帧数。
* `sprite.action` 只写
设置一组动作动画。在资源文件中，一个对象对象可以有多组动画，被称为不同的 action 。每组 action 有一个字符串名字，通过设置 action 可以切换到不同的动画序列。默认的动画序列为资源文件中描述的第一个 action 。
* `sprite.program` 只写
给对象设置一个专有的渲染程序。program 是一个数字 id 。这是一个高级特性，使用它需要对 opengl shader program 有一定了解。ejoy2d 目前在 shader.lua 中预设了几段 program 。比如 `sprite.program = shader.id "GRAY"` 可以让一个对象以灰色图形式渲染。
* `sprite.scissor` 只写
让一个 panel 类型的对象具有剪切器的能力。这可以让一个 panel 节点以一个矩形（指定宽和高）为范围裁减它的子节点。一般用来实现滚动框。
* `sprite.world_matrix` 只读
anchor 类型的对象的特有只读属性。它会返回上一次这个 anchor 对象最终渲染的世界矩阵。注：anchor 类型的对象默认 visible 为 false ，当不可显时，引擎不计算 world matrix 。

### sprite 资源结构
sprite 用 lua 表的形式描述在资源文件中。资源文件通常于开发期构建。不同于运行期 API ，资源的数据定义灵活度要更为灵活。开发者可以按格式手写资源文件，更可以用额外的工具生成它们。对于特殊的需求，还可以用另一段 lua 脚本生成需要的数据（比如用工具生成动画的关键帧，再用程序插值补上中间的运动帧）。

资源的数据结构说明见：http://blog.codingnow.com/2013/12/ejoy2d.html

也可以参考 [examples/asset/sample.lua](https://github.com/cloudwu/ejoy2d/blob/master/examples/asset/sample.lua)

我的同事制作了一个脚本可以将 flash 动画导出成 ejoy2d 的资源文件： https://github.com/robinxb/flash-parser

详细文档待补充。

### sprite 类型
> Picture

最基础的渲染单位之一，表示一个四边形从贴图到屏幕的映射关系。一个Picture结构可以是多个四边形的组合，渲染顺序与组合排列顺序一致。示例如下：

```Lua
{
	type = "picture",
	id = 2,
	{ tex = 1 , 
	src = { 178, 112, 144, 112, 144, 137, 178, 137 }, 
	screen = { -388, -652, -384, 452, 416, 452, 412, -652} },
}
```
src 字段 8 个整数表示的是 4 组贴图坐标，以像素为单位。screen 字段的 8 个整数则表示对应的屏幕坐标，是以屏幕像素为单位，但放大了 16 倍。你也可以认为这是一种定点数表达。
> Polygon

同Picture，对应多边形。

> Animation

一个 animation 是由若干 component 构成的。每个 component 可以用 id 引用其它的 sprite 对象（不限于静态图片组还是另一组动画），但你得保证不能成环。

animation 可以又多组动画序列，每组是一个 action 。当然你也可以只有一组默认动画组，这样不需要给 action 起名字。当有多组的时候，需要在描述数据里加上 action 字段，给一个字符串名字。

一组动画序列（action）是由若干 frame 组成的。每个 frame 可以由这个 animation 中定义的 component 任意组合。我们可以在资源文件中描述组合关系，如果简单引用一个 componenent ，就写上其编号（ base 0 ）。也可以加上一个变换矩阵，以及两个颜色值（ color ，additive ）。渲染的时候引擎会把这个 component 的颜色乘上 color 加上 additive 。

> Label

用于显示文本内容的控件，定义字体的大小，颜色，描边，锚点等信息。

> Panel

用于定义一个剪切区（scissor）

> Anchor

可以认为是Animation中一种特殊类型的component，当一个component没有指定id只有name字段的时候会被当作是一个Anchor。默认情况下Anchor可以作为一个挂点用于mount其他对象，另外在将Anchor对象设为visible状态后，它将在每次渲染之后记录下自己相对于根节点的matrix，可以通过world_matrix方法或world_pos方法获得这个节点的坐标等信息。



## <span id="particle">particle</span>

ejoy2d粒子发射器的描述文件见[examples/asset/particle_particle_config.lua](https://github.com/cloudwu/ejoy2d/blob/master/examples/asset/particle_particle_config.lua)。该文件基于[Particle Designer](http://particledesigner.71squared.com/)的输出而生成，即可以使用Particle Designer来编辑粒子系统。

```Lua
local c = require "ejoy2d.particle.c"
local particle = c.new(config)
```
config对应描述文件中的一个table。ejoy2d只负责根据这个table发射粒子,并不负责具体的渲染，所以它是一个抽象的渲染无关的模块。除了new外，ejoy2d.particle.c还提供如下三个接口：

1. 更新粒子系统，world_matrix为粒子系统的世界坐标矩阵，当希望粒子发射器在世界坐标系类发射粒子（对应于在粒子系统自身坐标系类发射粒子）时，每个粒子会根据这个矩阵记录下当它出生时的世界坐标。
> c.update(particle, deltaTime, world_matrix)

2. 获取粒子系统每个粒子的具体信息，包括矩阵和颜色信息
> local mat = {}
> local colors = {}
> local cnt = c.data(particle, mat, colors)

3. 重置粒子系统，用于重复使用之前生产的粒子系统
> c.reset(particle)

完整的特效系统通过[ejoy2d/particle.lua](https://github.com/cloudwu/ejoy2d/blob/master/ejoy2d/particle.lua)封装实现。除了完成粒子系统的渲染外，一个特效还支持多个粒子系统组成的组合，一个组合内的多个粒子系统可定义它们之间的层级关系，相对位置，甚至可以为单个粒子系统指定动画信息。这一切都是基于sprite来实现的，即我们先定义一个简单的sprite层级结构，每个子节点对应一个粒子系统。特效系统sprite层级结构的示例见[asset/particle.lua](https://github.com/cloudwu/ejoy2d/blob/master/examples/asset/particle.lua)

一个更简单的示例如下：
```lua
{
		component = 	{
			{name = "fire"},
			{name = "ice"}
		},
		export = "ice_fire",
		type = "animation",
		{
		    {{index = 0},{index = 1}},
		}
}
```
如上所示：一个export为ice_fire的sprite包含两个anchor：fire和ice，这是一个典型的sprite定义。ejoy2d/particle.lua会首先创建出这个sprite，并遍历它的component，根据anchor的名字创建单个particle。可以在[examples/asset/particle_particle_config.lua](https://github.com/cloudwu/ejoy2d/blob/master/examples/asset/particle_particle_config.lua)中找到他们对应的粒子发射器配置文件。当anchor和particle绑定之后，如上particle.update所述，我们可以将anchor.world_matrix传入备用

然后我们就可以做粒子系统的渲染。在update之后，通过data接口获取粒子的矩阵和颜色信息，通过sprite.matrix_multi_draw将所有粒子逐一渲染出来

在脚本层可以将一个特效系统当作一个普通的sprite来使用。
