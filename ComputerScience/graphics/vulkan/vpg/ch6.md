# 第六章 着色器和管线

1. 着色器(shader)是直接在设备上执行的一种小程序，是复杂Vulkan程序的基本组成元素
2. spir-v是vulkan官方支持的着色语言，且可以把GLSL等语言通过glslangvalidator转换为spir-v
3. 两种管线： 图形管线和计算管线
4. 管线创建后，要绑定指令缓冲
5. shader不直接读写资源，其通过Descriptor Set完成此类操作
6. Descriptor Set 是绑定在管线上的一组资源，一个管线可对应多个Descriptor Set

## shader中的资源访问

shader里面的资源(即texture，sampler，buffer等)要通过descriptor set访问，descriptor set通过set的序号分组，每组里面又根据binding序号进行编号

descriptor set layout就负责对你shader里面用到的资源的set和binding序号和资源类型进行确定，这个layout是可以复用的，具体资源不同，但只要set,binding,type一致即可
