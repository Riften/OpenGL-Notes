# Uniform Buffer
首先需要明确的是`Uniform Variable`是GLSL中的一个概念，指通过`uniform`关键字修饰的全局变量。Shader本身的使用类似于C程序，多个脚本文件通过编译和链接最终得到可以运行的program，在这个过程中，Shader之间经常需要互相传递数据，上一个阶段的Shader的输出作为下一个阶段Shader输入，类似这种。而`uniform`变量则是在不同阶段的shader中保持一致，program的运行过程中不会改变`uniform`变量的值，可以看做是GLSL中的全局变量。

**注意**：在绘制每一帧的时候，完整的Pipeline都会被执行一遍，其中的着色器程序也会完整运行一遍。但是绘制不同的两帧时，使用的就是相互独立的Pipeline，和相互独立的着色器程序了。所以`uniform`变量在两帧之间是可以自由改变的，也正因如此，`uniform`变量通常用来作为视角矩阵、投影矩阵、模型数据、形变矩阵（？待确定）等。

Uniform变量的基本特点是
- 无法在Shader内部被修改，无法在Program运行过程中修改
- 无法作为Shader的输入或者输出
- 可以在绘制不同帧时修改，而不用重新创建缓冲区。

Uniform可以通过`layout`关键字指明位置。在OpenGL中，可以通过`glUniform*`或者`glProgramUniform*`系列接口为其赋值。

**补充：Shader中的变量：**
Shader中的变量包括
- `in` `out`变量：负责定义shader的输入输出，可以在stage之间共享，一个stage的`out`可以作为另一个stage的`in`
- 普通变量：可以是`const`也可以是`non-const`，但是都是只在当前脚本文件(stage)中可访问的局部变量。
- `uniform`

## Resource Descriptor
Vulkan使用`Resource Descriptor`实现Uniform变量，descriptor可以看做是Vulkan为shader提供的除了`VkBuffer`以外另一种访问resource的方式。descriptor的使用分为以下三步
- 在创建Pipeline时声明`descriptor layout`
- 从`descriptor pool`中分配`descriptor`
- 在渲染过程中绑定`descriptor set`

Descriptor Layout之于uniform buffer，正如Render Pass之于vertex buffer。它指明了Pipeline中使用到的Uniform Buffer的格式、用处（位置）等信息，

而Descriptor Set之于Descriptor Layout正如FrameBuffer之于Render Pass，指明了descriptor实际上绑定的buffer或者image。

### 声明 Descriptor Layout
最终创建的Layout对象类型为`VkDescriptorSetLayout`，其依赖的信息通过一系列info指定，其中关系为
- `VkDescriptorSetLayoutCreateInfo`
  - 绑定的layout数量
  - layout数组，类型为`*VkDescriptorSetLayoutBinding`
    - [VkDescriptorSetLayoutBinding](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkDescriptorSetLayoutBinding.html)核心信息包括绑定到shader中位置（即layout指明的位置）、Descriptor类型、Descriptor数量、Descriptor从属的shader stage（vertex、fragment之类）。

然后将info信息作为参数调用`vkCreateDescriptorSetLayout`创建`VkDescriptorSetLayout`。

创建`VkPipeline`时，`VkPipelineLayoutCreateInfo`中的`pSetLayouts`为pipeline所使用的layout数组，`setLayoutCount`为数组长度。

### 创建用于Descriptor的VkBuffer对象并为其申请VkDeviceMemory

