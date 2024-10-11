# Physically Based Rendering From Theory To Implementation 4ed

> 读者需要具有 高等数学，线性代数，概率论与统计学，图形学，C++的基础
>
> 每一章重点和精华部分会逐步更新到此处
>
> 点击标题进到本章详细内容，点击每章总结直接看重点内容
>
> 原始代码: [pbrt-v4](https://github.com/mmp/pbrt-v4)
>
> pbrt代码本人注解版(也是fork自原始代码): [pbrt-v4-digest-cn](https://github.com/Ryu613/pbrt-v4-digest-cn)
>
> 本人学习后重写版:[《物理式渲染技术》](物理式渲染技术.md)
> 目前还在持续更新中
>
> 总结下来的第一遍阅读法：
>
> 1. 第一章还是值得一读，可快读
> 2. 第二第三是数学基础知识，可跳读
> 3. 第四章是辐射，光谱理论，包含重要理论和实现方式，必读
> 4. 第五章相机，胶片和成像的重要原理和实现，必读
> 5. 第六章
> 6. 待续
>
> 中间穿插阅读pbrt-v4源码
>
> 第二遍(todo):
>
> 1. 重新阅读，补充并修缮第一遍的总结内容
> 2. 在无参考代码的情况下(只能参考本书)自己做pbr系统的架构和类设计，类的主要功能给出，调用过程给出
>
> 第三遍(todo):
>
> 1. 把第二遍的代码的实现完成，读取场景并能渲染出不错的效果
> 2. 把本书整体总结成更简单易懂的文章
>
> 后续规划:
>
> 1. 补充实时渲染部分的知识
> 2. 参考业界真实pbr项目，如google filament等, 看是否能读懂，看懂代码，跑通代码
> 3. 把自己实现的pbr渲染系统嵌入到自己的引擎中

## [第一章 绪论](chapter1/chapter1.md)

> 基本概念介绍，章节介绍，代码整体结构介绍。

[第一章重点总结](chapter1/ch1_summary.md)

## [第二章 蒙特卡洛积分](chapter2/chapter2.md)

> 对pbrt用到的关键积分法：蒙特卡洛积分的原理和推导进行了详细说明

[第二章重点总结](chapter2/ch2_summary.md)

## [第三章 几何及其变换](chapter3/chapter3.md)

> 基础几何，线性代数知识介绍和复习

## [第四章 辐射度量学，光谱，色彩](chapter4/chapter4.md)

> 对渲染涉及到的重点物理学知识: 辐射度量学，光谱，色彩原理和实现进行了介绍

[第四章重点总结](chapter4/ch4_summary.md)

## [第五章 相机和胶片](chapter5/chapter5.md)

> 对现实中的相机的光线采集原理，和胶片的和成像的原理进行介绍，并进行代码实现

## [第六章 形状](chapter6/chapter6.md)

> 形状的几何信息部分的设计与实现

## [第七章 图元与求交加速](chapter7/chapter7.md)

> 几何形状的材质信息部分的抽象与实现

## [第八章 采样和重建](chapter8/chapter8.md)

## [第九章 反射模型](chapter9/chapter9.md)

## [第十章 纹理和材质](chapter10/chapter10.md)

## [第十一章 体积散射](chapter11/chapter11.md)

## [第十二章 光源](chapter12/chapter12.md)

## [第十三章 光的传播 I：表面反射](chapter13/chapter13.md)

## [第十四章 光的传播 II: 体渲染](chapter14/chapter14.md)

## [第十五章 利用wavefront在GPU上渲染](chapter15/chapter15.md)

## [第十六章 回顾与展望](chapter16/chapter16.md)

## 附录A: 采样算法

## 附录B: 工具

## [附录C: 场景描述文件的处理](appendix_C/appendix_C.md)

## 参考