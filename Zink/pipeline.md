# Zink中的pipeline
探究Zink中对于Pipeline的构建和使用，核心点在于回答下面几个问题：
- Zink如何构建Vulkan Pipeline？何时构建？
- Zink如何利用Vulkan的Pipeline Cache机制？是否有扩展余地？
- Zink实现了哪些其他机制来优化Vulkan Pipeline？尤其是Shader缓存机制。
- Pipeline和Batch之间是否存在强关联？例如每个batch有自己的pipeline之类。如果是这样，Batch的生命周期和Pipeline的生命周期是否存在关联？
- 已经成为Pipeline重要过程的曲面细分在Zink中是怎样实现的？

## Pipeline在Zink中组成部分
核心结构体：`zink_gfx_pipeline_state`，其中包含了对`VkPipeline`各种创建信息的映射。

## program cache
定义在`zink_context`中的一个hash_table。key为`zink_shader`的数组，data为`zink_gfx_program`。

## TODO List
- [ ] 整理`VkPipeline`与`zink_gfx_pipeline_state`之间的映射关系。
- [ ] 分析`zink_gfx_pipeline_state`生命周期
- [ ] 检查glmark2 case1 运行过程中zink对vulkan的调用情况
  - [ ] 使用validation layer记录接口调用
- [ ] pipe stream out是什么作用？（似乎和geometry shader，vertex transform有关系）
- [ ] pipe(gallium)接口分析

# 补充知识
## Mesa Hash Table
定义在`src/util/hash_table.h/hash_table`，本质上是一个`hash_entry`的array
```cpp
struct hash_entry {
   uint32_t hash;
   const void *key;
   void *data;
};
```
`hash_entry`是table中实际存储的内容，包括一个标明在array中实际位置的hash，和一对key-data。hash是key的哈希值，Mesa Hash Table提供了两类接口
- 插入：将一对key-value插入table，返回一个构建好的`hash_entry`。也可以在插入时指定hash值。
- 检索：根据key检索出一个`hash_entry`。也可以在检索时直接指定hash值。

从这两类接口可以看出，功能上`hash_table`提供的是一个简单的`map(key, data)`的功能，虽然`hash_entry`中有hash值，但是这个值对于工具的使用者来说是没有实际作用的。