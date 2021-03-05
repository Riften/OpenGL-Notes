# Framebuffer
Framebuffer是一个图形库通用的概念，指用来作为渲染目标的一系列buffer。

在OpenGL中，Framebuffer有两类不同的Framebuffer：
- 由状态机提供的Default Framebuffer，通常和窗口或者显示设备直接相关。
- 由用户创建的Framebuffer Objects (FBOs)，不是直接用于显示的Framebuffer，而是充当纹理或者渲染缓存。

## Blit
Blit操作指的是将一个Framebuffer的特定矩形区域内像素数据copy到另一个Framebuffer，在多重采样等功能上也有应用。

Vulkan中Blit通过以下接口实现：
```cpp
void vkCmdBlitImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions,
    VkFilter                                    filter);
```
可以看到Vulkan本身的Blit接口是非常直观的。不过需要注意的是，如果在Pipeline中引入Blit这类操作，通常需要同时引入相应的Semaphore来保证同步性。