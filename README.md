## 编译原理

### 运行环境
Linux或者MacOS，安装gcc、flex、bison以及nasm。
Windows环境下若要运行，请自行将相关Unix命令修改为Windows支持的命令，以及将表示路径的`/`修改为`\`，并注意需要转义的部分。

### 构建
- 使用cmake构建（推荐）  
[cmake安装方法](#install-cmake)
```bash
mkdir build
cd build && cmake ../ && cd ..
cmake --build ./build --target gencode
```

- 使用脚本构建
```bash
bash ./build.sh
```

### 运行
生成汇编代码
```bash
./gencode 源代码
```
使用nasm编译源代码生成.o文件后，若要生成可执行文件请链接libc库，或直接使用gcc编译。
或使用以下脚本快速生成可执行文件。
```bash
./run.sh 源代码 输出文件
```
例如
```bash
./run.sh testset/base.cpp base
```

### 如何安装cmake <span id="install-cmake"></span>
```
wget -O cmake.tar.gz https://github.com/Kitware/CMake/releases/download/v3.22.0/cmake-3.22.0-linux-x86_64.tar.gz

mkdir cmake

tar -zxvf cmake.tar.gz -C ./cmake --strip-components 1

ln -s `pwd`/cmake/bin/cmake /usr/bin/cmake
```
