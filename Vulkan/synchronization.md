# Vulkan同步机制
- [Vulkan Doc: Synchronization Examples](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples)
- [Vulkan Spec: Synchronization and Cache Control](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#synchronization)

Vulkan中对资源访问的同步（不可以访问在修改中的资源，在资源transfer等操作完成后才可以对其进行操作，等等）是由应用端负责的，主机（调用端）和设备（GPU执行端）上各种有依赖关系的指令的执行顺序也没有默认的保证机制，内存缓存等数据流控制也是应用程序控制的。为了给应用程序控制同步机制的方法，Vulkan提供了5种同步机制：
- Fence：让host可以知道device上某些任务已经完成。
- Semaphores：控制多个队列对于资源的访问
- Events：提供更细粒度同步，可以由command buffer或者host主动发出信号，实现命令缓冲区的等待，或者由host对等待信号进行查询。以基于`signal`和`wait`的等待和唤醒机制提供使用。
- Pipeline Barriers：同样是command buffer内的同步机制，但是不是通过`signal`和`wait`来使用，而仅仅是在单点设置同步需求。
- Render Passes：Render Passes本身也提供了同步框架，只不过通常需要借助其他同步机制才能完成工作。

> The difference is that the state of fences can be accessed from your program using calls like vkWaitForFences and semaphores cannot be. Fences are mainly designed to synchronize your application itself with rendering operation, whereas semaphores are used to synchronize operations within or across command queues.

## Fences
Fences本身有`signaled`和`unsignaled`两种状态，其基本使用如下
- 在提交queue的时候，可以设定本次提交中的Fence，从而在队列中指令执行过程中唤醒Fence
- 可以通过host端调用`vkResetFences`把Fence重置为`unsignaled`状态
- 可以通过host端调用`vkWaitForFences`来等待Fence被唤醒。
- 可以通过host端调用`vkGetFenceStatus`查询Fence当前状态。