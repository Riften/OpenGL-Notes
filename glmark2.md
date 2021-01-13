## 架构
glmark2架构总体非常简单，`src`目录下文件包含了核心基类和各个场景的实现，整体上都是面向对象的。

- main-loop.h/MainLoop: 用来执行主循环，`foreach scene`，执行并拿到评分。
- benchmark.cpp/Benchmark: 测试场景的描述类，保存了scene和对scene的配置
- canvas.h/Canvas: GL渲染输出目标的虚类，不同的平台和环境会使用不同的Canvas实现
- scene.cpp/Scene: 所有测试场景的基类。
- scene-xxx.cpp: 特定场景的测试类实现，是Scene的派生。需要注意的是，所有派生类的声明都在`scene.h`里面。
- libmatrix/program.cc/Shader: 用来加载和编译Shader，调用OpenGL的Shader相关接口。
- libmatrix/program.cc/Program: 用来组装和运行Program，调用OpenGL的Program相关接口。


## 测试执行过程
- main.cpp/do_benchmark: 创建`MainLoop`对象，循环执行`MainLoop.step()`直到`step()`函数返回`false`，输出`MainLoop.score()`
  - main-loop.cp/MainLoop.step(): 读取传入的`BenchmarkCollection`中的下一个`Benchmark`，对其中包含的测试进行场景初始化和执行。在执行过程中会首先检查当前场景是否还在运行，如果还在运行，那么就继续调用`MainLoop.draw()`
    - Benchmark.setup_scene(): 完成场景的初始化和设定，调用`Scene.setup()`
    - MainLoop.draw():
      - Canvas.clear():
      - Scene.draw(): 核心绘制过程，根据`update()`的影响重置视角之类，重新执行渲染函数。
      - Scene.update(): 通常用来更新场景，比如给模型转个角度
      - Canvas.update()
    - 执行场景资源的释放，如果已经没有场景需要运行了就返回false

执行过程中，实际的测试运行是通过`Scene.draw()`，其他大多数是框架代码，所以需要分析的只是每个`scene-xxx.cpp`中的`draw()`函数实现。

## TODO
- 啥叫interleave vertex attribute data?
- 为什么glmark2是多线程运行的？

## 场景列表
build, texture, shading, bump, effect2d, pulsar, desktop, buffer, ideas, jellyfish

## Scene: build
是最简单的渲染场景，加载单个无文理模型，单点光源，模型颜色为白色，不透明，漫反射颜色直接用法向乘光线方向得到。

### 可选参数
- use-vbo: 默认true
- interleave: 默认false，Whether to interleave vertex attribute data
- model: 默认horse

### setup()
加载shader，编译shader，组装并使用program:
- Vertex Shader /shaders/light-basic.vert: 基本的点光源漫反射，顶点颜色=材质颜色*$(N\cdot L)$
- Frame Shader /shaders/light-basic.frag: 

加载模型
- Model data/models/horse.3ds: 只加载了顶点和法线信息
- Texture 无

<details>
<summary>模型查看</summary>

![horse](imgs/glmark2_model_horse.png)
</details>

### draw()
根据旋转值调整viewpoint，根据是否使用`vbo`决定是否通过`glBindBuffer`绑定vbo对象，然后调用`glDrawArrays`绘图。

### update()
更新旋转值

## Scene: texture
主要测试不同纹理滤波方法

### 可选参数
- model: 默认cube
- texture-filter: 纹理过滤的方法，默认nearest
    - nearest
    - linear
    - linear-shader
    - mipmap
- texture: 使用的纹理，默认crate-base
- texgen: 是否需要生成纹理坐标，默认false

### setup()
根据选择的纹理过滤方法定义min_filter和mag_filter

根据是否选择生成纹理坐标、是否使用双线性过滤加载shader，并向vertex shader传入光源位置与材料漫反射参数等常量
- vertex shader
    - 不生成纹理坐标：light-basic.vert
    - 生成纹理坐标：light-basic-texgen.vert
- fragment shader
    - 非线性过滤：light-basic-tex-bilinear.frag：像素颜色=顶点颜色*纹理颜色
    - 线性过滤：light-basic-tex.frag: 像素颜色=顶点颜色*纹理颜色，纹理颜色线性过滤而来
    
加载模型，特定模型需要进行特定旋转，并根据需要计算模型法向量和纹理坐标

将model转换为mesh，本case中默认使用vbo

计算透视投影矩阵

### draw()
调整model_view

激活和绑定纹理

调用 render_vbo进行绘图

### update()
同其他，更新旋转值

