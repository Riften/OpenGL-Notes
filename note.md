blog 中提到了 **CTS isn’t a great tool for zink at the moment due to the lack of provoking vertex compatibility support in the driver**. 从中可以看出软件驱动路线天然缺少**provoking vertex capatibility support**造成了CTS的测试失败，这也可能是piglit中出现 50 个fail的原因。这是个 Vulkan 级别的问题，而不是 zink 级别的。

blog 中提到了包括上面原因的两个 piglit 测试失败的愿意：
- 前面提到的 provoking vertex issues
- corner case missing features such as multisampled ZS readback

## 常用链接

- [Gallium3D架构介绍](https://www.gpudocs.com/2019/12/13/Gallium3D)
- [Gallium3D wiki](https://www.freedesktop.org/wiki/Software/gallium/GAOnlineWorkshop/)
- [Vulkan教程翻译文档](https://blog.csdn.net/hccloud/category_7900178.html)
- [电子书《Learning Modern 3D Graphics Programming》](https://nicolbolas.github.io/oldtut/index.html)
- [mesa master git](https://gitlab.freedesktop.org/mesa/mesa)
- [mesa mike git](https://gitlab.freedesktop.org/zmike/mesa/-/tree/zink-wip)
- [mesa mainpage](http://mesa.sourceforge.net/index.html)
- [apitrace 及其替代工具](http://apitrace.github.io/)

## 常用指令
### 编译RST文档
`sphinx-build -b html [文档目录] [html存放目录]`

### 覆盖本地修改
```bash
git fetch --all
git reset --hard origin/zink-wip
git pull
```

### mesa 编译
编译安装
```bash
meson -Dvulkan-drivers=amd -Dgallium-drivers=swrast,zink,radeonsi -Dlibunwind=false build-zink
ninja -C build-zink/ install
```
安装后可在`/usr/local/lib/x86_64-linux-gnu/dri`路径下可以看到zink相关的链接库文件。

测试运行
```bash
MESA_LOADER_DRIVER_OVERRIDE=zink glxgears -info
MESA_GL_VERSION_OVERRIDE=3.3
```

查看当前编译选项
```bash
meson configure build-zink/
```
设置编译选项
```bash
meson configure build-zink/ -D vulkan-drivers=intel
```

### 禁用AMD显卡
通过BIOS修改默认显卡即可，记得改完之后把显示器插到集显上。
<!--
```bash
lspci -nnk | grep -i vga -A3 | grep 'in use'
sudo gedit /etc/default/grub
```

Modify the following line by adding radeon.modeset=0:
```bash
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash radeon.modeset=0"
```
-->

### 处理包依赖问题
```
正准备解包 …/libreadline7_7.0-3_i386.deb …
正在将 libreadline7:i386 (7.0-3) 解包到 (7.0-1) 上 …
dpkg: 处理归档 /var/cache/apt/archives/libreadline7_7.0-3_i386.deb (–unpack)时出错：
尝试覆盖共享的 ‘/usr/share/doc/libreadline7/changelog.Debian.gz’, 它与软件包 libreadline7:i386 中的其他实例不同
在处理时有错误发生：
/var/cache/apt/archives/libreadline7_7.0-3_i386.deb
E: Sub-process /usr/bin/dpkg returned an error code (1)
解决方案

#备份原来的dpkg info
sudo mv /var/lib/dpkg/info /var/lib/dpkg/baks
#建立新的bpdk info文件夹
sudo mkdir /var/lib/dpkg/info

sudo apt-get update

sudo apt-get install -f
#讲新的追加到新备份中
sudo mv /var/lib/dpkg/info /var/lib/dpkg/baks
#删除不要的
sudo rm /var/lib/dpkg/info -r
#重新设为info
sudo mv /var/lib/dpkg/baks /var/lib/dpkg/info
```