# 基本知识
## Vulkan设计理念
和发展时间长、旨在提供便捷API的OpenGL不同，Vulkan面向现代GPU架构实现，提供的API更加针对现代GPU的渲染管线，让程序员能够更加精确的指定GPU行为，同时也减轻Driver开发的负担。也正因如此，Vulkan总体上是**面向对象**的，而不是将渲染过程掩盖在状态机后。

**暴露GPU真正运行情况**是Vulkan的一大设计理念。

*Vulkan中`Vk`开头的是结构体和类，`vk`开头的是函数，`VK_`开头的是常量。*

*Vulkan虽然不同于OpenGL，可以直接拿到特定类型的结构体或者对象，但是许多Vulkan结构体中会包含一个`sType`成员，用于说明结构体类型。这看上去有些没必要，不过这一定程度上是考虑了Vulkan本身的快速迭代特性，[这里](https://stackoverflow.com/questions/36347236/vulkan-what-is-the-point-of-stype-in-vkcreateinfo-structs)对`sType`字段的必要性做了简单的解释。*

*Vulkan许多接口都有一个枚举类型`VkResult`的返回值，该返回值指明了接口执行是否成功以及失败原因等。换句话说，Vulkan本身不是调试友好的，许多调试工作需要设计专门的中间层完成。同样，也不会throw出任何错误，运行时错误需要通过VkResult来手动分析。*

*Vulkan程序需要创建各种类的实例，这些实例一般通过`vkCreateXXX`接口创建。实例的各类属性通常在创建时通过一个`VkCreateXXXInfo`结构体指定。*

*Vulkan中的数组，Vulkan实现自己的slice类，也没有依赖其他库来提供slice相关的接口。所有返回值、参数、成员中的数组都由简单的一个`count`和一个指针来定义。*

*Vulkan中的flag，Vulkan的许多结构体属性或者接口参数中会有flag，在Vulkan中flag通常是以bitmask形式存在的配置项，支持位操作来进行修改。*

## Vulkan渲染流程
### 创建Vulkan Instance
`VkInstance`指明了应用的一些特性，例如使用的Vk扩展等，是一个和应用相关的对象。在此基础上可以查询得到当前支持的`VkPhysicalDevice`。

### 创建VkDevice
对`VkPhysicalDevice`的抽象，包含详细的硬件信息。`VkDevice`的一个核心作用是提供一系列`VkQueue`，Vulkan中，不同的`VkQueue`支持接受不同的指令，对`VkQueue`的支持也一档程度上代表了硬件的能力。

### 创建 surface 和 swap chain
`Vulkan`是不依赖于显示的，如果需要渲染到终端则需要借助单独的WSI（Window Interface System），需要通过`VkSurfaceKHR`对象来管理显示终端，通过`VkSwapChainKHR`实现帧缓冲。需要注意的是，WSI相关接口本身不是Vulkan核心的一部分，而是扩展。

### Image View 和 Frame Buffer
`VkImageView`是图片的一部分，而`VkFramebuffer`是深度、颜色、模板等image view。swap chain中包含多个image，而绘制过程中需要选择正确的绘制对象。

### Render Passes
渲染通道，这个概念和CG中通常说的Render Pass是一致的。可以通俗的理解为渲染的过程是多通道分开渲染，然后叠加成最终结果的，这里的Render Passes就指明了渲染通道的特性。

也可以理解为Image的类型，例如如何理解image中的数据，如何使用其中的数据。FrameBuffer完成了Image和Render Passes之间的绑定关系。

需要注意的是不同Pass并不是完全并行和独立的，Pass之间普遍存在着依赖关系。

Render Pass的存在意义在于，可以让应用程序将一个帧的画面中的高级架构信息传递给驱动，从而让驱动可以更好地决定数据处理的方式。Vulkan中的RenderPass对象包含了一帧画面的结构。

### Graphic Pipeline
通过`VkPipeline`设定GPU渲染管线。许多渲染过程都通过`VkPipeline`来设定，例如视点、深度缓冲、着色器等。

`VkPipeline`承担着两方面的作用：
- 指明流程：即渲染管线的渲染流程如何
- 指明格式：即渲染过程中使用的各类数据的格式如何

`VkPipeline`是一个完整的渲染管线，在提前设定完成后一同提交渲染，所以如果想要修改管线中一些步骤，例如替换着色器，则需要创建新的`VkPipeline`。

对于Vulkan编程架构本身，这并没有什么问题，本身Vulkan各个类的使用也需要编写合适的工具类来处理生命周期等问题。但是当需要把Vulkan的架构打散，例如映射到OpenGL接口，渲染管线的生命周期管理就成了一个不得不解决的问题，实际应用场景中，Pipeline的创建是大量发生的。

### Command Pools 和 Command Buffers
向前面提到的`VkQueue`提交的对象，`VkCommandBuffer`从`VkCommandPool`中分配得到，例如在最简单的绘制一个三角形的程序中，`VkCommandBuffer`中需要有以下操作
- 启动渲染通道（Render Pass）
- 绑定渲染管线（pipeline）
- 绘制顶点
- 结束渲染通道

换言之Command Buffer是非常基础的工具，不是为了实现具体操作，而是每个操作都需要经由的一个途径。某种意义上，和OpenGL中的状态机是一个层级的东西。

### Main Loop
与OpenGL不同，Vulkan需要自己写Mainloop，而不是简单地调用一个接口。当Command Buffer构建好之后，Mainloop需要做以下一些事：
- vkAcquireNextImageKHR：从swap chain中获取下一个绘制的image（的index），实际返回的是哪一个则由swap chain的缓冲策略有关，**mainloop自己不关心缓冲策略**。
- vkQueueSubmit：提交command buffer，绘制image。
- vkQueuePresentKHR：将command buffer绘制的image返回给swap chain。

Vulkan中提交的指令的最终执行是异步的，所以需要额外的锁机制保证特定指令的顺序，在这个场景中需要保证的顺序包括：
- 提交的command buffer中的命令的执行需要在图像获取之后，不能往扔在读取的image中渲染。
- vkQueuePresentKHR需要等待渲染完成，不能把没画完的image扔回给swap chain。

实际接口中完成这种同步机制的是`VkSemaphore`和`VkFence`。

### 总体流程
上面提到的仅仅是完成绘制的最基本概念和流程，而实际的绘制场景中，还需要有分配Vertex Buffer，处理Uniform Buffer，处理贴图等操作。但是这些本质上都是向Command Buffer中填入更多地指令。最基本的绘制流程包括：
- 创建`VkInstance`
- 选取硬件`VkPhysicalDevice`
- 创建硬件接口`VkDevice`和对应的`VkQueue`
- 借助WSI创建surface和swap chain。对应的缓冲策略也在这里设定。
- 将swap chain中的图像打包成可作为渲染目标的`VkImageView`，需要注意的是这里不是设定特定的某个图像，而是设定对于所有图像的使用方式。
- 创建渲染通道render pass，指定渲染目标和目标用处（作为depth，作为模板，作为颜色，之类），这里所谓**指定**的客体也就是刚刚设定好的image view。
- 为render pass创建多个frame buffer。
- 组织渲染管线pipeline
- 为每个image分配command buffer，并且将绘制指令（例如刚刚的render pass，pipeline都是绘制指令）扔进command buffer。
- 从swap chain获取image，提交正确的command buffer，将image返回给swap chain，最终完成绘制。显示则交由swap chain。这里的image和前面提到的image view是不同概念，可以理解为，前面的image view告诉了vulkan应该怎么使用这里的实际image。

## 问题
- Zink怎么解决的swap chain问题？
- Vertex Buffer使用过程中Staging Buffer的原理和作用是什么？为什么Uniform Buffer不再使用Staging Buffer？
- Zink如何利用Vulkan异步特性？以UBO为例，Vulkan每次执行Command Buffer都需要UBO，但是在UBO被使用过程中是不可以对其进行修改的。所以Vulkan的通常做法是对每个Command Buffer创建UBO对象，在绘制同时准备UBO中的数据。通常的Vulkan程序中，利用这种异步特性的方法是，创建多个UBO对象（其他对象也是），swap chain能提供几个image的交替渲染，就创建多少个，这样就能保证瓶颈由swap chain的交替策略决定，而不会发生对象访问冲突造成的错误或者额外等待。