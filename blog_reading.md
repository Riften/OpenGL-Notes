# 博客阅读
这里的博客顺序也是逆序的，最上面的博客是最新的。

## [Between Loads](https://www.supergoodcode.com/between-loads/)
对21.1 Release进行了简单的说明，对近几个月zink-wip的进展进行了综述。是一个综述性质的博客。

关于 Master 21.1：
- 表示其包含了对GL4.6、ES3.1的特性支持，并且包含了一部分优化点。
- 其中**不包含**近期针对3A游戏正常运行所做的修改。
- 总体上21.1中的zink更新时基于5-6个月之前的zink-wip版本，加上一些零星的后续修改，拼凑出的一个更新。

关于 zink-wip:
- 提到了[对Linear Scanout的修改](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/10180)，在Phoronix benchmarks等场景极大提升了效果。但是**该修改已经放入了21.1版本**。
- 对海量bug的修复，主要是为了运行3A游戏进行的修复。

### 对`alpha-to-one`格式的支持
`alpha-to-one`相当于忽略透明度通道，例如RGBX、BGRX，然而Vulkan中缺乏对相应格式的支持，所以方法是在采样时直接将alpha通道强制采样为最大值。

而当在用RGBX格式的FBO进行blending operation的时候，会有额外的问题出现。这里的blending operation即如何混合两个FBO中的颜色，例如`VK_BLEND_OP_ADD`混合模式定义的就是将两个FBO的alpha值乘上权重系数之后相加`As0 × Sa + Ad × Da`。这种时候，并不能像前面那样直接通过修改采样策略来保证RGBX正确性（因为这是color blend，而不是texture sample过程）。

最终采取的策略是通过Vulkan中的颜色混合参数直接控制混合操作本身，而不是修改数据。具体方法是，通过额外的标志位`BITFIELD_BIT`记录当前render pass中每个attachment（FBO）是否是`alpha-to-one`，然后在draw过程中根据该值对`pAttachment.dstAlphaBlendFactor/srcColorBlendFactor/dstColorBlendFactor`的值进行修改，而不是使用原本管线中记录的attachment属性。

这些修改通过了piglit `spec@arb_texture_float@fbo-blending-formats`测试项。

## [Sparse](https://www.supergoodcode.com/sparse/)
该博客主要是分析了Zink在对Sparse Buffer实现上的潜在优化点（**应该还没有实际执行这个优化**）

Sparse Buffer可以理解为并不要求被完全实际分配的buffer，可以通过该机制得到大于GPU实际显存的buffer。

Sparse Buffer无法直接支持Map操作（Map操作的一个很重要作用就是通过Host、Device的内存地址映射，从而从Host端读写Device内存。Sparse
Buffer不支持的原因我个人理解是因为Sparse Buffer本身都不一定连续，而且其在实际内存中的驻留情况也可能是变化的，自然也无法进行地址映射）。所以当需要对Sparse Buffer进行Host端读写的时候，往往需要中间的`Staging Buffer`协助。

Gallium中对Sparse Buffer提供支持的接口是resource_commit
```cpp
   /**
    * Change the commitment status of a part of the given resource, which must
    * have been created with the PIPE_RESOURCE_FLAG_SPARSE bit.
    *
    * \param level The texture level whose commitment should be changed.
    * \param box The region of the resource whose commitment should be changed.
    * \param commit Whether memory should be committed or un-committed.
    *
    * \return false if out of memory, true on success.
    */
   bool (*resource_commit)(struct pipe_context *, struct pipe_resource *,
                           unsigned level, struct pipe_box *box, bool commit);
```
该接口我个人理解是通过告知底层驱动 “我要使用Sparse Buffer的box段了” 而让底层驱动去修改实际的显存驻留情况，决定将哪段显存用作box。

zink中对该接口的实现是`zink_resource_commit`，其核心目标是调用Vulkan提供的`vkQueueBindSparse`接口来完成Sparse Buffer Commit。但是当前的实现有以下潜在可优化的地方。

