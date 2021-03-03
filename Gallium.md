# Gallium
## 总览
早在09年就已经提出，本质上是一个构建图形驱动程序的体系结构。需要注意的一点是，Gallium3D本身并不仅仅可以用于实现OpenGL接口，它是一个通用的图形驱动程序架构。

在Mesa（OpenGL）中，通过 State Tracker 将指令传递给 Gallium3D（Zink），然后 Galluim3D 根据自己的架构实现去执行指令。这也是为什么 Zink 算是软件驱动的一种。

通常来说，一个Gallium架构的图形驱动程序在整个系统中位置如下所示：

```bash
-----------------------------------
|            Application          |
-----------------------------------
|   OpenGL       |       GLX      |
-----------------------------------
|   OpenGL       |     GLX/DRI2   |
| State Tracker  |  State Tracker |
-----------------------------------
|        Gallium3D Interface      |
-----------------------------------
|        GPU Specific Driver      |
-----------------------------------
|            OS & HardWare        |
-----------------------------------
```

只是在我们的情况中，显卡驱动的功能被 Vulkan承担了。

State Tracker 充当了 Gallium 的前端，负责将标准图形库API翻译成 Gallium 调用。这就意味着，对于任意一种新的设备，只需要关心如何对其实现 Gallium，而不需要关心如何实现 State Tracker。需要注意的是，在我们的情形中，这个所谓的 “新的设备” 指的是 Vulkan。


## State Tracker
OpenGL 2.1 的 State Tracker
- 上层是OpenGL Core（src/mesa/main/）
- 使用了Mesa的 GLSL Compiler（src/mesa/shader/）
- 使用了 OpenGL 的 Vertex Buffer Object（VBO），将 OpenGL 的绘图指令转化成 VBO 形式。
- 最终以设备驱动的形式提供给上层，实现所有 ctx->Driver.Foobar() 钩子函数（src/mesa/state_tracker/）

## 关于GLSL
OpenGL SHading Language，是用于编写GPU程序的语言。

渲染过程中，GPU 相当于 Server，而运行在 CPU 上的程序相当于 Client。为了让 Server 知道怎么处理你传给他的东西，需要提前上传你的 Shader 组件给 Server，这有点像提供一个 dll/so，从而让 Server 可以运行你之后希望调用的某个接口。

早期是使用类似汇编语言的方式编写 Shader 的，Shader Language 就是在这基础上发展的高级语言。

Shader 的使用使得程序员编写 OpenGL 程序的时候，可以参与到渲染管线的设计中，精确控制对象的表现细节。

在`Gallium`架构中，GLSL被新的着色器编程语言TGSI替代。

## Barrier & Fence
Barrier 本意为屏障，Fence 本意为栅栏。他们意思差不多，都是为了保证一些操作按特定顺序进行。之所以用屏障和栅栏这样的词语，是因为这种保序是以**在Barrier之前提交的指令必须保证先于在Barrier之后提交的指令执行**。

一种常见的 Barrier 是 Memory Barrier，用来让多个CPU协同工作（即多线程系统工作）。

# Gallium3D 实现
[Gallium3D Documentation](https://dri.freedesktop.org/doxygen/gallium/index.html)

Gallium3D所有对外接口的定义都在`src/gallium/include/pipe/p_context.h`文件中