# How To Learn Vulkan

> 作者对如何学习vulkan给出了个人心得和有价值的资源

## 学习心态

1. 没有看得到的进展时，不要觉得没进展
2. 当你学到的越多，之前学过做过的动机和原理是会遗忘的，即便你记性很好
3. 同一件事是有多种做法的，Vulkan的实现自由度很高，多注重基础，别去把某种做法当成信条
4. 尽早查阅Vulkan的特性文档，在SDK下的docs文件夹中，你会经常用到它

## 预备阶段

1. 必读[图形管线之旅](https://fgiesen.wordpress.com/2011/07/09/a-trip-through-the-graphics-pipeline-2011-index/)
2. 读RTR里面的图形渲染管线章节
3. 读[Vulkan in 30 minutes](https://renderdoc.org/vulkan-in-30-minutes.html),这个是RenderDoc作者写的，尝试把学到的概念与本文做联系
4. 安装VulkanSDK和RenerDoc
5. 可选安装nVidia nSight或Radeon GPU Profiler 这俩是做性能检测的工具
6. 收藏这些资源
    - API without Secrets
    - Vulkan Tutorial
    - Vulkan Spec
    - Vulkan Examples
    - Awesome Vulkan
    - Vulkan Synchronization Primer
    - 考虑订阅或加入社区
      - Vulkan的reddit主题
      - Vulkan的discord

## vulkan的心智模型

1. 特别注意各种layout选项, location定义符, binding和set, push)constant。当你学到某个新的API或者与描述符绑定有关的对象，传入的参数会对应binding,set或者一个数字索引形成的数组，绑定到那个location会被定义为buffer,uniform,sampler2D，sampler2Darray。文档里面很多都没写。
2. 当你读文档时，多注意类似"binding"和"set"这类字眼究竟是对应哪种类型
3. 当你写自己的项目时，数据和着色器的绑定是灵活的，各种实现方式都是权衡，不要害怕你做了某些不一样的事，即便你错了，经验也会有用

### 渲染通路和管线

需要理解的另一个重要理论就是渲染通路和管线了。这类对象的在游戏和图形工程中抽象出来是很常见的。

### 内存

Vulkan的内存模型比C++的复杂一些，它有多种类型的内存(device visible coherent local等)，选择哪种类型的内存取决于你每一帧要做的事

### 同步

vulkan新手最怕的一部分。若你已经学过C/C++的多线程代码，就不用怕，但是还是得多注意。把GPU看成理论上执行分开的线程，把你分配的内存看成是共享的，当那些内存再用时不要去写入或释放。

## 阅读Vulkan Tutorial 或 API without Secrets

1. 不要怕ctrl cv
2. 不要自作聪明去定义自己的函数和类
3. 当你进入教程新的章节时，去读以下spec文档

无脑打代码没啥用，你不应该去死记硬背api，等以后实际用到了再说，搞定了理论以后，你很快就能学会API了。畸变你觉得某个抽象概念很重要，但是没有足够时间的代码基类也是会犯错的。有几个建议

- 教程里预先在交换链里为每个framebuffer记录的指令是非常不现实的，大部分应用都需要动态绘制
- 教程以线性顺序组织的，但是实际上没必要，你可以画一张时序图

## 自己实现

1. 过完教程后，推荐自己写一个渲染器，对于内存分配器，强烈推荐AMD的，也推荐自己写一个简单的分配器。
2. 参考Sascha Willem的 Vulkan examples
