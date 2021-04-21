# Trace Vulkan
对 Vulkan Application 进行 Trace 的方法。

## 相关概念
- [Renderdoc: Brief Guide to Vulkan Layers](https://renderdoc.org/vulkan-layer-guide.html)
- [Github Vulkan Guide: Layers](https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/layers.md)

### Loader
- [Github Vulkan-Loader Spec: Architecture of the Vulkan Loader Interfaces](https://github.com/KhronosGroup/Vulkan-Loader/blob/master/loader/LoaderAndLayerInterface.md)

![Vulkan Loader](../imgs/high_level_loader.png)

Loader是任何Vulkan程序直接交互的对象，其另一端是Vulkan Installable Client Driver (ICD)，即适用于具体硬件的Vulkan驱动。

总的来说，Loader提供以下功能
- 在应用和ICD之间插入各种自定义的Layer。
- 提供多GPU并行支持。
- 充当调度层，将特定的Vulkan Function调度到正确的Layer和ICD。
- 在达到上述能力的同时尽可能降低对Vulkan程序的效率影响。

### Layers
> They can intercept, evaluate, and modify existing Vulkan functions on their way from the application down to the hardware.

Layers通常被用来提供以下功能：
- 验证API使用
- 对调用进行Trace以便于Debug
- Overlay additional content on the application's surfaces

而Layer一个很大的方便之处在于，它是被充当调度层的Loader插入执行过程中的，所以一方面Layer可以截获驱动执行过程之前的所有输入输出和调用信息，另一方面Layer可以被很方便的启用和停用，而不用对应用和驱动进行大的修改。

## GFX Reconstruct
- [GFXReconstruct Github](https://github.com/LunarG/gfxreconstruct)

是[vktrace](https://vulkan.lunarg.com/doc/view/1.2.135.0/windows/trace_tools.html)工具的进阶，初衷是对Vulkan API进行Trace和Replay。提供了进行调用截取的Vulkan Layer和对输出trace进行分析和replay的脚本。

### 设定环境变量
Vulkan Layer 有多重启用和设置方式，最常见的是在Create Instance时通过参数指定，或者通过环境变量指定。

设定`VK_LAYER_PATH`，目的是为了让Loader找到Layer的动态链接库。
```bash
export VK_LAYER_PATH=/gfxreconstruct/build/layer:$VK_LAYER_PATH

# 如果是直接下的release
export VK_LAYER_PATH=/gfxreconstruct:$VK_LAYER_PATH
```

设定`VK_INSTANCE_LAYERS`以启用GFXReconstruct Layer
```bash
export VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_gfxreconstruct
```

进行额外设定，可以通过环境变量，也可以通过`VK_LAYER_SETTINGS_PATH`指明一个配置文件。其中比较重要的是`GFXRECON_CAPTURE_FILE`，指明了输出trace文件目录，默认是`gfxrecon_capture.gfxr`

### 通过脚本运行
```bash
gfxrecon-capture.py -h

# Example
gfxrecon-capture.py -o vkcube.gfxr vkcube
```