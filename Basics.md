## 图形学渲染的各个参与模块之间的关系
**A. 软件（如游戏引擎）`--调用-> ` B. OpenGL提供的API `--转化成驱动指令->` C. 显卡驱动 `--渲染指令->` D. 显卡**

项目核心在于，对于Mali GPU，缺乏`B->C`的途径。但是我们有另一个途径

**A. 软件（如游戏引擎）`--调用-> ` E. Vulkan提供的API `--转化成驱动指令->` C. 显卡驱动 `--渲染指令->` D. 显卡**

但是Vulkan是非常新的架构，这就造成了一些旧的代码当需要迁移到Mali GPU上的时候，需要使用Vulkan进行完全的代码重构。本项目就是为了解决这个问题，寻找 `B->E` 的途径，目前找到的技术路线是这样的

**A. 软件（如游戏引擎）`--调用-> ` B. OpenGL提供的API `--实际实现->` F.Zink通过Galium3D架构构建的OpenGL实现 `--调用->` E. Vulkan提供的API `--转化成驱动指令->` C. 显卡驱动 `--渲染指令->` D. 显卡**

即通过 Zink 作为 OpenGL 和 Vulkan 之间的桥梁。需要注意的是，Zink 本身就是 OpenGL 的实现，只不过原本的 OpenGL实现使用的是驱动程序提供的接口，而 Zink 使用的是 Vulkan 提供的接口。这样，从 OpenGL API 的视角来看，Zink + Vulkan + 驱动 替代了原来直接调用硬件驱动，所以 Zink + Vulkan 可以看做是这里的软件驱动，同样是屏蔽了上层对硬件的直接访问。而项目的最终目的，就是两点
- 让 Zink + Vulkan 可以完成软件驱动工作，即可以支持需要的 OpenGL 接口（OpenGL3.X or 4.X）
- 让 Zink + Vulkan 可以达到足够的效果，向着 “就像直接调用硬件驱动一样” 的效果优化

## Mesa 目录结构
Mesa简单说就是OpenGL社区版，通过Zink+Vulkan实现的OpenGL也是在这个版本基础上完成的，且正在进行到主分支的合并。

<!--[Mesa目录结构](https://winddoing.github.io/post/39ae47e2.html)-->