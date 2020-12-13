## 架构
glmark2架构总体非常简单，`src`目录下文件包含了核心基类和各个场景的实现，整体上都是面向对象的，比mesa阳间多了。

- main-loop.h/MainLoop: 用来执行主循环，`foreach scene`，执行并拿到评分。
- scene.cpp/Scene: 所有测试场景的基类。
- scene-xxx.cpp: 特定场景的测试类实现，是Scene的派生。

## 测试执行过程