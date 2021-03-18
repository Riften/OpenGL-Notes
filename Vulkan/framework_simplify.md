# 架构简化
解析代码层面上Mesa项目从上层接口到底层Vulkan调用的项目架构，最终实现对整个Mesa架构的轻量化和简化，最终目的有两个方面
- 在编译层面移除不需要的驱动和工具，最终只保留Zink-Vulkan软件驱动。包括编译选项、源码的移除，主要牵扯到Meson脚本的修改。
- 合并并简化调用逻辑，缩短从OpenGL到Zink/Vulkan的Code Path。最理想情况下，将Vulkan以上的所有抽象层，包括Mesa、Gallium、Zink，合并为统一的抽象层Zink。

对整个架构的简化可以分为以下三步：
- 去除不需要的编译模块，只留下Zink驱动。
- 以直观的方式将state tracker即以上的层合并为一层接口层，合并GL状态机和Gallium状态机，接口接收参数，直接修改Gallium状态机。
- 合并Gallium状态机与Zink状态机，`pipe_context`和`zink_context`中只留下一个。

Mesa项目的架构总体上可以分为以下几个层次
- Mesa接口层：形如`_mesa_OpenglCmd`，是OpenGL接口的直接映射。例如`glDrawArrays - _mesa_DrawArrays`
- Mesa工具函数：形如`_mesa_opengl_cmd`，通常是在Mesa层对接口进行的一次切分，例如将多个不同的Draw函数用同一个工具函数来实现，只是改变调用过程中的参数。例如`_mesa_DrawArrays - _mesa_draw_arrays`
- Function table：形如`Driver.OpenglCmd`，`Driver`的成员函数。`Driver`本身是一个函数指针的table，是声明是`mesa/main/mtypes.h/gl_context`的一个类型为`dd_function_table`的成员。`dd_function_table`声明在`src/mesa/main/dd.h`，是一个定义了驱动层所需要实现接口的列表，OpenGL调用通过列表中的函数指针最终传递给驱动。例如`Driver.DrawGallium`
- Mesa State Tracker：形如`st_draw_gallium`，可以理解为驱动层的入口，实现了Function Table中需要实现的函数指针，在状态机初始化过程中会将state tracker的函数给到function table。
- Gallium State Tracker：本质上和Mesa State Tracker是一个东西，只是在OpenGL接口扩展过程中Mesa State Tracker逐渐无法满足需求才有了Gallium State Tracker。是理应和Mesa State Tracker合并的抽象层。

# 编译模块移除
整个Mesa项目使用Meson进行编译，编译模块的移除本质上是修改meson编译配置文件。

最终各类编译配置会通过以下几种方式对实际编译产生影响
- 由meson控制编译的依赖项，以dependency的形式控制链接过程
- 由meson格式化成`-DXXX=XXX`的gcc argument，然后在代码中通过宏控制代码逻辑

## 去除GLES相关模块
首先将根目录`meson.build`中`gles`相关的配置删掉，主要为`with_gles*`相关配置。

将`src/mapi/meson.build`和`include/meson.build`中由`with_gles`控制的编译选项去掉，这里的编译选项指明的主要是相关头文件的安装情况，移除GLES编译选项之后，可以把下列路径中的头文件源文件也移除：
- src/mapi/es2api
- src/mapi/es1api
- include/GLES
- include/GLES2
- include/GLES3

## 去除对于HaikuOS的支持

## 去除不需要的Gallium Frontend

## 移除多余dri
最终目的仅仅保留基于Vulkan的Zink Driver，而移除`dri`和其他`gallium_dri`。移除的驱动主要包括三类：
- 硬件驱动
- Gallium实现的软件驱动
- vulkan驱动，是vulkan本身的驱动而非基于vulkan实现的驱动，可以由第三方提供

**问题：** arm gpu可以使用zink吗？

### 移除步骤

