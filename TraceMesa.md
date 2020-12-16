# Mesa 打点分析
## 目标
以外部动态链接库的形式实现打点工具，并且在mesa合适的位置嵌入打点代码，分析glmark2运行过程中mesa执行情况，核心为分析调用栈和执行时间，最终定位到glmark2跑分不高的瓶颈点。

**使用动态链接库并不会让事情更加简单或者更加困难，只是方便维护，可以将打点的记录和分析工具和为一个，并且可以分开编译mesa和打点工具。**

**注意**：除了对zink进行打点分析，也要对dri进行打点分析，以便于对两者执行效率差别进行对比。

## 基本流程
1. 编译`debug`版本的`glmark2`和`mesa`
2. 在`glmark2`的`Canvas.clear()`、`Scene.setup()`、`Scene.draw()`、`Scene.update()`几个核心函数处添加gdb断点，输出从glmark2到mesa的stacktrace。
3. 通过`CGO`编写独立线程运行的分析工具，以动态链接库的形式提供给mesa使用
4. 根据gdb调用栈，在mesa代码中合适位置添加分析工具函数（Zink和硬件驱动都需要添加）
5. 根据分析工具得到的统计信息，分析Zink运行瓶颈

### debug版本编译方法
如果要重新编译安装，最好先卸载以前的安装
```bash
ninja -C build uninstall
```

由于打点过程中使用到了后面编译得到的动态链接库，所以需要将动态链接库的头文件放到`glmark2/src/`下。另外为了让meson编译过程中能够找到libMesaTracer，需要在根目录和src目录下的meson.build中添加依赖项。

glmark2 和 mesa 的 debug 版本编译方式是一致的，通过 meson 进行配置。
```bash
meson build
meson configure build -D c_args=-g
meson configure build -D cpp_args=-g
meson configure build -D buildtype=debug
```

需要注意的是，编译mesa过程中还需要进行以下额外配置
```bash
meson configure build -D vulkan-drivers=intel
meson configure build -D gallium-drivers=swrast,zink
```

配置完成后重新安装即可
```bash
ninja -C build install
```

### gdb调试
运行 gdb 调试 glmark2
```bash
gdb glmark2
```

以下指令均在`gdb shell`内执行。

运行程序
```bash
r arg1 arg2 ...
# 如运行build场景
r -b build
```

添加断点
```bash
b 函数名 # 函数名可以通过 func 或者 class::func 指定
b 行号 # 默认为main的行号，可以通过 filename:linenum 指定特定文件特定行号
```

### 分析工具
[github链接](https://github.com/Riften/goMesaTracer)

使用go+cgo编写，主要考虑和设计包括：
- 利用go语言对多线程的良好支持和对c语言的兼容性。
- 编译成动态链接库供`mesa`和`glmark2`调用，能够方便的做到同时对`mesa`和`glmark2`打点分析。
- 使用宏定义做到接口尽量简单（目前接口`cgoAddTrace(cgoType C.int)`只需要传入单个整型作为参数，整型值则有宏定义提供）。
- 能够复用打点和分析的代码。
- 使用多线程减少对mesa效率的影响。