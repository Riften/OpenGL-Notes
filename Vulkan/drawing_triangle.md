# Vulkan基本程序
简单的绘制三角形示例程序的组成和编写方式。

# SetUp 初始化阶段
- Vulkan本身初始化
- 扩展初始化
- 硬件初始化
  - 物理硬件和逻辑硬件
  - Queue Family检查和`VkQueue`创建

## Instance
Vulkan的环境初始化首先需要创建`VkInstance`，Vulkan设计上是面向对象的，对instance的创建就是对Vulkan的初始化过程。Instance承担了两方面的角色：
- 作为应用和Vulkan之间的桥梁，应用通过Instance调用Vulkan
- 通过Instance，向Driver提供一部分应用相关的信息。

创建`VkInstance`的基本接口使用如下：（[vkCreateInstance](https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#_vkcreateinstance3)）
```cpp
VkInstanceCreateInfo createInfo{};
VkInstance instance;
VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
```

其中核心函数`vkCreateInstance`的声明为
```cpp
VkResult vkCreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance);
```

要想正确完成Instance，需要做的就是根据应用正确的完成[VkInstanceCreateInfo](https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkInstanceCreateInfo)的设置。这些设置主要包含以下几个部分：
- pApplicationInfo: `VkApplicationInfo`类型的指针，对应用进行一些说明，包含类似于应用名称、类型、所属引擎、引擎类型等，Vulkan对于一些特定的应用会有相应的优化。
- 全局扩展：`enabledExtensionCount`指明扩展数量，`ppEnabledExtensionNames`指明扩展名称。用于桌面显示的扩展是一类通常都需要加载的扩展，`GLFW`库本身提供了生成Vulkan扩展的接口。
- 中间层信息，例如一些调试用的中间层。

## Validation Layer

## 显卡和队列适配性
创建`VkInstance`的过程相当于对软件库的初始化，剩下的还需要完成对硬件的选择和相关设定。

### 显卡适配性
列出所有显卡，然后检查适配性。

```cpp
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
uint32_t deviceCount = 0;
vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
std::vector<VkPhysicalDevice> devices(deviceCount);
vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
        physicalDevice = device;
        break;
    }
}
if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
}
```

这里的函数`isDeviceSuitable`是假设的适配性检查函数，基本的检查方式通常是通过两个Vulkan接口获得硬件参数：
- [vkGetPhysicalDeviceProperties](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetPhysicalDeviceProperties.html)获得基本参数，例如硬件名称、类型、支持vulkan版本。
- [vkGetPhysicalDeviceFeatures](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetPhysicalDeviceFeatures.html)获得一些特性的支持情况，例如纹理压缩、64位浮点、多视角渲染。

实际应用中，physical device 的选择策略可能更加复杂，例如根据特性支持情况给不同硬件打分，选择得分最高者。

### 队列适配性
Vulkan中所有操作需要通过组织成command buffer，然后将command buffer提交到对应队列中完成。Vulkan中有**Queue Families**的概念，每一种queue family只支持特定类型的指令，而不同的硬件提供的queue family可能是不同的。

列出queue families的接口和列出physical devices的接口类似
```cpp
uint32_t queueFamilyCount = 0;
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
```

查询结果类型`VkQueueFamilyProperties`中包含了一类queue family的特性，例如是否支持某种指令。示例程序中只关心是否支持`graphics commands`，所以用的判断逻辑是
```cpp
if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
```

## 逻辑设备和逻辑队列
Physical Device 和 Queue Family 都是硬件相关的概念，而到实际应用层则需要在这些实例的基础上构建接口层，也就是逻辑设备和逻辑队列，以作为通用的接口使用。这里的逻辑设备和逻辑队列分别是类`VkDevice`和`VkQueue`

*问题：直接物理设备实现虚拟设备不行吗...*

`VkDevice`的创建接口和`VkInstance`的创建接口类似
```cpp
VkDeviceCreateInfo createInfo{};
VkDevice device;
vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
```
这里的`physicalDevice`是前面选择的物理设备，而队列配置等其他配置则在`createInfo`中。[VkDeviceCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkDeviceCreateInfo.html)需要提供的主要信息如下：
- `sType`: 指明结构体类型
- `uint32_t queueCreateInfoCount`: 数组pQueueCreateInfos的长度
- `const VkDeviceQueueCreateInfo* pQueueCreateInfos`: 指明需要创建的逻辑队列的信息。
- `uint32_t enabledExtensionCount`: 下面这个数组的长度
- `const char* const* ppEnabledExtensionNames`: 需求的扩展的名字
- `const VkPhysicalDeviceFeatures* pEnabledFeatures`: 启用的物理设备特性，由需求和物理设备实际的特性支持情况决定。

需要注意的是，`VkDeviceQueueCreateInfo`中，除了`sType`，queue family索引，需要的逻辑队列数量之外，还有一个`queuePriority`，这是一个`0-1`的浮点数，来一定程度上控制命令缓冲区的调度策略，需要注意的是即使只有一个队列也需要指明该字段。

### Queue Family查询
最终目的是查询设备支持的所有queue family，并且找到程序所需要的的queue family在当前设备的queue family列表中的序号。对示例程序来说，为了找到支持`VK_QUEUE_GRAPHICS_BIT`的queue family的序号，从而可以在之后正确的创建graphic command的目标queue。

查询的核心接口为[vkGetPhysicalDeviceQueueFamilyProperties](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetPhysicalDeviceQueueFamilyProperties.html)
```cpp
void vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice                            physicalDevice,
    uint32_t*                                   pQueueFamilyPropertyCount,
    VkQueueFamilyProperties*                    pQueueFamilyProperties);
```

接口的输出是一个[VkQueueFamilyProperties](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkQueueFamilyProperties.html)数组，**后续许多接口用到family信息的时候，用的不是property本身，而是property在该数组中的索引，queue family index**。`VkQueueFamilyProperties`结构体提供了一个queue family的相关信息：
```cpp
typedef struct VkQueueFamilyProperties {
    VkQueueFlags    queueFlags;
    uint32_t        queueCount;
    uint32_t        timestampValidBits;
    VkExtent3D      minImageTransferGranularity;
} VkQueueFamilyProperties;
```

各字段含义如下：
- queueCount：该queue family包含的queue数量。
- timestampValidBits：时间戳变量中数据位数，可以是32-64，也可以是0表示不支持时间戳。
- minImageTransferGranularity：图像传输操作支持的最小粒度
- queueFlags：指明了该queue family的能力，需要注意的是一个queue family可能同时有多种能力，同时满足多种queueFlag。该flag可能的值如下

```cpp
typedef enum VkQueueFlagBits {
    VK_QUEUE_GRAPHICS_BIT = 0x00000001,
    VK_QUEUE_COMPUTE_BIT = 0x00000002,
    VK_QUEUE_TRANSFER_BIT = 0x00000004,
    VK_QUEUE_SPARSE_BINDING_BIT = 0x00000008,
    VK_QUEUE_PROTECTED_BIT = 0x00000010,
} VkQueueFlagBits;
```

# 送显
## Window Surface
## Swap Chain
## Image Views
Image是由Swap Chain管理的画面对象，由Swap Chain负责生命周期。而Image View则是应用端创建的，应用端需要做好Image View的生命周期管理。

Image View可以用来载入texture等工作，但是如果想要用作渲染目标，还需要进一步处理framebuffer。

# 渲染管线
![渲染管线](../imgs/vulkan_simplified_pipeline.svg)

- Input Assembler: 输入装配，之所以是一个“Assembler”而不是简单地“Reader”是因为对于读入的节点数据，可能会有一些读取顺序、节点重复之类的需求，这些初步处理也一并算在了装配阶段。
- Vertex Shader: 顶点着色器
- tessellation shaders: 曲面细分
- geometry shader: 作用在基本体（primitive，可以理解为面片、线段、断点都可以是基本体，这和primitive vertex中的概念是一致的）的shader，可以对基本体进行修改或者增删，比曲面细分更加灵活，但是效率比较差。在Intel集显中可能表现更好。
- rasterization: 光栅化。将基本体离散化为fragment，一个更加重要的作用是，这一步决定了后续渲染的对象和范围，例如超出屏幕、遮挡的基本体，都在后续步骤中丢弃。另外需要注意的是，遮挡、阴影之类一般是通过深度测试完成的。
- fragment shader: 片段着色器，决定了片段将写入到哪个framebuffer，以及片段中各类数值是多少，例如颜色、深度等。
- color blending: 处理片段到像素的映射，并且根据遮挡或者透明关系，根据多个片段决定当前位置颜色。（透明难道不是在fragment shader阶段就解决的问题吗...）

整个管线的过程可以分成两类，一类是示意图中的绿色部分，是相对固定的步骤，想要完成光栅化渲染就需要包含这些步骤。而黄色的部分则是自定义着色器部分。

所有上述的部分会被最终组成一个`VkPipeline`，然后作为command提交给command buffer。

## Shader Modules
Shader本身在Vulkan中是比较独立的部分，其编译和链接过程可以理解为由单独的编译器进行编译。实际由Vulkan使用的时候已经是bytecode形式的SPIR-V了。但是实际进行shader编写时，通常仍然是使用GLSL。

关于着色器本身的一些知识见[shader.md](shader.md)。

### 创建 VkShaderModule
调用`vkCreateShaderModule`创建，通过`VkShaderModuleCreateInfo`指明创建信息。

## Fixed Functions
除去Shader之外，其他一些必须的渲染过程的配置。

## Render Pass
Render Pass 渲染通道指明了pipeline和framebuffer之间的关联关系，例如渲染需要多少depth 和 color 的缓冲，如何采样，如何将pipeline结果存入framebuffer。

Vulkan中的Framebuffer本质上是有特定格式设定的`VkImage`对象。

创建`VkRenderPass`需要的是正确设定`VkRenderPassCreateInfo`，而其中的重中之重就在于设定需要用到的`VkAttachmentDescription`，Description可能会有多个，RenderPass本身也可以有Subpass，Description通过fragment shader中的`layout(location = 0) out vec4 outColor`所指明的序号完成对应关系。换句话说在Description中，需要对fragment shader中出现的每个这样的有输入输出的vec指定格式。

需要注意的是，这里只是对需要使用的FrameBuffer进行了描述，而没有真正创建FrameBuffer。

## 管线组装
最终需要将pipeline中各个部分组织成一个`VkPipeline`用来说明管线流程。

# 绘制和送显
将前面创建好的用于说明渲染流程的`VkPipeline`，结合用于说明输出和中间过程格式的`VkRenderPass`、用于实际承担最终输出的`VkFrameBuffer`、用于提供顶点数据的`VkBuffer`、用于提供Uniform变量和纹理的`VkDescriptorSet`，共同组织成用于提交的`VkCommandBuffer`，并提交到合适的`VkQueue`中。

在构建好Command Buffer的情况下，绘制过程可以抽象为以下几个过程
- 从swap chain中获取一个image。
  ```cpp
  VkResult vkAcquireNextImageKHR(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    timeout,
    VkSemaphore                                 semaphore,
    VkFence                                     fence,
    uint32_t*                                   pImageIndex);
  ```
- 以刚刚获取到的image为framebuffer中的attachment，提交Command Buffer到Queue中执行。
  ```cpp
  VkResult vkQueueSubmit(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence);
  ```
- 将绘制后的image还给swap chain。
  ```cpp
  VkResult vkQueuePresentKHR(
    VkQueue                                     queue,
    const VkPresentInfoKHR*                     pPresentInfo);
  ```

## FrameBuffer
## Command Buffers
Vulkan中的绘图指令和内存转移指令都是通过Command Buffer来提交执行，而不是直接调用执行的。每个Command Buffer都是相对独立的，包含了一个绘图或者内存操作任务的全部信息。CommandBuffer机制为Vulkan带来了异步和多线程能力。

### 创建VkCommandPool
就如同所有Buffer对象一样，Command Buffer一样需要从资源池`VkCommandPool`中创建。每个`VkCommandPool`只能创建向特定queue提交的buffer。

调用`vkCreateCommandPool`创建，通过`VkCommandPoolCreateInfo`进行配置，其声明如下。
```cpp
typedef struct VkCommandPoolCreateInfo {
    VkStructureType             sType;
    const void*                 pNext;
    VkCommandPoolCreateFlags    flags;
    uint32_t                    queueFamilyIndex;
} VkCommandPoolCreateInfo;
```
`flags`字段有多个可选mask：
- `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`: 表明分配出的command buffer经常被重新记录新的指令。
- `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`: 允许command buffer被单独重新组织，如果不这样设置，所有该pool的buffer需要共同重置。

`queueFamilyIndex`指明了command pool所支持的queue类型。需要注意的是，同样类型的queue family在不同设备上不一定有同样的index，这是一个通过`vkGetPhysicalDeviceQueueFamilyProperties`查询得到的值。

### 创建VkCommandBuffer

## 同步问题
绘制提到的三个任务，获取image、提交command buffer、返回image，互相之间是存在依赖关系的：必须拿到了空闲的image（没人读取没有其他人写入）后才可以把image给command buffer，必须执行完整个command buffer之后绘制才算是完成才可以把图片返回给swapchain。

然而这些并不是自动保证的，这些依赖和同步关系要有应用程序自己搞定。

### Semaphores
Semaphores用来处理队列内或者队列之间操作的同步性，不会直接影响host端的程序运行。

在最简单的三角形绘制场景中，我们需要以下两个Semaphore
```cpp
VkSemaphore imageAvailableSemaphore; // 告知用于渲染的目标image已经获得
VkSemaphore renderFinishedSemaphore; // 告知渲染已经完成可以将image送显
```

Semaphore的创建非常简单
```cpp
VkSemaphoreCreateInfo semaphoreInfo{};
semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore)
vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore)
```

Semaphores的唤醒和等待并不是通过host段直接调用wait或者reset进行的，而是作为`将要被唤醒的semaphore`和`等待被唤醒的semaphore`，以参数的形式提供给接口，或者以属性的形式提供给结构体。

**imageAvailableSemaphore**

用于“告知用于渲染的目标image已经获得”的Semaphore提供给了`vkAcquireNextImageKHR`的参数和`VkSubmitInfo.pWaitSemaphores*`，达到的效果是在图片实际获取完成之后唤醒，在队列提交之前等待唤醒。

同样的同步逻辑也可以通过Fence实现，方法是
- 创建VkFence，并且在info指定初始是unsignaled
- 同样以参数的形式提供给`vkAcquireNextImageKHR`，该接口既有semaphore参数，也有fence参数。
- 在调用`vkQueueSubmit`之前调用`vkWaitForFences`，直接在host端等待fence被唤醒
- `vkWaitForFences`正常返回后调用`vkQueueSubmit`

**renderFinishedSemaphore**

用于“告知渲染已经完成可以将image送显”的Semaphore提供给了`VkSubmitInfo.pSignalSemaphores*`和`VkPresentInfoKHR.pWaitSemaphores*`，达到的效果是在渲染完成后唤醒，送显操作执行前等待唤醒。

和前面同理，也可以用Fence来实现同样的逻辑。

**注意**：提交函数`vkQueueSubmit`的`VkSubmitInfo`中既有执行之前等待唤醒的wait semaphores，也有执行后会被唤醒的signal semaphores。但是送显函数`vkQueuePresentKHR`只有wait semaphores。

**为什么可以用Fence而不用Fence？**

Fence的等待唤醒是在host端进行的，这就使得使用Fence会阻塞当前线程。而使用Semaphore并不会，比如调用`vkQueueSubmit`的时候，函数不会被其设置的`VkSubmitInfo.pWaitSemaphores*`阻塞，实际阻塞和等待是发生在device端的，host端会在实际执行前就返回。

### Fence
Semaphore做到了不阻塞host的情况下保证队列中指令的执行顺序和依赖关系。但是也正因为不阻塞host，所以如果device效率低于host，就会造成队列中的任务过多阻塞。而且在当前的实现中，根据swap chain可提供的image数量创建了相应数量的`VkBuffer`和`VkSemaphore`等每帧变量，并且在每帧绘制的最后通过`currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;`完成索引更新，从而对部分资源进行了复用而不是重复创建。如果host效率高于device，就可能复用到还在使用中的此类资源。

一种暴力的方式就是在每帧绘制的最后强制等待队列中所有命令执行完成，实现方法是调用`vkQueueWaitIdle`，但是这样就使得device闲置。

解决方法是使用Fence来保证**最多只有MAX_FRAMES_IN_FLIGHT个frame正在渲染**，方法如下
- 创建大小为`MAX_FRAMES_IN_FLIGHT`的Fence数组`inFlightFences`，并且初始化为Signal状态，以防止接下来的wait造成死锁。
- 在绘制之前首先调用`vkWaitForFences`等待`inFlightFences[currentFrame]`被signal
- 随后通过`vkResetFences`重新置`inFlightFences[currentFrame]`为unsignal状态。
- 通过`vkQueueSubmit`提交第`currentFrame`个`VkCommandBuffer`时，使用`inFlightFences[currentFrame]`作为它的signal fence，即指令实际执行结束后会signal这个fence
- 在绘制的结尾更新`currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;`

`MAX_FRAMES_IN_FLIGHT`如果直接设定为swap chain可以提供的image数量，那么这样是最容易理解的，fence同时保证了不会 “绘制一个还没绘制完的image”。但是这并不是必须的，`MAX_FRAMES_IN_FLIGHT`数量实际上可以是随意的。只是需要额外加一个数组记录下来 swap chain image 和 current frame 的对应关系，在vkAcquireNextImageKHR更新了`imageIndex`之后，提交image到queue进行渲染之前，先等待该image对应的fence被唤醒。

```cpp
vkAcquireNextImageKHR(..., &imageIndex);

...

if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(..., &imagesInFlight[imageIndex], ...);
}
imagesInFlight[imageIndex] = inFlightFences[currentFrame];

...

vkResetFences(..., &inFlightFences[currentFrame]);

...

vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
```
这里的`imagesInFlight`直接记录了和Fence的对应关系。

**Zink使用了不同的策略处理Fence，但是最终达到的效果是一样的。**