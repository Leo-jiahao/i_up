# i_up
在嵌入式MCU完成部署的一种增量升级方案，使用MCU的内部flash实现。

# 1、文件说明
1. master文件夹，通常用于linux系统或者windows具有大量的RAM平台，执行较为复杂的算法，比如生成差分文件，和对文件进行压缩。

2. slave文件夹，主要是适用于嵌入式MCU级别的，主要是融合算法和解压缩算法，我对二者进行了一些特殊处理以实现对MCU的支持。

# 2、其他
LZ77参考：https://github.com/cstdvd/lz77.git  
bsdiff参考：https://github.com/mendsley/bsdiff.git


