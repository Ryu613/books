# 第六章 着色器和管线

1. 着色器(shader)是直接在设备上执行的一种小程序，是复杂Vulkan程序的基本组成元素
2. spir-v是vulkan官方支持的着色语言，且可以把GLSL等语言通过glslangvalidator转换为spir-v
3. 两种管线： 图形管线和计算管线
4. 管线创建后，要绑定指令缓冲
5. shader不直接读写资源，其通过Descriptor Set完成此类操作
6. Descriptor Set 是绑定在管线上的一组资源，一个管线可对应多个Descriptor Set
7. Descriptor Set由多个Descriptor组成，其又从Descriptor Pool里分配，还需绑定到command buffer,以便对descriptor表示的资源进行操作