## Scene:shading
主要测试不同着色模型

### 可选参数
- model: 默认cat
- shading: 选择着色模型，默认gouraud
    - gouraud
    - blinn-phone-inf
    - phone
    - cel
- num-lights：场景光源的数量，默认为1（只针对phone光照模型）

### setup()
为blinn-phone模型计算half-vector

根据选择的shading方式加载不同的shader，并向shader传入常量
- gouraud shading
    - light-basic.vert， 传入光源位置和漫反射参数
    - light-basic.frag
- blinn-phone-inf shading
    - light-advanced.vert
    - light-advanced.frag，传入光源位置和光源half vector 
- phone shading
    - light-phong.vert
    - light-phong.frag，注意这里要根据光源数量依次传入光源位置和颜色，并传入漫反射参数
- cel shading
    - light-phong.vert
    - light-cel.frag
    
加载模型，旋转、计算法向量等，同上

将model转换为mesh，本case中默认使用vbo

计算透视投影矩阵

### draw()
调整model_view

将ModelViewProjectionMatrix, NormalMatrix, ModelViewMatrix加载到shader中

本case中没有使用纹理

调用render_vbo进行绘图，默认使用vbo

### update()
同其他case， 更新旋转值

## 以下部分相似内容不再重复

## Scene: bump
主要测试对同一模型的不同渲染方式

### 可选参数
- bump-render：bump的渲染方式，默认off
    - off
    - normals
    - normals-tangent
    - height
    - high-poly

### setup()
此用例不可选择model，没有加载model列表，默认使用小行星模型

根据选择的render方式加载不同的shader，计算half-vector，并向shader传入相应常量

- off或high-poly（high-poly与其他的区别是使用的是asteroid-high模型
    - bump-poly.vert，提供点的位置和法线信息
    - bump-poly.frag，传入光源位置，光源half vector
- normals
    - bump-normals.vert，提供点的位置和纹理信息
    - bump-normals.frag，传入光源位置，光源half vector
    - model:asteroid-normal-map
- normals-tangent
    - bump-normals-tangent.vert，提供点的位置，纹理，法线和切线信息
    - bump-normals-tangent.frag，传入光源位置，光源half vector
    - model:asteroid-normal-map-tangent
- height
    - bump-height.vert，提供点的位置，纹理，法线和切线信息
    - bump-height.frag，传入光源位置，光源half vector，纹理x方向step和纹理y方向step
    - model：asteroid-height-map

将model转换为mesh，本case中默认使用vbo

### draw()和update()同上

## Scene: effect2d
主要测试不同的卷积核对2D图像卷积的影响（通过创建实现2D图像卷积的fragment shader）

### 可选参数
- kernel: 卷积核，默认"0,0,0;0,1,0;0,0,0"
- normalize: 是否归一化给定的卷积核

### setup()
加载shader
- effect-2d.vert，提供每个顶点的position，线性映射到纹理的坐标
- effect-2d-convolution.frag，通过变量替换的方式在setup()中进行了卷积计算的代码，提供TextureStepX和TextureStepY

### draw()
本样例就是绑定固定的纹理图片进行2D卷积

## Scene: pulsar
主要测试alpha透明混合处理（即半透明场景的展示）

### 可选参数
- quads: 场景中方形纸片的数量，默认为5
- texture: 是否渲染纹理，默认为false
- light: 场景中是否有光照，默认为false
- random: 是否随机模型的旋转速度，默认为false

### setup()
禁用后脸消除(back-face culling)

启用alpha透明混合(alpha blending)

进行颜色混合但不改变目标像素的alpha值：glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

为每个quad设定一定的旋转速度（根据是否随机旋转速度而不同）

加载shader
- vertex shader
    - 添加光照：pulsar-light.vert，需要以常量形式传入光源位置
    - 不添加光照：pulsar.vert
- fragment shader
    - 添加纹理：light-basic-tex.frag
    - 不添加纹理：light-basic.frag

创建并setup mesh，向vertex shader传入顶点的位置，颜色，纹理坐标和法向量信息
- 此case直接手动创建一个quad包括顶点位置，颜色，纹理，法向量等，而不是从model文件中读取，在create_and_setup_mesh()中完成

### update()
对每一个quad用其特定的旋转速度乘所过时间得到这一帧该quad的旋转角度

### draw()
对每个quad加载modelViewProjectMatrix，如果启用了光照还得加载normalMatrix

默认使用render_vbo

## Scene: desktop
主要测试透明物体的模糊效果和阴影效果

