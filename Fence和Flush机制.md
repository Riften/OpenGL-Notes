# Fence 和 Flush机制
## 概念
Fence（栅栏）这里指两部分操作之间的分隔，常用来处理依赖，这里用来处理双buffer的操作先后依赖问题。

核心代码：
```cpp
// mesa/src/gallium/frontends/dri/dri_drawable.c
void
dri_flush(__DRIcontext *cPriv,
          __DRIdrawable *dPriv,
          unsigned flags,
          enum __DRI2throttleReason reason)
{
    // 获取 screen、context、以及依赖信息
    struct st_context_iface *st;
    st = ctx->st;

    ...

    struct pipe_fence_handle *new_fence = NULL;
    st->flush(st, flush_flags, &new_fence, args.ctx ? notify_before_flush_cb : NULL, &args);
    if (drawable->throttle_fence) {
         screen->fence_finish(screen, NULL, drawable->throttle_fence, PIPE_TIMEOUT_INFINITE);
         screen->fence_reference(screen, &drawable->throttle_fence, NULL);
    }
    drawable->throttle_fence = new_fence;

    ...
}
```

## fence_finish
- 声明：`mesa/src/gallium/include/pipe/p_screen.h`, `pipe_screen->fence_finish`
- 函数指针赋值：`mesa/src/gallium/drivers/zink/zink_fence.c`, `zink_screen_fence_init`
- 实现：`mesa/src/gallium/drivers/zink/zink_fence.c`, `zink_fence_finish`
- vulkan 接口调用：
  - `vkWaitForFences`, [接口说明](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkWaitForFences.html), 声明位置`mesa/include/vulkan/vulkan_core.h`

12.5版本之后，对于fence_finish函数有一定的修改，但是这部分修改没有改变效果。

### fence_finish 函数返回条件
`fence_finish`最终会等待`vkWaitForFences`函数返回，这时候会传入`fence`作为其中一个参数。

### fence 的使用
`fence`和zink层的`batch`概念是一一对应的，每一个batch都对应了一个fence。

`batch`和`fence`结构体之间的关系(这部分结构体定义在`src/gallium/drivers/zink/zink_batch.h`和`src/gallium/drivers/zink/zink_fence.h`)：
```
zink_batch
    zink_batch_state
        VkCommandPool
        VkCommandBuffer
        resource_size
        zink_fence
            VkFence
            batch_id
    last_batch_id
```

## 表现
- glmark2 的swapBuffer时间占比远大于 heaven
- glmark2 的drawcall比heaven简单许多许多
- glmark2 有非常严重的屏闪问题，而且在帧数低、和硬件驱动差别小的scene中屏闪尤为严重
- 如果直接在dri_flush函数的开始加wait，屏闪问题依然存在，但是确实会发现fence_finish函数所占用的时间比重大大减少。
- heaven 运行时CPU负载（60%，资源加载时130%）远高于glmark2运行时的CPU负载（简单场景30%~40%）。

## 原因分析

## 其他知识
- Vulkan的主要作用在于降低CPU开销和多线程支持。
- 对于CPU高负载的渲染场景，Vulkan能达到更好地效率。