- 对Semaphore使用的优化。当前的使用逻辑如下
  ```cpp
  /* if any current usage exists, flush the queue */
   if (zink_batch_usage_matches(&res->obj->reads, ctx->curr_batch) ||
       zink_batch_usage_matches(&res->obj->writes, ctx->curr_batch))
      zink_flush_queue(ctx);

   VkBindSparseInfo sparse;
   sparse.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
   sparse.pNext = NULL;
   sparse.waitSemaphoreCount = 0;
   sparse.bufferBindCount = 1;
   sparse.imageOpaqueBindCount = 0;
   sparse.imageBindCount = 0;
   sparse.signalSemaphoreCount = 0;
  ```
  简单说就是，不使用任何Semaphore，而是在提交sparse指令之前，先检查resource使用，如果当前batch对resource有任何读写操作，都直接进行flush来保证当前batch对resource使用完成。这造成了多余的flush，正常的方法应当是使用Semaphore同步对该资源的使用。
- 在对Sparse Buffer进行Map/Unmap的时候，通过添加Sparse Buffer相关的标志位，从而控制对于Staging Buffer的使用（**看这么说，Sparse Buffer的Map/Unmap应该是还没完全实现**）。另外尝试使用device-local的memory类型而不是host-visable的memory类型作为Sparse Buffer的Memory。

## [You Want It I Got It](https://www.supergoodcode.com/you-want-it-i-got-it/)
对mesamatrix中Zink尚不支持的特性在zink-wip中的进展状况进行了一个整理。

## [Not A Prank](https://www.supergoodcode.com/not-a-prank/)
简介合并21.1过程中的几个合并内容。

最核心的`Thread Context`已经在21.1支持，在部分场景可以达到native的60%~70%。

另外还提到了像furmark一类benchmark中的alpha blending使用极大地降低了表现，这应该和`Between Loads`中提到了`alpha-to-one`实现有关系。

`Timeline Semaphore Handling`也已经提交到21.1，主要是对CPU开销的优化。但是这里没有对其具体实现进行介绍。

## [A Reminder](https://www.supergoodcode.com/a-reminder/)
简单介绍了Vulkan中Array Type和Image Type进行accessing/copying/blitting等操作是所访问的数据对象不同。

## [Description](https://www.supergoodcode.com/description/)
基于Descriptor Templates的Descriptor Update和Cache策略。在Vulkan中Descriptor是用于提供Uniform变量以及纹理资源的核心类。

之前Descriptor Update策略是完全基于Cache的，但是对Cache的维护和更新给CPU带来了更大的开销，而且大多数场景下，Descriptor Cache实际上都很难发挥其缓存作用，写入管线的Descriptor在Host端实际上没有什么复用的需求。

基于此特性，在之前的优化中zink-wip就提供了lazy mode来用非caching的方法进行Descriptor Update。相应的函数是`src/gallium/drivers/zink/zink_descriptors_lazy.czink_descriptors_update_lazy`，表现优于基于cache的方法，虽然在绝大多数场景里都不明显。

值得注意的是Descriptor Cache相关的优化在GPU瓶颈的场景中（Heaven等绝大多数场景）优化表现都不明显。

## [Layin That Pipe](https://www.supergoodcode.com/layin-that-pipe/)
Lavapipe 相关优化。

## [A More Accurate Update](https://www.supergoodcode.com/a-more-accurate-update/)
对一段时间的优化工作的总结
- 对Zink现有优化进行精简
- Descriptor Caching
- zink now has lavapipe-based CI jobs going to prevent regressions（不知道是个啥）
- 引入了buffer invalidation机制，避免了一些会让GPU挂起的情形，对表现有提升。

（其他和优化关系不大）

## [Getting Back In](https://www.supergoodcode.com/getting-back-in/)
对Unigine Superposition测试有时出错的问题进行了定位和修复，早期版本的zink似乎是可以的。

对flush和wait fence机制进行了简单的介绍，后续可以详细看一下。

## [Delete The Code](https://www.supergoodcode.com/delete-the-code/)
对NIR变量相关的代码必要性进行了分析，结论是，很多代码是无用的，可以忽略。

## [Notes](https://www.supergoodcode.com/notES/)
解释了下支持ES 3.2特性的难点，简单说，暂时不做。

## [Roadmapping](https://www.supergoodcode.com/roadmapping/)
对接下来一个月的优化点进行了计划
- 提升barrier使用（从后续blog看并没有实质性进展）
- 减少绘制时的pre-fencing（前面Sparse Buffer的Commit应该是比较典型的pre-fencing）
- descriptor caching
- 对架构带来的bug尝试修复

当前正在做的feature
- 着重CTS相关的bugfix

## [Two With One Blow](https://www.supergoodcode.com/two-with-one-blow/)
对Stream游戏支持的一个综述。

## [Nv](https://www.supergoodcode.com/nv/)
对Nvidia的一些基本支持