### 可选参数
- effect：场景使用的效果，默认为blur
    - blur，使透明的window具有模糊效果
    - shadow，使透明的window具有阴影效果
- windows：场景中出现的移动窗口的个数，默认为4
- window-size：场景中出现的移动窗口的大小，默认为0.35(0-0.5)
- passes：effect passes的数量，默认为1
- blur-radius：blur效果的半径（单位为像素），默认为5
- separable：是否使用可分卷积实现blur效果，默认为true
- shadow-size：阴影效果的大小，默认为20（单位为像素）

### setup()
设置透明效果

分别对screen和desktop执行setup()，此步骤中调用多级init()，主要工作是创建FBO并绑定texture，进行shader加载
- vertex shader：desktop.vert，主要工作是依次将将vec2转换为vec4，并设置z为0，阿尔法值为1
- fragment shader
    - blur效果：desktop-blur.frag， 使用卷积获得模糊效果
    - shadow：desktop.frag， 进行texture坐标处理
    
创建场景中的窗口并设置移动速度，并使移动方向能偏转一定的角度，使得window不止在X或Y方向上移动

### update()
对每一个window根据其运动速度和方向进行位置的更新，并检测window是否到达边界处，如果已经到达边界处则改变速度方向使其调转

### draw（）
glClearColor()确保获得透明效果
进行render渲染

## Scene: buffer
主要测试模型边缘抗锯齿效果

### 可选参数
- interleave：Whether to interleave vertex attribute data，默认false
- update-method：vertex buffer obejct方法
    - map
    - subdata
- update-fraction：每次画面更新的mesh长度百分比，默认为1（0.0 - 1.0)
- update-dispersion：每次画面更新的分散程度，默认为0(0.0 - 1.0)
- columns：纵向网格的细分数，默认为100
- rows：横向网格的细分数，默认为20
- buffer-usage：vertex buffer buffer的使用方式，默认为static
    - static
    - stream
    - dynamic

### setup()
解析传入参数

根据传入的网格信息创建wavemesh
- 单个mesh长度和宽度默认为5，2
- 加载shader
    - buffer-wireframe.vert
        - 计算出待处理点所处的三角形的三边向量
        - 计算出当前点距三边的距离，传入fragment shader
        - 这里先预乘pos.W，在fragment shader中再乘它的逆将此效果消除，原因是openGL执行的是perspective-correct interpolation（除pos.W)
        但我们想要的是线性插值
    - buffer-wireframe.frag: 某个位置像素的颜色是当前所属三角形颜色、距离最近的边线颜色和距边线距离的加权混合
- 要依次向vertex shader传入当前待处理点的坐标和其所属的三角形网格三点的位置

### draw()
加载ModelViewProjectionMatrix

使用render_vbo进行渲染

## Scene:ideas
主要测试多物体与移动光源场景的阴影渲染效果？

### 可选参数
- speed:场景运行速度，默认为duration
    - duration: 将速度作为场景运行duration的函数，speed = (CYCLE_TIME_ - START_TIME_) / duration
    - 1.0: wall clock
    - \> 1.0: faster
    - < 1.0: slower

### setup()
对场景中的各个物体（桌面、logo等）进行初始化（这些写在/src/scene-ideas文件夹下）
- 初始化灯光位置
- 初始化table
    - 为渲染table的program加载shader，考虑灯光效果并随时间fade
        - ideas-table.vert
        - ideas-table.frag
    - 为渲染paper的program加载shader，考虑灯光效果并随时间fade
        - ideas-paper.vert
        - ideas-paper.frag
    - 为渲染texture的program加载shader，文本随时间而fade
        - ideas-text.vert
        - ideas-text.frag
    - 为table以外背景着色的program加载shader，直接渲染为黑色
        - ideas-under-table.vert
        - ideas-under-table.frag
    - 将文本的各个字母初始化（字母都是分别渲染的）
    - 初始化vertex data buffer 和 index data buffer
- 初始化logo
    - 为渲染logo主体的program加载shader
        - ideas-logo.vert
        - ideas-logo.frag
    - 为渲染logo平面部分的program加载shader
        - ideas-logo-flat.vert
        - ideas-logo-flat.frag
    - 为渲染logo阴影部分的program加载shader
        - ideas-logo-shadow.vert
        - ideas-logo-shadow.frag
    - 初始化vertex data buffer 和 index data buffer
    - 初始化logo将使用的texture
- 初始化light
    - 为渲染灯光的program加载shader
        - ideas-lamp-lit.vert
        - ideas-lamp-lit.frag
    - 加载无光照场景的shader
        - ideas-lamp-unlit.vert
        - ideas-lamp-unlit.frag
    - 初始化vertex data buffer 和 index data buffer
    

