# Gallium 架构如何运行 Vulkan
基本的逻辑是，Gallium提供了Gallium状态机，作为OpenGL状态机的一种实现，从而可以正确的响应OpenGL接口对于状态机修改的需求。

但是Vulkan不是依赖于状态机的，而是依赖于指令队列和指令缓冲来完成绘制任务。

**OpenGL的运行逻辑：**
- 完成对状态机的初始化
- 调用接口对状态机进行修改
- 绘制，即让GPU根据状态机当前的状态进行渲染和结果往framebuffer的写回
- 帧交换，保证前景帧绘制完成，可以继续对状态机进行新的修改

**Vulkan的运行逻辑：**
- 完成对instance（Vulkan库本身）、Device（虚拟硬件）、Queue（虚拟队列）、surface（显示扩展）、swap chain（帧缓冲扩展）的初始化。最后两者可以不初始化。
- 构建用于提交的Command Buffer
  - 完成Render Pass配置，指明渲染中间过程的格式
  - 完成Pipeline的Fix Function部分
    - 节点输入
    - 输入属性
    - 视角和视窗
    - 光栅化
    - 采样
    - 深度和模板测试
    - 颜色渲染
    - 可变管线步骤配置
    - Uniform变量配置
  - 完成Pipeline的Shader部分
  - 完成`VkPipeline`的组装
  - 完成FrameBuffer的创建，与Render Pass对应
  - 从Command Pool中申请新的Command Buffer
  - 将Render Pass 和 Pipeline 绑定到 Command Buffer上
  - 声明Command Buffer可以绘制
  - 声明Command Buffer停止
- 将Command Buffer提交到正确的Queue
- 将渲染结果还给Swap Chain，并且继续索要下一个Image

对于OpenGL来说，绘制接口实际上是围绕着状态机的修改来进行组织的。而对于Vulkan来说，绘制接口是围绕Command Buffer的构建来进行调用的，最终目的是完成Render Pass（包含了渲染对象、中间过程缓存等信息）和Pipeline（包含了实际渲染过程信息，即得到和使用Render Pass）的构建，并且将这些组织成Command Buffer提交到队列中。

由此可见，两类接口并不能进行严格的映射。而使用Gallium架构实现的Zink采用的总体策略是：
- OpenGL初始化 - Vulkan初始化，借助Device的查询接口为OpenGL状态机提供硬件信息，同时由于OpenGL不支持无绘制调用，所以需要完成surface 和 swap chain 扩展的初始化。
- OpenGL调用状态机修改函数 - 修改Gallium状态机
- OpenGL调用绘制指令 - 根据Gallium状态机的情况，完成整个Command Buffer的构建，提交到队列中执行。
- OpenGL调用刷新指令 - 借助Swap Chain完成显示和新的Image的获取