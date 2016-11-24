## 编译依赖
sudo apt install libffmpegthumbnailer-dev zlib1g-dev
## 编译
mkdir build && cd build && cmake . && make -j2
## 运行测试
./jinshan ./test.swf