- 将根目录`meson.build`中`with_dri`设置为false，并将`gallium_drivers`只保留`'zink'`
- 将根目录`meson.build`中`with_any_vk`设置为false，这里是是否编译特定的vulkan驱动，而我们本身需要的不是vulkan驱动，经过测试可以在不编译任何mesa vulkan驱动的情况下运行zink，zink依赖的vulkan库可以由第三方提供，具体来说`/usr/share/vulkan/icd.d`中的`json`文件指明了不同显卡对应的vulkan驱动目录，然后应用程序会根据这些信息加载不同的链接库。如果在mesa中也进行vulkan编译，则会在`$prefix/share/vulkan`得到同样的`json`文件，但是指明的驱动是mesa编译得到的`libvulkan_xxx.so`
- 去掉个编译选项对应的子目录`meson.build`中相关编译选项，包括
  - with_intel_vk: src/intel/meson.build
  - with_intel_vk: include/meson.build
  - with_amd_vk: src/amd/meson.build
  - 各类驱动: src/meson.build
  - dri硬件驱动：src/mesa/drivers/dri/meson.build
  - with_gallium_iris: src/loader/meson.build
  - 除zink外的其他gallium驱动: src/gallium/meson.build
  - with_gallium_softpipe: src/gallium/frontends/dri/meson.build
  - 除zink外其他gallium驱动的安装：src/gallium/targets/dri/meson.build
