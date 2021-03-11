# 部分资料
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Api List](https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html)
- [Learning Modern 3D Graphics Programming](https://paroj.github.io/gltut/)
- [OpenGL Api Reference](https://www.khronos.org/registry/OpenGL-Refpages/gl4/)
- [Gallium3D Documentation](https://dri.freedesktop.org/doxygen/gallium/index.html)
- [Freedesktop Wiki For Gallium3D](https://www.freedesktop.org/wiki/Software/gallium/)
- [Vulkan Specification](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html)

# 本仓库文档说明
- [basic.md](basic.md)：Vulkan基本知识，包括Vulkan的核心概念和设计理念。
- [drawing_triangle.md](drawing_triangle.md)：完成绘制三角形基本程序的过程。
- [gallium_run_vulkan.md](gallium_run_vulkan.md)：Zink如何将Vulkan作为Gallium架构的底层来实现OpenGL，包含OpenGL的设计逻辑、Vulkan与OpenGL的核心区别、Gallium架构的设计理念以及如何充当OpenGL与Vulkan的中间层。
- [vertex_buffer.md](vertex_buffer.md)：Vulkan中Vertex Buffer的详细使用方法。
- [uniform_buffer.md](uniform_buffer.md)：Vulkan中Uniform变量的详细使用方法。
- [texture_mapping.md](texture_mapping.md)：Vulkan中纹理映射的详细使用方法。
- [shader.md](shader.md)：着色器相关知识。

# 安装注意事项
# 一些思路和想法
## 优化Vertex Buffer类型
Vulkan中VertexBuffer的使用是十分细化的，这体现在两个方面
- 一方面面对应用层，在创建`VkBuffer`对象的时候，需要通过`VkBufferCreateInfo`指定应用对缓存的需求。
- 另一方面面对硬件层，不同的硬件可以提供的memory类型也是不同的，在为`VkBuffer`对象实际分配内存的时候，需要根据`VkBuffer`需求**查询**得到硬件能够提供的内存类型，然后使用满足要求的。

在上述过程中，之所以需要有**查询**步骤，一定程度上就是因为硬件能够提供的memory类型是不一样的。但是当硬件确定时，该过程是可以通过简单的映射来做到的。即可以较少的考虑兼容性，而专注于效率。

另外如果想要让`VkBuffer`可以被多个不同的Queue中的指令共享，需要设置`sharingMode`为`VK_SHARING_MODE_CONCURRENT`，而实际上有些只有一个队列中的指令会用到的`VkBuffer`是可以直接使用`VK_SHARING_MODE_EXCLUSIVE`的，有没有可能在Zink中对合适的`VkBuffer`设置`VK_SHARING_MODE_EXCLUSIVE`。

## 优化Pipeline使用
Vulkan对于每一帧的绘制，都需要创建相应的Command Buffer，并且向其中装入Pipeline。

Vulkan的Pipeline非常固定，任何渲染过程的修改都需要创建新的Pipeline对象。

可以通过`VkPipelineCache`为管线部分过程创建缓存，达到复用的目的。但是如何进行复用，本身是高度定制化的，Zink对Vulkan管线复用能力的应用程度还有待调研。**补充：pipeline cache就是用来加速shader加载的，因为shader即使是spirv这样底层的代码了，执行对应gpu的shader代码还是需要一趟编译变为gpu专用的最优内容，为了节约这个编译时间就搞出了pipeline cache。**

## 优化资源复用
Vulkan中Command Buffer、Semaphore等资源是可以复用的，调研当前Zink对这些资源的复用程度和通常的Vulkan程序的复用程度。

## 减少GPU闲置
首先得有评估GPU闲置情况的方法。

## 优化 batch 迭代逻辑
当前Zink用hash table维护使用中的batch_state，但是在同步机制的协作下不应该是这么笨的方法来维护才对。示例程序中通过Fence的使用，用一个大小为`MAX_FRAMES_IN_FLIGHT`的数组维护了in flight的资源，zink也可以用同样的机制实现才对。

分析hash在zink中的必要性。

## 优化 render_pass 创建逻辑
`get_render_pass`会在每个batch中调用，而在该函数中每次都会对format等参数进行复杂的查询，按道理这是不需要的。

## 其他莫名其妙的问题
```cpp
if (batch->state->work_count[0] + batch->state->work_count[1] >= 100000)
      pctx->flush(pctx, NULL, 0);
```
这是`zink_draw_vbo`函数中的最后一行，虽然不知道为什么但我觉得这行傻逼一样的代码必有问题。

```cpp
/* flush anytime our total batch memory usage is potentially >= 1/10 of total system memory */
if (ctx->batch.state->resource_size >= screen->total_mem / 10)
      flush_batch(ctx, true, false);
```
在超过系统内存1/10的时候刷新...好像也没什么问题，但是正常来说是这样决定了的吗...大场景岂不是一直刷新。而且这个函数在每次调用draw的时候都会被调用一次，可能造成大量的刷新。

而且除了draw的时候，在执行compute program的时候也会进行这个检查。其实一定程度上可以理解这样做的原因，对于zink来说，是完全不知道绘制相关的语义信息的，哪一部分指令是在绘制一帧，哪一部分指令不属于这一帧，这样的信息都是没有的，所以会出现这种自行判断flush时机的需求。但是正常的逻辑难道不是**限制Frame in flight的数量吗！Fence难道不是用来干这个东西的吗！**

**为什么draw_vbo的流程都会受到图元类型的影响？难道不是映射到vulkan中对应的图元类型，或者设定正确的offset、step，然后绘制就行了吗。**

**TODO:** 在draw_vbo函数中不同的分支加入判断，验证不同效率的绘制场景中走到哪几种分支，是否使用了primitive restart或者primitive convert。以及是否进行了大量由于内存不足而进行的flush。

```cpp
if (ctx->gfx_pipeline_state.primitive_restart != !!dinfo->primitive_restart)
      ctx->gfx_pipeline_state.dirty = true;
```
`!!`这样的东西作用在整型上面的时候可以保证整型和bool类型比较的正确性，但是这里是俩bool啊。

不管怎么说Zink里用的hash table也忒多了...而且很多是直接用地址作为key，换句话说啥都存。

```cpp
if (vkCreateFramebuffer(screen->dev, &fci, NULL, &ret) != VK_SUCCESS)
      return;
```
你们zink都是这么做错误处理的？

## TODO
- Gallium中的Streamout是什么东西？Stream在Gallium架构中的作用是什么？
- zink结构体和pipe结构体之间互相转换的原理是什么？
- 函数指针的绑定方式，像`zink_descriptors_update`这样的函数是怎么绑上去的？