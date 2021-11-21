## 编译原理

### 构建

- 使用cmake构建（推荐）  
[cmake安装方法](#install-cmake)
```bash
mkdir build
cd build && cmake ../ && cd ..
cmake --build ./build --target exe
```

- 使用脚本构建
```bash
bash ./build.sh
```

### 运行
```bash
./exe [ 输入文件 [输出文件] ]
```
后两个为可选参数，表示输入以及输出的文件，缺省默认为标准输入输出
例如
```bash
./exe testset/base.cpp

./exe testset/base.cpp output.txt
```

### 如何安装cmake <span id="install-cmake"></span>
```
wget -O cmake.tar.gz https://github.com/Kitware/CMake/releases/download/v3.22.0/cmake-3.22.0-linux-x86_64.tar.gz

mkdir cmake

tar -zxvf cmake.tar.gz -C ./cmake --strip-components 1

ln -s `pwd`/cmake/bin/cmake /usr/bin/cmake
```
