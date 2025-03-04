# Vulkan心得

经个人实践后总结，供参考

## 学习路线

首先，万般学习都有隐型或显性的前置知识或技能的要求，不然学起来效率和效果都不会好，vulkan也不例外，主要包括这两个要求

- 了解C/C++基础知识，并且需要具有一定工程能力(至少会搭项目，查询和解决工程方面的常见问题)
- 英语，这是因为国内较好且完整的vulkan资料较少，经我个人实践，我强烈不推荐花时间在尝试国内的大部分资料上，这些资料有很大部分理解偏差多，质量不高。而且有些已经过时

在学习上，要注重记录，实践，和重复练习

步骤已经被我实践过，可避免大部分弯路，如下:

1. 先在LunarG网站下载SDK，看(或下载)doc,这里面已包含了对应vulkan版本的特性文档， 搞清楚SDK怎么用，配好SDK相关的环境，官网:doc.vulkan.org也要收藏
2. 收藏github的awesome-vulkan，里面已经整理好了一系列有用的资料，但是注意筛选
3. 把Vulkan cookbook一书实现一遍，先自己想办法写How to do it,然后再看How it works
4. 看和实践: Vulkan-Game-Engine（若是用vulkan做图形）
5. 看Vulkan Programming Guide一书,这是目前最好的资料
6. 看和实践Vulkan-Samples, Sancha Willems的Vulkan项目，里面有大量工程实际实现的案例，最好自己从头搭建实现
7. 实战，手搓新项目，复习总结之前所学，写大量的代码，找更多的vulkan项目参考

## Vulkan是什么

Vulkan定义了一组API，用于在GPU上做图形渲染或计算，这些功能的实现不在SDK里面，SDK里面只是API的定义，硬件厂商才负责实现，并通过驱动把这些API暴露出来。显卡不一定支持vulkan，vulkan也不一定只在显卡上实现

注意不同驱动版本，支持的vulkan版本和扩展有可能不同

## Vulkan的API

vulkan分为核心API，非核心API，每个版本的API可能是不同的，需要注意。为了使API方便维护管理，把各种功能模块和插件化了，一种叫layer(层)，一种叫extension(扩展)。layer是用来启用除vulkan核心API以外新功能API，extension则是为了对vulkan现有启用的API做增强

## Vulkan的调用结构

![图1](img/vulkan_essentials_1.png)

loader目的有二:

1. 根据不同的操作系统，vulkan版本，驱动，把Vulkan的API映射到具体的驱动实现上
2. 使vulkan可配置启用哪些layer或extensions

loader根据平台不同而不同，windows上，设备的驱动可能就附带了vulkan的loader

注意图中多对一和一对多关系，主要是为了灵活和扩展性考虑

## Vulkan的对象设计

vulkan为了给开发者提供更灵活的掌控力，抽象出了几个关键对象，这些对象就像积木，经过不同的组合过程，就可以最终实现各种功能。也因为灵活性，初学可能感觉比较抽象，这部分只能重复学习和练习慢慢消化

## Vulkan的初始化

根据操作系统，把vulkan的运行时库加载出来，导出loader, 用loader导出vulkan函数，然后在进行实例初始化过程