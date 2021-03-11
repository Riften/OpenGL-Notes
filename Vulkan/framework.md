# 总体架构
解析代码层面上Mesa项目从上层接口到底层Vulkan调用的项目架构。

总体上可以分为四个层次
- Mesa接口层：形如`_mesa_OpenglCmd`，是OpenGL接口的直接映射。例如`glDrawArrays - _mesa_DrawArrays`
- Mesa工具函数：形如`_mesa_opengl_cmd`，通常是在Mesa层对接口进行的一次切分，例如将多个不同的Draw函数用同一个工具函数来实现，只是改变调用过程中的参数。例如`_mesa_DrawArrays - _mesa_draw_arrays`
- 映射层：`Driver`的成员函数。`Driver`本身是一个函数指针的table，是声明为`mesa/main/mtypes.h/gl_context`的一个类型为`dd_function_table`的成员，`dd_function_table`声明在`E:\lab219\2020OpenGL\mesa-zink-wip20210301\src\mesa\main\dd.h`，