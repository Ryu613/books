# 3 几何和矩阵转换

## 坐标系统

pbrt中，使用三维直角坐标系。某个点的坐标取决于所使用的坐标系统和位置，但是这个点的绝对位置是固定的。

对于n维场景，坐标系有原点$p_o$，n个线性独立的基向量定义的n维仿射空间。所有的向量$\vec v$可以被多个基向量的相加来表示。给定一个向量$\vec v$和其基向量$v_i$,有一组特定系数$s_i$，使:
$$
\vec v = s_1\vec v_1 + s_2\vec v_2 + ... + s_n\vec v_n
$$

相似的，对于所有点p，有一组特定系数$s_i$，公式如下:
$$
p=p_o + s_1\vec v_1 + s_2\vec v_2 + ... + s_n\vec v_n
$$
因此，虽然点和向量都是用x,y,z来表示的，但是它们不同的数学表达不同，不能被简单地互换。

所以，为了定义一个坐标系，我们需要原点和基向量，但是我们只能在这个坐标系里表示点和向量，在其他坐标系就不同了。因此我们定义一个标准坐标系,即原点(0,0,0),基向量x,y,z就是三个坐标轴,其他的坐标系会基于这个标准坐标系来定义，这种方式叫世界空间(world space)。

### 左/右手坐标系

作者在这里叙述了左右手坐标

> 简单来说，我们把x都指向右侧，y都指向上的时候，z轴朝远处就是左手坐标系，朝近处就是右手坐标系。

## n-元组基类

pbrt的类都是基于n-元组(tuple)的类来实现的。这些文件定义在/util/vecmath.h中。

这些类使用了很多高级C++编程技术，只需要知道做什么就可以了，不感兴趣的话不用关注实现。

> 后续文章主要介绍了Tuple2,和Tuple3,这里不详述

## 向量

pbrt基于2维和3维元组类提供了2维和3维向量类

> 后续文章介绍了vector类的用法和向量相关的计算基础，此处略

## 点

在pbrt里，有Point2和Point3类，来表示点。

> 后续段落讲了这些类的使用，略过

## 法线

> 法线的定义和在pbrt中的实现，略

## 射线

> Ray类和RayDifferential类(射线微分量)在pbrt的实现的介绍，略

## 包围盒

> 包围盒的理论和实现，略

## 球面几何学

< 球面几何介绍，和相关函数，略

## 矩阵转换

> 略

## 矩阵转换的应用

> 略

## 3.11 interaction类

SurfaceInteraction和MediumInteraction分别代表表面上的交点和有介质参加的交点的信息。例如：在第六章中，光线到物体的相交过程，是用SurfaceInteraction返回交点的微分几何信息。之后，在第十章中，纹理相关的代码会利用SurfaceInteraction来计算材质的属性。类似的，MediumInteraction类是用于光在介质中(比如烟雾，云)传播的场景。所有的这些实现放在interaction.h和interaction.cpp中

SurfaceInteraction和MediumInteraction都继承自Interaction类，这个类提供了公共的成员变量和方法，这样做能让不同表面和介质的交互只与Interactions相关。

> interaction类是把表面材质和光的介质的相互作用过程分离，这样做是为了让各种表面和各种介质的处理过程解耦化

<<Interaction的定义>>

```c++
class Interaction {
  public:
    // <<Interaction的Public方法>> 
    // <<Interaction的Public成员>> 
};
```

这个类里有各种构造器，取决于构造的顺序和对应接收的参数的顺序，下列这个构造器是最通用的:

<<Interaction的public方法>>

```c++
Interaction(Point3fi pi, Normal3f n, Point2f uv, Vector3f wo, Float time)
    : pi(pi), n(n), uv(uv), wo(Normalize(wo)), time(time) {}
```

所有interaction都有一个p点，用Point3fi存储，是用Intervval来代表每个坐标的值。存储一个浮点值的区间而不是单个Float是为了表示在交点处的计算错误的边界，这种错误发生于p点在与光线相交计算的时候。这个信息有助于避免光线在离开表面时不正确的自相交现象，详见6.8.6

<<Interaction的public成员>>

```c++
Point3fi pi;
```

### 3.11.1 SurfaceInteraction类

表面上特定点的几何属性用SurfaceInteraction表示， 持有此类可以让大部分场景下，系统运行时不需要考虑这个点所对应的物体的几何特征。

## 延伸阅读