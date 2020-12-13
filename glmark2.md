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

## 场景列表
build, texture, shading, bump, effect2d, pulsar, desktop, buffer, ideas, jullyfish

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

## TODO
- 啥叫interleave vertex attribute data?