重置时钟
    
依据传入参数设置speed，参数为duration时设置如上

更新投影矩阵（此后每一帧更新均再执行update_projection）

### update()
更新时间和投影矩阵

### draw()
开始时先根据currentTime获取各object（包括桌面、灯光、logo等object）当前所应处于的位置

根据灯光位置计算阴影效果并渲染

依次调用各object的draw()进行相应的绘制，各object具体实现都在src/scene-ideas文件夹下

## Scene: jellyfish
主要测试Fresnel效果和颜色色散
### 可选参数
无

### setup()
加载水母模型：/models/jellyfish.jobj

初始化dataMap（vertex data在缓冲区中的排布形式）

加载shader
- jellyfish.vert：顶点参数有位置、法向量、颜色、纹理坐标等信息，此外还有光照信息、当前时间等uniforms，顶点运动速度是当前时间的线性函数
    - 计算顶点动画
    - 计算漫反射效果
    - 计算环境光（顶部）
    - fresnel效果
    - 纹理坐标
- jellyfish.frag：像素颜色是环境光加漫反射加焦散效果加本身颜色加透明度的混合

初始化vertex data buffer 和 index data buffer

初始化纹理，包括主体纹理（jellyfish256）和caustics纹理（jellyfish-caustics-n）

### update()
更新viewport和时间信息（用于vertex shader中顶点动画）

### draw()
加载时间、光照信息（漫反射、环境光、Fresnel）、投影矩阵等全局参数，进行渲染



## 1.11 Task List
从多方面整理**glmark2 和 heaven**、**zink 优化前和优化后**两个方面的差别，整理成文档或表格，包括：
- 性能信息，即优化前后使用两个测试工具的跑分分数（包括详细数据）
- 接口调用差别，即使用打点工具跑出来的统计结果，给出核心过程的时间占用区别，包括
  - SwapBuffer (Flush)
  - 着色器编译 (zink_shader_compile)
  - VBO、VAO申请和绑定 (_mesa_BufferData, _mesa_BindData等)，可能是Draw的子过程
  - DrawArrays
  - 纹理加载和映射 (接口还得找一下)
- 其他统计信息，现在可以想到的包括
  - Buffer申请大小统计 (例如每次绑定VAO、VBO传入的大小参数，按照帧数和时间进行单位帧数和单位时间统计)
  - 纹理使用情况 (TODO)

完成以上工作，现在已经有的工具包括：
- 打点工具，可以通过直接源码中插入代码实现接口时间统计，可以通过对trace进行调用栈分析和接口使用情况统计
- gdb调试工具，可以直接通过上层接口自动找出下层调用栈，方便寻找打点位置
- 部分版本的测试结果，包括master分支、zink-wip分支的不同版本，使用不同GPU，在不同测试工具下的结果，详见`1.8优化前后跑分对比.xlsx`
- OpenGL全流程接口调用逻辑，详见`zink_trace_mapping_20201225_2309(1).vsdx`

缺少的东西
- `zink-wip`分支无法在AMD显卡上稳定运行`heaven`，造成无法统计部分结果，需要拉取下最新的更新多尝试一下
- `master`分支只有少数新版本可以正常运行`heaven`，且实际效果优于之前统计，找一找有没有结果更靠谱的版本
- 打点工具功能缺失或不完善
  - 缺少传入额外参数的接口，由于存在跨平台编译，需要解决GL变量类型如何传入的问题
  - 缺少对错误调用栈的容错性，对于多线程结果无法正确统计，调用栈分析逻辑可能需要改一下
  - 缺少对于着色器编译过程、纹理加载和映射过程的相关打点。

### 纹理接口
- glGenTextures, _mesa_GenTextures: 生成对象，不包含申请数据存储空间
- glBindTexture, _mesa_BindTexture：绑定到状态机上，OpenGL所有操作都是对状态机进行操作，想要调用接口进行一些操作就需要先把对象绑到状态机上。
- glTexParameteri, _mesa_TexParameteri：类似的还有其他`glTexParameter`前缀的接口。为纹理贴图设置各类属性，采样过滤之类。这类接口的作用对象就是状态机上绑定的贴图，所以要先有绑定操作。
- glTexImage2D, _mesa_TexImage2D: 类似的接口还有`1D`, `3D`。实际导入数据的接口，参数中会有数据和贴图尺寸。
- 最后需要在调用一次`glBindTexture`把空对象绑定上去实现解绑。

### 纹理映射接口
TODO