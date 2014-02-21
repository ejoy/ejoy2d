## package

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

spritepack.pack 打包一个很大的表时，可能会临时消耗大量的内存，并消耗很多的时间解析这张表。在移动设备上，很可能没有这么多的内存可供使用，过长的解析时间也对用户体验造成坏影响。可以用 spritepack.export 在开发期预处理 spritepack.pack 生成的结果。spritepack.export 返回一个字符串，可讲这个字符串持久化到文件中。

spritepack.import 接受 spritepack.export 生成的字符串，返回和 spritepack.pack 相同的结构，可供 spritepack.init 使用。

> spritepack.query( packname, name )

从 packname 指代的图元包中查询一个命名为 name 的对象。对于匿名对象，name 可以为一个数字 id 。如果找到指定的对象，返回这个对象以及它的 id 。

这是一个内部 api ，供 sprite 构造使用。

## simplepackage

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

## texture

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

## matrix

ejoy2d 使用一个 3*2 的 2D 变换矩阵，进行 2D 图像的各种变换操作。这个矩阵使用定点运算，所以矩阵即是一个 6 个整数构成的整数数组。ejoy2d.matrix 是一个方便开发者使用的针对矩阵数据类型处理的数学库。所有相关 API 在接受一个 matrix 参数时，可以是一个由 ejoy2d.matrix 构造程序来 lua userdata ，也可以是其它 C 模块传递过来的 lightuserdata （C 指针）。如果它是一个 lightuserdata ，需要调用者保证它指向的的确是一个长度为 6 的整数数组。
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

