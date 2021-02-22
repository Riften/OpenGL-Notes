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