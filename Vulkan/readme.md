# 部分资料
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Api List](https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html)
- [Learning Modern 3D Graphics Programming](https://paroj.github.io/gltut/)
- [Vulkan Specification](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html)

# 本仓库文档说明
- [basic.md](basic.md)：Vulkan基本知识，包括Vulkan的核心概念和设计理念。
- [drawing_triangle.md](drawing_triangle.md)：完成绘制三角形基本程序的过程。
- [gallium_run_vulkan.md](gallium_run_vulkan.md)：Zink如何将Vulkan作为Gallium架构的底层来实现OpenGL，包含OpenGL的设计逻辑、Vulkan与OpenGL的核心区别、Gallium架构的设计理念以及如何充当OpenGL与Vulkan的中间层。
- [vertex_buffer.md](vertex_buffer.md)：Vulkan中Vertex Buffer的详细使用方法。

# 安装注意事项
# 一些思路和想法
## 优化Vertex Buffer类型
Vulkan中VertexBuffer的使用是十分细化的，这体现在两个方面
- 一方面面对应用层，在创建`VkBuffer`对象的时候，需要通过`VkBufferCreateInfo`指定应用对缓存的需求。
- 另一方面面对硬件层，不同的硬件可以提供的memory类型也是不同的，在为`VkBuffer`对象实际分配内存的时候，需要根据`VkBuffer`需求**查询**得到硬件能够提供的内存类型，然后使用满足要求的。

在上述过程中，之所以需要有**查询**步骤，一定程度上就是因为硬件能够提供的memory类型是不一样的。但是当硬件确定时，该过程是可以通过简单的映射来做到的。即可以较少的考虑兼容性，而专注于效率。

另外如果想要让`VkBuffer`可以被多个不同的Queue中的指令共享，需要设置`sharingMode`为`VK_SHARING_MODE_CONCURRENT`，而实际上有些只有一个队列中的指令会用到的`VkBuffer`是可以直接使用`VK_SHARING_MODE_EXCLUSIVE`的，有没有可能在Zink中对合适的`VkBuffer`设置`VK_SHARING_MODE_EXCLUSIVE`。