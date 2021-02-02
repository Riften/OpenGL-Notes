# OpenGL 3.0 - GLSL 1.30
## GL_EXT_texture_compression_rgtc
- [特性说明](https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_compression_rgtc.txt)
- Zink实现时间2021-1-12

所有分支对该特性的实现都是2021-1-12，看上去和`zink-wip`没关系。

# OpenGL 3.1 - GLSL 1.40
## GL_ARB_uniform_buffer_object 
- [特性说明](https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt)
- [Master分支实现提交](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/7079)
- Zink实现时间2020-10-13

### 特性说明
在GLSL uniform变量的基础上，提出了`uniform block`的概念，并且给出了将`uniform block`存入GL buffer 对象的方法。可以回顾`note.md`中uniform相关内容。

在改特性出现之前，可以通过`glUniform*()`相关接口传入uniform变量或变量列表的值。