# 7 图元和求交加速

在上一章的类专门用于描述三维物体的几何属性，虽然Shape接口提供了很多便利的几何操作抽象，比如求交和包围。但是对于完全描述场景中物体还不足够。比如：为了定义物体的外观，在每个形状上绑定材质属性是必要的。

在此问题上，pbrt的CPU和GPU渲染路径上的做法是不同的。本章的类的是在CPU上的方式来实现的。若是在GPU上，比如与形状关联的类似材质这样的属性的细节是被GPU的光追API处理的，所以与在CPU上的表示方法是不同的。对于GPU上等价于CPU的处理将在章节15.3.6讨论

对于CPU部分，本章会介绍Primitive接口，并且提供了多种实现类，来支持定义图元的各种属性。然后，我们会介绍两种额外的Primitive实现类，它们扮演聚合体的角色，聚合体即持有多个图元的容器。这两个类的实现能让我们实现加速结构，加速结构即一种数据结构，此结构能帮助我们把对于光线与场景中n个物体求交的时间复杂度O(n)减少。

加速结构BVHAggregate,是基于场景中包围盒的一种层次，本书的在线版也包含了第二种加速结构kdTreeAggregate的实现，此类是基于自适应的递归空间细分来实现的。虽然也提及过很多其他的加速结构，但是几乎所有光追器都是用这两种加速结构的其中一种。在本章末尾的"延伸阅读"章节有其他加速结构可能性的扩展参考资料。因为求交加速结构的构造和使用是GPU光追API的一部分，所以本章中的加速结构只用于CPU上

## 7.1 图元接口和几何图元

Primitive类定义了图元接口，在本小节描述的接口和图元的实现类皆在cpu/primitive.h和cpu/primitive.cpp中

```c++
<<Primitive Definition>>= 
class Primitive
    : public TaggedPointer<SimplePrimitive, GeometricPrimitive,
                           TransformedPrimitive, AnimatedPrimitive,
                           BVHAggregate, KdTreeAggregate> {
  public:
    <<Primitive Interface>> 
};
```

Primitive接口只由3个方法组成，每个方法分别对应一个Shape类中的方法，第一个：Bounds(),返回在渲染空间中包围图元几何体的包围盒，这种包围盒有很多用途，其中一种最重要的用途就是把图元放到加速结构中

```c++
<<Primitive Interface>>
Bounds3f Bounds() const;
```

另外两个方法提供了两类光线求交测试

```c++
<<Primitive Interface>>
pstd::optional<ShapeIntersection> Intersect(const Ray &r,
                                            Float tMax = Infinity) const;
bool IntersectP(const Ray &r, Float tMax = Infinity) const;
```

在求交前，Primitive的Intersect()方法也要负责初始化在ShapeIntersection里的SurfaceInteraction返回的一些成员变量。前两个参数代表形状的材质和其发光属性，如果它自己是发光体，为了方便，SurfaceInteraction提供了一个方法来设置此变量，用来降低无意中没设置完所有值的风险。往后的两个变量与初始化介质的散射属性和片段有关，会在后续的章节11.4介绍

```c++
<<SurfaceInteraction Public Methods>>+= 
void SetIntersectionProperties(Material mtl, Light area,
        const MediumInterface *primMediumInterface, Medium rayMedium) {
    material = mtl;
    areaLight = area;
    <<Set medium properties at surface intersection>>  
}
```

```c++
<<SurfaceInteraction Public Members>>
Material material;
Light areaLight;
```

### 7.1.1 几何图元

GeometricPrimitive类提供了图元接口的基础实现，其存储了各种可能与shape关联的属性

```c++
<<GeometricPrimitive Definition>>
class GeometricPrimitive {
  public:
    <<GeometricPrimitive Public Methods>> 
  private:
    <<GeometricPrimitive Private Members>> 
};
```

每个GeometricPrimitive类持有了一个Shape，和其外观属性的描述。包括有它的材质，发光属性(若其是光源)，表面两侧的介质，和一个可选的alpha纹理(此种纹理能让形状的某个表面消失)

```c++
<<GeometricPrimitive Private Members>>
Shape shape;
Material material;
Light areaLight;
MediumInterface mediumInterface;
FloatTexture alpha;
```

### 7.1.2 对象实例化和动态图元

## 7.2 聚合体

光线求交加速结构是任何光线追踪器核心的一部分。若没有算法来减少无关光线的求交测试的数量，追踪单条光线花的时间就会随着场景中图元的数量以线性增长，因为光线需要与每个图元来做求交测试，来找到最近的交点。然而，在大部分场景中，这么做极其浪费，因为光线不会经过大部分图元的周围。加速结构的目标是能快速同时拒绝一组图元的求交，并且为搜索图元的过程排序，以便使近处的交点更可能被优先找到，远处的交点更大可能被忽略。

由于在光线追踪器中，光线到物体的求交可能占了大部分执行时间，故有许多研究投入到了光线的求交加速算法。我们不会在此试图探索所有的这些研究，但是为感兴趣的读者在本章末的"延伸阅读"中提供了参考材料。

一般来说，光线求交算法有两种主要方法：空间细分和物体细分。空间细分算法把三维空间拆分成多个区域(比如：在场景中叠加与坐标轴对齐的盒子)，然后把跨越每个区域的图元记录下来。在一些算法中，也可能根据跨越区域的图元数量来划分这些区域。当需要为光线找到交点，光线经过的这一组区域会被计算，并且只会对跨越这些区域的图元进行求交测试

相对地，物体细分是基于渐进的把场景中的图元拆分成更小的一组相邻的物体。比如：某个房间的模型可能被拆分成四堵墙，一个地板和一把椅子。若某个光线没有与房间的包围体相交，那么所有的这些图元就可被剔除，否则，光线就会与每个图元做求交测试。比如：若光线打到了椅子的包围体，那么它就可能需要对椅子的每条腿，座位和靠背做求交测试，否则，椅子就会被剔除。

这两种方法在解决一般性的光线相交计算问题上十分成功。从本质上没有哪一种更好。BVHAggregate是基于物体细分的，kdTreeAggregate(本书在线版)是基于空间细分的。两种方法都定义于文件cpu/aggregate.h和cpu/aggregates.cpp

就如TransformedPrimitive和AnimatedPrimitive类一样，对于聚合体的相交方法不负责在交点设置材质，面积光和介质信息，这些信息是由相交的那个图元来设置，聚合体不应更改这些设置。

## 7.3 包围体的层次
