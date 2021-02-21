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

## Fixed Functions

## Render Pass
Render Pass 渲染通道指明了pipeline和framebuffer之间的关联关系，例如渲染需要多少depth 和 color 的缓冲，如何采样，如何将pipeline结果存入framebuffer。