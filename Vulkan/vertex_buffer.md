# Vulkan 中的 Vertex Buffer
Vulkan中通过类型`VkBuffer`来完成节点数据的设定和输入，`VkBuffer`以及其分配和绑定过程是可以经过详尽配置的，这也是Vulkan通过这一个简单类型就能完成原本`VAO`和`VBO`任务的基础。

## 基本的Vertex Buffer使用
Vulkan中使用Vertex Buffer包括以下几个步骤
- 在`VkPipeline`管线中说明通过绑定传入的VertexInput的格式
  - 创建`VkVertexInputBindingDescription`，指明将要绑定的Vertex Buffer需要被如何读取，包括有多少个绑定，读取的时候间隔多少读取每个`entry`
  - 创建`VkVertexInputAttributeDescription`，指明如何从Vertex Buffer中读取出一个属性，可以理解为如何解析每个`entry`，需要指定读取哪个binding、读取的结果作为shader中哪个输入、读取的格式是什么、读取的位置（在entry中的offset）是哪里。
  - 将以上两类描述符放到`VkPipelineVertexInputStateCreateInfo`中，并通过`VkGraphicsPipelineCreateInfo.pVertexInputState`传递给Pipeline属性。
- 创建`VkBuffer`对象并为其分配内存
  - `VkBuffer`通过`vkCreateBuffer`创建，通过`VkBufferCreateInfo`指定其属性。这些属性除了基本的buffer大小信息，还需要包括`usage`，`sharingMode`之类的应用信息，以便Vulkan能够高效的使用Buffer。
  - 申请`VkDeviceMemory`，通过`vkAllocateMemory`申请，由`VkMemoryAllocateInfo`指明申请的内存的要求。申请过程本身和`VkBuffer`无关（参数中没有`VkBuffer`），但是目的是申请到能够满足`VkBuffer`需求的内存，因此需要借助`VkMemoryAllocateInfo`中的`VkMemoryRequirements`类型字段来指明需求，而这个需求则可以通过`VkBuffer`对象为依据查询得到。
  - 绑定`VkBuffer`和`VkDeviceMemory`，通过接口`vkBindBufferMemory`
- 填充数据
  - 需要注意的是填充的对象是`VkDeviceMemory`，通过`vkMapMemory`将`VkDeviceMemory`映射到普通的指针`void*`，然后通过`memcpy`等方法实现数据的实际输入。这个方法针对的是通过CPU可访问内存完成填充的方法。
- 将`VkBuffer`绑定到Command Buffer
  - `vkCmdBindVertexBuffers(command buffer, offset of binding, number of binding, VkBuffer[], read byte offset)`，注意是绑定到**Command Buffer**而不是**Pipeline**，Pipeline中指明的是格式和流程，而Command Buffer则是将实际数据应用到Pipeline中。

## Staging Buffer
核心概念：
- **Host Visable**：可以在代码中访问到，即可以通过CPU访问到。
- **Device Local**：显卡设备本地内存，无法直接从外部访问到。

前面的内存分配方式分配的内存对于CPU也是可访问的，而对于GPU来说，最高效的内存标志位为`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`，而这类GPU显存通常是无法直接通过CPU访问的。

在Vulkan中，如果想要创建只有GPU可以访问的内存并且向其中填充内容，需要首先创建CPU可访问的Staging Buffer，然后通过buffer copy将内存copy到配置为`DEVICE_LOCAL`的Vertex Buffer。

缓存拷贝指令需要提交到支持`VK_QUEUE_TRANSFER_BIT`的队列中，绘图指令所使用的支持`VK_QUEUE_GRAPHICS_BIT`或者`VK_QUEUE_COMPUTE_BIT`的队列都是支持`VK_QUEUE_TRANSFER_BIT`的，不过也可以另外申请队列来进行缓存复制操作。这时候需要将Buffer的`sharingMode`设置为`VK_SHARING_MODE_CONCURRENT`以保证队列之间resource一致性。不过在这里使用的仍然只有`VK_GRAPHICS_BIT`的Queue，所以`sharingMode`仍然是`VK_SHARING_MODE_EXCLUSIVE`。

最终的buffer copy需要调用`vkCmdCopyBuffer`通过Command Buffer提交执行。

## Index Buffer
简单说就是通过指明顶点在Buffer之中的Index从而实现顶点数据的复用。