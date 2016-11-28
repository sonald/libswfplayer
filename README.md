## 编译依赖
sudo apt install libffmpegthumbnailer-dev zlib1g-dev gnash libqtwebkit-dev

## 编译
mkdir build && cd build && cmake . && make -j2

## 运行测试
`./jinshan ./test.swf`

** 运行前确保系统中安装有flash插件，如果没有可以：sudo apt install libflashplugin **
swfplayer提供的功能见swfplayer.h头文件
