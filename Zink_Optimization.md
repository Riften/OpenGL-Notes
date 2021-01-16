## 2020.7.19 Master分支 Merge Request !5970
[Diffs](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/5970/diffs?diff_id=88719&start_sha=f50311dcc6cb71f75829d2edf16ab966f6deea94#fda2f3a4bf3b7336a48643fa923ec5415c3c51be_62_64)

irc channel: `irc://irc.freenode.net/zink`

主要修改内容：
- 添加对vulkan中`VK_USE_PLATFORM_DIRECTFB_EXT`扩展的支持。[DirectFB Wikipedia](https://en.wikipedia.org/wiki/DirectFB)，核心在于添加了`include/vulkan/vulkan_directfb.h`
- 在`src/gallium/drivers/zink/zink_draw.c`的`draw_vbo`函数中添加了对`zink_batch_reference_program`的调用，后续更新中换成了`zink_batch_reference_resource_rw`

## 2020.10.16 Master分支 Merge Request !7193
[Diffs](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/7193/diffs)

主要修改内容：
- 添加对shader key的支持
- 添加对已缓存gfx program的更新能力

### src/gallium/drivers/zink/zink_compiler.c
- 核心函数`zink_shader_compile`
- 功能：将`nir` shader编译成`spir-v` shader，存入缓存文件中，并返回[VkShaderModule](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkShaderModule.html)对象。
- 注意：Vulkan的IR输入为 spir-v，且不开放直接输入 nir 的接口。OpenGL 的 IR 输出为 nir，且 zink 作为软件驱动层，自然也已 nir 作为 IR 输入，这是 OpenGL 和 Gallium 架构决定的。spir-v 本身是 GLSL 同级的可读高级 IR 语言，所以这里会将结果直接写入缓存文件以供 Vulkan 的渲染管线使用，而不是提交硬件。

为`zink_shader_compile`函数增加`zink_shader_key`类型参数`key`，为shader本身添加无法从`IR`中直接获取的信息，依次实现更细粒度的编译功能。

```cpp
struct zink_fs_key {
   unsigned shader_id;
   //bool flat_shade;
   bool samples;
};

/* a shader key is used for swapping out shader modules based on pipeline states,
 * e.g., if sampleCount changes, we must verify that the fs doesn't need a recompile
 *       to account for GL ignoring gl_SampleMask in some cases when VK will not
 * which allows us to avoid recompiling shaders when the pipeline state changes repeatedly
 */
struct zink_shader_key {
   union {
      struct zink_fs_key fs;
   } key;
   uint32_t size;
};

static inline const struct zink_fs_key *zink_fs_key(const struct zink_shader_key *key)
{
   return &key->key.fs;
}
```

<details>
<summary>后续zink-wip分支对zink_shader_key的扩展</summary>

```cpp
// src/gallium/drivers/zink/zink_shader_keys.h
struct zink_vs_key {
   unsigned shader_id;
   bool clip_halfz;
};

struct zink_fs_key {
   unsigned shader_id;
   //bool flat_shade;
   bool samples;
   bool force_dual_color_blend;
};

struct zink_tcs_key {
   unsigned shader_id;
   unsigned vertices_per_patch;
   uint64_t vs_outputs_written;
};

/* a shader key is used for swapping out shader modules based on pipeline states,
 * e.g., if sampleCount changes, we must verify that the fs doesn't need a recompile
 *       to account for GL ignoring gl_SampleMask in some cases when VK will not
 * which allows us to avoid recompiling shaders when the pipeline state changes repeatedly
 */
struct zink_shader_key {
   union {
      /* reuse vs key for now with tes/gs since we only use clip_halfz */
      struct zink_vs_key vs;
      struct zink_fs_key fs;
      struct zink_tcs_key tcs;
   } key;
   uint32_t size;
};
```
</details>

- **zink_vs_key**：vertex shader key. 包括参数 `clip-halfz`，应该是为了应对某些需要提升提升深度缓冲效率从而需要裁减深度缓冲范围的情况。因为深度缓冲的一个基本问题就是，越近的物体，Z值的采样越多，精度越高，深度缓冲得到的效果越真实。而远的物体（距离 ZFar 更近），为了不使计算量增大，对Z值的采样（通常是顶点Z值的插值）也就相对稀疏，精度也就低。深度缓冲范围对精度的影响在[这个文档](https://www.khronos.org/opengl/wiki/Depth_Buffer_Precision)中有较为详细介绍。
- **zink_fs_key**：fragment shader key.
- **zink_tcs_key**： Tessellation control shader key. [Tessellation control shader](https://www.khronos.org/opengl/wiki/Tessellation_Control_Shader)

zink_shader_key 发挥作用的方式并不是控制编译过程，对`shader`的编译依然是通过同一个接口`nir_to_spirv`完成的。key所控制的是对`nir`的直接修改，通过对`nir`本身进行优化，达到减轻编译工作的目的。

**潜在优化点**：
是否可以直接对 `spir-v` 的编译结果直接进行 cache 和部分重用，而不是通过修改`nir`，来达到提升编译效率的目的？

**难点**：
这部分优化需要对 Shader IR 有非常充分的了解。

### src/gallium/drivers/zink/zink_context.c
对`zink_set_framebuffer_state`进行修改，主要是对状态更新前后 “framebuffer 的采样数是否为1” 这一点是否变化进行了判断，并且以此为依据修改`ctx->dirty_shader_stages`。TODO：作用分析。

另外，该处判断在`zink-wip`分支中进行了进一步完善，直接记录采样数（`rast_samples`，Rasterization Samples）是否发生变化，放在`ctx->gfx_pipeline_state.dirty`里面。TODO：作用分析。

TODO：[glSampleMask](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/gl_SampleMask.xhtml)概念和作用。

### src/gallium/drivers/zink/zink_draw.c
`get_gfx_program`中添加对`zink_update_gfx_program`的调用，之前的处理逻辑中，对于已缓存的program，直接忽略不进行编辑。而现在则是不忽略，转而进行`update`操作。

可以认为除了这里的修改，缓存策略也一定做了修改，允许更多地情况下保留`gfx program`缓存。

### src/gallium/drivers/zink/zink_program.c
在对应头文件中添加了`zink_shader_cache`的声明，从而能够对`shader`进行更复杂的缓存。

```cpp
struct zink_shader_cache {
   struct pipe_reference reference;
   struct hash_table *shader_cache;
};
```

`zink_shader_cache`本身是一个hash表，存储的映射关系为`zink_shader_key::VkShaderModule`。`zink_shader_cache`也和shader列表一样作为了一个`zink_gfx_program`的成员。

对原有`update_shader_modules`函数进行修改。
- 按照pipeline顺序遍历zink_shaders。需要注意的是，所谓的*pipeline顺序*实际上是由shader类型决定的，渲染管线对于不同类型shader的执行顺序是一定的。
  
  ![Rendering Pipeline](imgs/RenderingPipeline.png)
- 

**TODO**：`zink_gfx_program`使用方法。

## Vulkan WSI
[Mike blog: poll()ing For WSI](http://www.supergoodcode.com/poll()ing-for-wsi/)

[Vulkan-Guide/wsi.md](https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/wsi.md)

Window System Integration，窗口集成系统。Vulkan本身是不依赖于窗口系统绘图的，可以在不实际绘制的情况下使用。而真的需要实际绘制的时候，就需要 WSI 扩展来与不同的窗口系统交互。WSI主要包括两部分：

- Surface：一个窗口系统无关的对象，不过创建的时候需要根据实际的 native 窗口系统调用对应的创建接口。
- SwapChain：与Surface交互，实现双buffer等机制。

WSI的一个重要功能就是提供“何时绘制工作完成了”这个信息，而这对于双缓冲机制是非常重要的。

**双buffer机制**：渲染内容写入backbuffer，显示的图片是frontbuffer。


### Zink目前的机制
Zink虽然目的是作为GL和Vulkan之间的软件驱动层工作，却没有集成Vulkan的WSI系统。这就造成，Zink完全依赖一些驱动前端的方法来获取 “何时可以向image resource绘制” 这个信息。

![WSI工作流程](imgs/wsi_setup.png)

## Shader Compile 着色器编译
[Wiki: Shader Compilation](https://www.khronos.org/opengl/wiki/Shader_Compilation)

着色器编译指的是将OpenGL着色器脚本语言（GLSL）加载到OpenGL中用作着色器的过程，编译过程的最终输出是 Program Object。

Shader 又可以按照不同阶段分为
- Vertex Shader
- Tessellation Shader
- Geometry Shader
- Fragment Shader
- Compute Shader

得到的Program Object可能包含多个阶段的Shader可执行代码。最终执行渲染的时候，只能绑定一个 program object，编译完成后 program object 就成为了一个整体。

多个 Shader 编译为一个 Program Object 的过程类似于多个源文件编译为一个可执行程序的过程：
- 创建 Shader `GLuint glCreateShader(GLenum shaderType​);`，这里的 `shaderType` 可以指明上述五种Shader类型。创建之后还需要通过`void glShaderSource(GLuint shader​, GLsizei count​, const GLchar **string​, const GLint *length​);`把源码输入。
- 编译创建的 Shader `void glCompileShader(GLuint shader​);`
- 为创建的 Shader 设置各类属性，例如Vertex shader input attribute locations, Fragment shader output color numbers, Transform feedback output capturing, Program separation.
- 创建并连接 Program。`GLuint glCreateProgram();`创建，`void glAttachShader(GLuint program​, GLuint shader​);`链接。链接过程中，是允许同一个shader stage链接多个shader的，但是有一定限制，而且并不推荐这样做。通常情况下，每个 shader stage 都最多有一个 shader。

OpenGL 允许使用多个独立的 Program Object。

- mesa_compile_shader
  - mesa_glsl_compile_shader

### NIR_PASS_V
- 位置：`src/compiler/nir/nir.h` 宏定义
- 功能：按照传入的钩子函数处理NIR shader

## Lowering Pass
[Mike Blog: aNIRtomy](https://www.supergoodcode.com/aNIRtomy/)

Lowering Pass 指的是在底层对着色器进行简化或修改，这样的修改可能是兼容性的需要，也可以是优化的需要，例如把无用的变量移除。这个修改的过程使用的是在`mesa/src/compiler/nir/nir.h`中的宏定义`NIR_PASS`。宏定义的第二个参数可以理解为一个回调函数，用于进行实际的NIR处理。

## TODO
- 为`src/gallium/drivers/zink/zink_compiler.c/zink_shader_compile`加断点统计执行时间和调用频率。预计结果：heaven可能会对shader进行频繁编译，而glmark2的测试场景单一，编译次数可能极少
- 如果上述预计结果属实，进一步对比`intel`驱动下`shader`编译的负载情况，是否存在明显的性能差距。

## 其他问题或思路
### 为什么 Shader 的 Lower Pass 是修改 NIR？
核心疑问在于，NIR本身是OpenGL输送给驱动层的着色器，是GLSL的编译结果。换句话说，如何执行NIR是硬件驱动的决定的。

当前的shader key策略中，使用的`NIR_PASS`方法是从mesa公用的compiler库中拿来用的，而不是zink层专门编写的。所以与其说这里zink为了进行lower pass优化而实现`NIR_PASS`方法，不如说是zink为了实现硬件驱动的`NIR_PASS`功能，而实现了shader key这个工具。

在这个基本逻辑下，对shader key进行扩展，核心的任务是找到从gallium架构中拿到相应的状态信息的方法，而不是`NIR_PASS`方法本身的实现。