- 移除不再需要编译的源文件，包括
  - src/intel/vulkan: mesa为intel设备提供的vulkan驱动实现
  - include/vulkan/vulkan_intel.h: intel vulkan驱动头文件
  - src/amd/vulkan: mesa为amd设备提供的vulkan驱动实现
  - src/amd/compiler: [ACO Compiler](../note.md#aco)
  - src/mesa/drivers/dri/i915,i965,nouveau,r200,radeon: 硬件驱动实现
  - src/gallium/winsys; src/gallium/drivers/*(除了zink): 除zink以外其他gallium驱动实现
  - src/amd,broadcom,etnaviv,freedreno,panfrost,virtio,nouveau: 部分dri在source目录下的库，是驱动的依赖项
    
- `glx`最好维持`auto`，但是在`with_gallium`为`true`的时候glx还是会被置为`dri`
- `drm`（Direct Rendering Manager）处理。drm是mesa项目中广泛存在的一个依赖，然而drm的版本是根据驱动来判断的，zink本身似乎不依赖与drm，但是不排除zink使用的Gallium方法依赖于drm。所以目前不移除drm依赖，但是其版本直接指定一个版本，而不是根据驱动判断。

## 移除多余 Gallium State Tracker
暂且理解为是为不同Gallium驱动所适配的state tracker，驱动本身都被删了，自然也需要移除，包括根目录下设定的`with_gallium_xvmc`、`with_gallium_omx`、`with_gallium_va`、`with_gallium_xa`、``

### swarst驱动问题
swarst本身是Mesa提供的一个软渲染库，它可以让CPU运行着色器，以便于在没有显卡的情况下运行OpenGL。[Mesa Software Renderer Wikipedia](https://en.wikipedia.org/wiki/Mesa_(computer_graphics)#Software_renderer)

它的实现在Gallium层也被称为softpipe驱动。

## 最终留下和删除的模块和文件
- include
  - GL: OpenGL头文件，最终会被安装到指定的头文件目录

## 移除源码中多余的控制宏
编译过程中许多选项通过添加argument`-Dxxx`的形式增加宏定义，从而直接控制代码逻辑。对编译选项进行精简后，也需要对源码中这部分参数进行直接移除。

宏 | 编译参数 | meson.build文件 | 源码文件 | 说明
- | - | - | - | -
GALLIUM_SOFTPIPE | with_gallium_softpipe | 

## TODO List
记一下要做的事别回头忘了
- [x] 把GLES相关的编译选项删掉
- [x] 写个安全删除文件以及恢复文件的小shell脚本
- [x] 把GLES相关的文件删了
- [ ] 移除多余dri
  - [ ] 找到根目录中的各个`with_dri`参数对编译造成的影响
  - [ ] 删除多余dri源文件
  - [ ] 删除dri依赖的硬件库源码
- [ ] 搞清楚llvm必要性
- [ ] 减少编译参数，例如with_gallium_zink等不需要判断

# 附录
## Meson 相关知识
### Meson 中的 option
Meson中的option可以通过在build脚本里使用[`value get_option(option_name)`](https://mesonbuild.com/Reference-manual.html#get_option)接口来获取。

Meson有两类Option，一类是Meson直接提供的option，一般是各类路径定义和编译器参数（option列表见[附录](#由Meson直接提供的option)）。另一类是用户自己定义的option，通常通过在项目根目录中的`meson_options.txt`文件声明（mesa option列表见[附录](#mesa-option-list)）。Meson Option的格式和使用见[Meson Doc: Build-options](https://mesonbuild.com/Build-options.html)。

option可以在setup或者之后通过configure指令设置：
`meson setup -Doption=value <build-dir>`或者`meson configure <build-dir> -D option=value`

### Meson Argument
Meson可以通过`add_global_arguments('-DFOO=bar', language : 'c')`或者`add_project_arguments('-DMYPROJ=projname', language : 'c')`类似方式添加编译argument，从而对编译过程进行更加精准的控制。

需要注意的是通过`global`的argumets将会在所有子项目中使用，也没有办法在特定项目中取消。

### Meson subdir
`subdir('dir')`指令的作用是，进入到子目录`dir`，并且执行里面的`meson.build`。

### Meson install_headers
`install_headers('foolib.h')`将头文件`foolib.h`安装到系统的头文件目录，默认情况下是`/[install prefix]/include`。这里的`prefix`可以通过直接修改meson的`prefix`参数来修改。

## 核心编译选项解释
详细可以参考后面 [Mesa Option List](#mesa-option-list)。

### `shared_glapi`
这里的`shared`是与`static`对应的，即将opengl库编译成动态的还是静态的，默认情况下windows下为静态，其余为动态。

### `with_glx`


## 由Meson直接提供的option
<details>
<summary>
很长，点击展开
</summary>

```
Core properties:
  Source dir /home/syr/yongxi/meson_tutorial
  Build dir  /home/syr/yongxi/meson_tutorial/build
                                                                                                         
Main project options:                                                                                    
                                                                                                         
  Core options        Current Value        Possible Values                                               Description
  ------------        -------------        ---------------                                               -----------
  auto_features       auto                 [enabled, disabled, auto]                                     Override value of all 'auto' features
  backend             ninja                [ninja, vs, vs2010, vs2015, vs2017, vs2019, xcode]            Backend to use
  buildtype           debug                [plain, debug, debugoptimized, release, minsize, custom]      Build type to use
  debug               true                 [true, false]                                                 Debug
  default_library     shared               [shared, static, both]                                        Default library type
  install_umask       0022                 [preserve, 0000-0777]                                         Default umask to apply on permissions of installed files
  layout              mirror               [mirror, flat]                                                Build directory layout
  optimization        0                    [0, g, 1, 2, 3, s]                                            Optimization level
  strip               false                [true, false]                                                 Strip targets on install
  unity               off                  [on, off, subprojects]                                        Unity build
  warning_level       1                    [0, 1, 2, 3]                                                  Compiler warning level to use
  werror              false                [true, false]                                                 Treat warnings as errors
  wrap_mode           default              [default, nofallback, nodownload, forcefallback]              Wrap mode
  cmake_prefix_path   []                                                                                 T.List of additional prefixes for cmake to search
  pkg_config_path     []                                                                                 T.List of additional paths for pkg-config to search
                                                                                                         
  Backend options     Current Value        Possible Values                                               Description
  ---------------     -------------        ---------------                                               -----------
  backend_max_links   0                    >=0                                                           Maximum number of linker processes to run or 0 for no limit
                                                                                                         
  Base options        Current Value        Possible Values                                               Description
  ------------        -------------        ---------------                                               -----------
  b_asneeded          true                 [true, false]                                                 Use -Wl,--as-needed when linking
  b_colorout          always               [auto, always, never]                                         Use colored output
  b_coverage          false                [true, false]                                                 Enable coverage tracking.
  b_lto               false                [true, false]                                                 Use link time optimization
  b_lundef            true                 [true, false]                                                 Use -Wl,--no-undefined when linking
  b_ndebug            false                [true, false, if-release]                                     Disable asserts
  b_pch               true                 [true, false]                                                 Use precompiled headers
  b_pgo               off                  [off, generate, use]                                          Use profile guided optimization
  b_pie               false                [true, false]                                                 Build executables as position independent
  b_sanitize          none                 [none, address, thread, undefined, memory, address,undefined] Code sanitizer to use
  b_staticpic         true                 [true, false]                                                 Build static libraries as position independent
                                                                                                         
  Compiler options    Current Value        Possible Values                                               Description
  ----------------    -------------        ---------------                                               -----------
  c_args              []                                                                                 Extra arguments passed to the c compiler
  c_link_args         []                                                                                 Extra arguments passed to the c linker
  c_std               none                 [none, c89, c99, c11, c17, c18, gnu89, gnu99, gnu11, gnu17,   C language standard to use
                                            gnu18]                                                       
                                                                                                         
  Directories         Current Value        Possible Values                                               Description
  -----------         -------------        ---------------                                               -----------
  bindir              bin                                                                                Executable directory
  datadir             share                                                                              Data file directory
  includedir          include                                                                            Header file directory
  infodir             share/info                                                                         Info page directory
  libdir              lib/x86_64-linux-gnu                                                               Library directory
  libexecdir          libexec                                                                            Library executable directory
  localedir           share/locale                                                                       Locale data directory
  localstatedir       /var/local                                                                         Localstate data directory
  mandir              share/man                                                                          Manual page directory
  prefix              /usr/local                                                                         Installation prefix
  sbindir             sbin                                                                               System executable directory
  sharedstatedir      /var/local/lib                                                                     Architecture-independent data directory
  sysconfdir          etc                                                                                Sysconf data directory
                                                                                                         
  Testing options     Current Value        Possible Values                                               Description
  ---------------     -------------        ---------------                                               -----------
  errorlogs           true                 [true, false]                                                 Whether to print the logs from failing tests
  stdsplit            true                 [true, false]                                                 Split stdout and stderr in test logs
```
</details>

## Mesa Option List
<details>
<summary>
很长，点击展开
</summary>

```js
option(
  'platforms',
  type : 'array',
  value : ['auto'],
  choices : [
    'auto', 'x11', 'wayland', 'haiku', 'android', 'windows',
  ],
  description : 'window systems to support. If this is set to `auto`, all platforms applicable will be enabled.'
)
option(
  'android-stub',
  type : 'boolean',
  value : false,
  description : 'Build against android-stub',
)

option(
  'dri3',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'disabled', 'enabled'],
  description : 'enable support for dri3'
)
option(
  'dri-drivers',
  type : 'array',
  value : ['auto'],
  choices : ['auto', 'i915', 'i965', 'r100', 'r200', 'nouveau'],
  description : 'List of dri drivers to build. If this is set to auto all drivers applicable to the target OS/architecture will be built'
)
option(
  'dri-drivers-path',
  type : 'string',
  value : '',
  description : 'Location to install dri drivers. Default: $libdir/dri.'
)
option(
  'dri-search-path',
  type : 'string',
  value : '',
  description : 'Locations to search for dri drivers, passed as colon separated list. Default: dri-drivers-path.'
)
option(
  'gallium-drivers',
  type : 'array',
  value : ['auto'],
  choices : [
    'auto', 'kmsro', 'radeonsi', 'r300', 'r600', 'nouveau', 'freedreno',
    'swrast', 'v3d', 'vc4', 'etnaviv', 'tegra', 'i915', 'svga', 'virgl',
    'swr', 'panfrost', 'iris', 'lima', 'zink', 'd3d12'
  ],
  description : 'List of gallium drivers to build. If this is set to auto all drivers applicable to the target OS/architecture will be built'
)
option(
  'gallium-extra-hud',
  type : 'boolean',
  value : false,
  description : 'Enable HUD block/NIC I/O HUD status support',
)
option(
  'gallium-vdpau',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'enable gallium vdpau frontend.',
)
option(
  'vdpau-libs-path',
  type : 'string',
  value : '',
  description : 'path to put vdpau libraries. defaults to $libdir/vdpau.'
)
option(
  'gallium-xvmc',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'enable gallium xvmc frontend.',
)
option(
  'xvmc-libs-path',
  type : 'string',
  value : '',
  description : 'path to put xvmc libraries. defaults to $libdir.'
)
option(
  'gallium-omx',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'disabled', 'bellagio', 'tizonia'],
  description : 'enable gallium omx frontend.',
)
option(
  'omx-libs-path',
  type : 'string',
  value : '',
  description : 'path to put omx libraries. defaults to omx-bellagio pkg-config pluginsdir.'
)
option(
  'gallium-va',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'enable gallium va frontend.',
)
option(
  'va-libs-path',
  type : 'string',
  value : '',
  description : 'path to put va libraries. defaults to $libdir/dri.'
)
option(
  'gallium-xa',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'enable gallium xa frontend.',
)
option(
  'gallium-nine',
  type : 'boolean',
  value : false,
  description : 'build gallium "nine" Direct3D 9.x frontend.',
)
option(
  'gallium-opencl',
  type : 'combo',
  choices : ['icd', 'standalone', 'disabled'],
  value : 'disabled',
  description : 'build gallium "clover" OpenCL frontend.',
)
option(
  'opencl-spirv',
  type : 'boolean',
  value : false,
  description : 'build gallium "clover" OpenCL frontend with SPIR-V binary support.',
)
option(
  'opencl-native',
  type : 'boolean',
  value : true,
  description : 'build gallium "clover" OpenCL frontend with native LLVM codegen support.',
)
option(
  'static-libclc',
  type : 'array',
  value : [],
  choices : ['spirv', 'spirv64', 'all'],
  description : 'Link libclc SPIR-V statically.',
)
option(
  'd3d-drivers-path',
  type : 'string',
  value : '',
  description : 'Location of D3D drivers. Default: $libdir/d3d',
)
option(
  'vulkan-drivers',
  type : 'array',
  value : ['auto'],
  choices : ['auto', 'amd', 'broadcom', 'freedreno', 'intel', 'swrast'],
  description : 'List of vulkan drivers to build. If this is set to auto all drivers applicable to the target OS/architecture will be built'
)
option(
  'freedreno-kgsl',
  type : 'boolean',
  value : false,
  description : 'use kgsl backend for freedreno vulkan driver',
)
option(
  'shader-cache',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Build with on-disk shader cache support.',
)
option(
  'shader-cache-default',
  type : 'boolean',
  value : true,
  description : 'If set to false, the feature is only activated when environment variable MESA_GLSL_CACHE_DISABLE is set to false',
)
option(
  'shader-cache-max-size',
  type : 'string',
  value : '',
  description : '''Default value for MESA_GLSL_CACHE_MAX_SIZE enviroment variable.
   If set, determines the maximum size of the on-disk cache of compiled
   GLSL programs, can be overriden by enviroment variable if needed. Should be set to a number optionally followed by
   ``K``, ``M``, or ``G`` to specify a size in kilobytes, megabytes, or
   gigabytes. By default, gigabytes will be assumed. And if unset, a
   maximum size of 1GB will be used.'''
)
option(
  'vulkan-icd-dir',
  type : 'string',
  value : '',
  description : 'Location relative to prefix to put vulkan icds on install. Default: $datadir/vulkan/icd.d'
)
option(
  'vulkan-overlay-layer',
  type : 'boolean',
  value : false,
  description : 'Whether to build the vulkan overlay layer'
)
option(
  'vulkan-device-select-layer',
  type : 'boolean',
  value : false,
  description : 'Whether to build the vulkan device select layer'
)
option(
  'shared-glapi',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Whether to build a shared or static glapi. Defaults to false on Windows, true elsewhere'
)
option(
  'gles1',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Build support for OpenGL ES 1.x'
)
option(
  'gles2',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Build support for OpenGL ES 2.x and 3.x'
)
option(
  'opengl',
  type : 'boolean',
  value : true,
  description : 'Build support for OpenGL (all versions)'
)
option(
  'gbm',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Build support for gbm platform'
)
option(
  'glx',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'disabled', 'dri', 'xlib', 'gallium-xlib'],
  description : 'Build support for GLX platform'
)
option(
  'egl',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Build support for EGL platform'
)
option(
  'glvnd',
  type : 'boolean',
  value : false,
  description : 'Enable GLVND support.'
)
option(
  'microsoft-clc',
  type : 'feature',
  value : 'auto',
  description : 'Build support for the Microsoft CLC to DXIL compiler'
)
option(
  'spirv-to-dxil',
  type : 'boolean',
  value : false,
  description : 'Build support for the SPIR-V to DXIL library'
)
option(
  'glvnd-vendor-name',
  type : 'string',
  value : 'mesa',
  description : 'Vendor name string to use for glvnd libraries'
)
option(
   'glx-read-only-text',
   type : 'boolean',
   value : false,
   description : 'Disable writable .text section on x86 (decreases performance)'
)
option(
  'llvm',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Build with LLVM support.'
)
option(
  'shared-llvm',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Whether to link LLVM shared or statically.'
)
option(
  'draw-use-llvm',
  type : 'boolean',
  value : 'true',
  description : 'Whether to use LLVM for the Gallium draw module, if LLVM is included.'
)
option(
  'valgrind',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Build with valgrind support'
)
option(
  'libunwind',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Use libunwind for stack-traces'
)
option(
  'lmsensors',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Enable HUD lmsensors support.'
)
option(
  'build-tests',
  type : 'boolean',
  value : false,
  description : 'Build unit tests. Currently this will build *all* unit tests except the ACO tests, which may build more than expected.'
)
option(
  'enable-glcpp-tests',
  type : 'boolean',
  value : true,
  description : 'Build glcpp unit tests. These are flaky on CI.'
)
option(
  'build-aco-tests',
  type : 'boolean',
  value : false,
  description : 'Build ACO tests. These require RADV and glslang but not an AMD GPU.'
)
option(
  'install-intel-gpu-tests',
  type : 'boolean',
  value : false,
  description : 'Build and install Intel unit tests which require the GPU.  This option is for developers and the Intel CI system only.'
)
option(
  'selinux',
  type : 'boolean',
  value : false,
  description : 'Build an SELinux-aware Mesa'
)
option(
  'osmesa',
  type : 'boolean',
  value : false,
  description : 'Build OSmesa.'
)
option(
  'osmesa-bits',
  type : 'combo',
  value : '8',
  choices : ['8', '16', '32'],
  description : 'Number of channel bits for OSMesa.'
)
option(
  'swr-arches',
  type : 'array',
  value : ['avx', 'avx2'],
  choices : ['avx', 'avx2', 'knl', 'skx'],
  description : 'Architectures to build SWR support for.',
)
option(
  'shared-swr',
  type : 'boolean',
  value : true,
  description : 'Whether to link SWR shared or statically.',
)

option(
  'tools',
  type : 'array',
  value : [],
  choices : ['drm-shim', 'etnaviv', 'freedreno', 'glsl', 'intel', 'intel-ui', 'nir', 'nouveau', 'xvmc', 'lima', 'panfrost', 'all'],
  description : 'List of tools to build. (Note: `intel-ui` selects `intel`)',
)
option(
  'power8',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Enable power8 optimizations.',
)
option(
  'xlib-lease',
  type : 'combo',
  value : 'auto',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  description : 'Enable VK_EXT_acquire_xlib_display.'
)
option(
  'glx-direct',
  type : 'boolean',
  value : true,
  description : 'Enable direct rendering in GLX and EGL for DRI',
)
option(
  'prefer-iris',
  type : 'boolean',
  value : true,
  description : 'Prefer new Intel iris driver over older i965 driver'
)
option('egl-lib-suffix',
  type : 'string',
  value : '',
  description : 'Suffix to append to EGL library name.  Default: none.'
)
option(
  'gles-lib-suffix',
  type : 'string',
  value : '',
  description : 'Suffix to append to GLES library names.  Default: none.'
)
option(
  'platform-sdk-version',
  type : 'integer',
  min : 25,
  max : 28,
  value : 25,
  description : 'Android Platform SDK version. Default: Nougat version.'
)
option(
  'zstd',
  type : 'combo',
  choices : ['auto', 'true', 'false', 'enabled', 'disabled'],
  value : 'auto',
  description : 'Use ZSTD instead of ZLIB in some cases.'
)
option(
   'zlib',
   type : 'feature',
   value : 'enabled',
   description : 'Use ZLIB to build driver. Default: enabled'
)
option(
  'sse2',
  type : 'boolean',
  value : true,
  description : 'use msse2 flag for mingw x86. Default: true',
)

```

</details>