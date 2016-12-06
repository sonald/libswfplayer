# libswfplayer
a swf player widget based on Qt4 and libflashplugin

## 编译依赖
sudo apt install zlib1g-dev libqtwebkit-dev

## 运行依赖
sudo apt install ffmpegthumbnailer gnash libflashplugin

## 编译
mkdir build && cd build && cmake .. && make -j2

## 运行测试
`./qswfplayer ./test.swf`

** 运行前确保系统中安装有flash插件，如果没有可以：sudo apt install libflashplugin **
swfplayer提供的功能见swfplayer.h头文件
