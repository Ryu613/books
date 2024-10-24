# 8 采样与重建

虽然渲染后最终输出的是一个二维带颜色的像素点组成的网格阵列，但入射光的辐射量事实上是一个在胶片平面上的连续函数。从这个连续函数上取出离散像素值的方法，能明显影响渲染器生成的最终图像的质量。若这个过程处理的不周全，人造的痕迹就会出现。反过来说，若处理得好，相对多那么一点计算量就可以明显提高渲染出来的图像质量。到目前为止，我们是从蒙特卡洛积分的角度讨论这个问题，然而，其他的观点也很有启发。

本章将从介绍采样理论开始(从连续区域里取出离散的样本值，然后使用这些样本来重建一个与原来函数近似的新的函数)。在pbrt中，积分比起重建往往是更普遍的目标，尽管我们会见到傅里叶分析(采样理论的基础)，这个理论也会为蒙特卡洛积分的误差提供启发。在本章的第二部分，我们会讨论蒙特卡洛积分与傅里叶分析的联系，也会讨论采样算法质量的衡量方法。

在后文中，六种采样器的实现占据了本章的大部分篇幅。它们涵盖了解决采样问题的大部分方法。本篇也会包含Filter类，这个类决定了在像素点附近的多个采样如何被合成在一起，用来计算最终像素值。pbrt中的两种Film的实现类都用这些滤波器，来把图像样本的贡献累加到图像的像素上。

## 8.1 采样理论

### 8.1.1 频域与傅里叶变换

### 8.1.2 理想化的采样和重建

### 8.1.3 混叠

### 8.1.4 像素的理解

### 8.1.5 渲染中的采样和混叠

### 8.1.6 采样模式的光谱分析

## 8.2 采样和积分

### 8.2.1 变量的傅里叶分析

### 8.2.2 低差异和准蒙特卡洛

## 8.3 Sampler接口

pbrt的Sampler接口让渲染中使用各种样本生成算法成为可能。采样器提供的样本点被pbrt的积分器以各种方式使用，从确定图像平面上相机光线的来源点，到选择阴影射线跟踪到哪个光源，以及阴影光线应该在哪个点停止。

在后面的章节中，我们将会看到精心设计的采样模式不仅仅只是有理论上的好处，而是会极大的改善渲染图象的质量。使用好的采样算法带来的运行时间成本相对很小，因为，比起计算样本的分量值，衡量每个图像样本的辐射量的成本要比前者大得多。所以，做这项工作是有回报的。

采样器的任务是生成均匀的d-维的样本点，每个点的坐标值都在[0,1)间。点的维度总数没有提前设定，采样器必须根据光传输算法执行时的计算要求，生成额外的维度。虽然这种设计比起一开始就生成每个采样点的所有维度来说要更复杂一些，但是对于那些特定路径需要不同维度数量的积分器来说，就更方便了。

> 这里的维度就是指采样值有几个值组成，一个值算一个维度

![图8.21](img/fg8_21.png)

图8.21 采样器生成了一个d-维的样本点，对于每个图像样本，会用来生成最终图像。在此处，像素(3,8)被采样，并且在像素面上有2个图像样本，样本中的前两个维度(x,y)给出了在像素中的偏移量，后三个维度决定了时间，和对应相机光线的镜头位置。其余的维度是在pbrt的积分器中用于在蒙特卡洛光线传输算法

```c++
<<Sampler的定义>>
class Sampler : public TaggedPointer<<<Sampler Types>> > {
  public:
    <<Sampler Interface>> 
};
```

除了MLTSampler以外，所有采样器都在本章定义，MLTSampler只在MLTIntegrator中使用，详见本书在线版。

```c++
<<Sampler Types>>
IndependentSampler, StratifiedSampler, HaltonSampler, PaddedSobolSampler,
SobolSampler, ZSobolSampler, MLTSampler
```

Sampler的实现类定义了每个像素点要取的样本数，然后通过samplesPerPixel()返回。大部分采样器已把此值作为成员变量，并且通过此方法的实现直接返回这个值。这个方法的实现很直接，我们一般不会纳入本文。

```c++
<<Sampler Interface>>
int SamplesPerPixel() const;
```

当某个积分器正准备用某个给定的像素样本开始工作时，会从调用StartPixelSample()开始，提供给此方法图像中的像素点坐标和像素点中样本的索引(索引值大于等于0，且小于SamplePerPixel()返回的值)，积分器可能也会提供一个起始维度，用于决定样本从哪个维度开始生成

> 上文提到pbrt里的sampler生成采样的维度数一开始是不确定的，所以给了个起始维度参数，用于在某个维度开始，继续生成后续维度值

这种方法是为了两个目的：

1. 某些采样器的实现类，会利用被采样的像素点情况，来改进它们生成采样的整体分布情况，例如：为了保证相邻像素点不会有2个距离很近的样本。虽然这个细节很小，但能大幅提高图像质量
2. 采样器在生成每个样本点前，可把自己置于可确定的状态。让操作可确定是pbrt重要的一部分，对于调试更是重要。所有采样器都被期望能实现这种确定性，以便它们能在多趟渲染过程中，为给定的像素点和样本索引生成几乎一致的样本坐标值。那么，比如pbrt在某次长时间渲染中途崩溃了，调试时就可以指定的像素和样本索引，从渲染器崩溃的地方开始。利用确定性的渲染器，调试时就不需要把崩溃前所有的渲染重新执行一遍。

```c++
<<Sampler Interface>>
void StartPixelSample(Point2i p, int sampleIndex, int dimension = 0);
```

积分器可以通过Get1D()和Get2D()一次性请求d维样本点的1个或2个维度。虽然二维的样本值可以通过调用2次Get1D()获取，但是一些采样器如果知道有2个维度值会被一起使用时，能生成更好的样本值。然而，采样器的接口不支持直接返回3维或更高维度的样本值，因为一般来说，在本书中实现的这类渲染算法并不需要。若有这种场景，可以通过调用低维度的样本值生成方法来构建更高维度的样本点。

```c++
<<Sampler Interface>>
Float Get1D();
Point2f Get2D();
```

有个与Get2D()区别开来的方法:GetPixel2D(), 调用它能拿到一个二维样本，用来确定在胶片平面上被采样的点。后续的某些采样器的实现类，在处理样本的这些维度与处理其他维度中的二维样本的方式不同。其他采样器是调用Get2D()方法实现的。

```c++
<<Sampler Interface>>
Point2f GetPixel2D();
```

由于每个样本的坐标值都是严格小于1的，所以定义一个常数是很有用的，即OneMinusEpsilon,这个常数代表了小于1的最大可表示的浮点数。之后，采样器的实现类有时会把样本值夹拢在这个值的范围内。

```c++
<<Floating-point Constants>>
static constexpr double DoubleOneMinusEpsilon = 0x1.fffffffffffffp-1;
static constexpr float FloatOneMinusEpsilon = 0x1.fffffep-1;
#ifdef PBRT_FLOAT_AS_DOUBLE
static constexpr double OneMinusEpsilon = DoubleOneMinusEpsilon;
#else
static constexpr float OneMinusEpsilon = FloatOneMinusEpsilon;
#endif
```

这些接口里要特别注意的是，使用样本值的代码必须谨慎编写，保证代码请求样本维度的顺序是一致的。考虑以下代码:

```c++
sampler->StartPixelSample(pPixel, sampleIndex);
  Float v = a(sampler->Get1D());
  if (v > 0)
      v += b(sampler->Get1D());
  v += c(sampler->Get1D());
```

在这种情况下，样本的第一个维度总是会传到a()中，当调用b()的代码路径被执行，b()会接收到第二个维度。然而，如果if语句不总是true或false, 那么c()有时候就会接收到第二个维度，或者第三个维度。这种情况会阻碍采样器在每个维度上提供良好分布的样本点。因此，使用采样器的代码必须谨慎编写，使其始终使用掉样本的维度，来避免这个问题。

Clone()是接口所要求的最后一个方法，这个方法返回采样器的的拷贝。因为采样器的的实现类存储了关于当前样本的各种状态，如：被采样的是哪个像素，样本的维度已被使用了多少等等。在是多线程并发时，使用单个Sampler是不安全的。因此，积分器调用Clone()来获取初始Sampler的拷贝，这样每个线程有自己的一个采样器。各种Clone()函数的实现在这里并不关注，所以不会纳入本文。

```c++
<<Sampler Interface>>+= 
Sampler Clone(Allocator alloc = {});
```

## 8.4 独立采样器

独立采样器是Sampler接口最简单的采样器实现。它为每个样本返回独立均匀的样本值，不考虑样本的分布质量。若关注图像的质量时，不应使用。但是这个采样器可以作为与其他采样器比较的基准。

```c++
<<IndependentSampler Definition>>= 
class IndependentSampler {
  public:
    <<IndependentSampler Public Methods>> 
  private:
    <<IndependentSampler Private Members>> 
};
```

与后续的采样器类似，独立采样器会取一个种子用于初始化伪随机数生成器，用来生成采样值。设置不同的种子使在多躺渲染中生成独立的一组样本成为可能，当测量各种采样算法的收敛性时很有用。

```c++
<<IndependentSampler Public Methods>>= 
IndependentSampler(int samplesPerPixel, int seed = 0)
    : samplesPerPixel(samplesPerPixel), seed(seed) {}
```

所以, 为了使独立采样器对于给定的像素样本总是给出相同的样本值，把RNG重设为确定的状态是重要的，而不是比如让其保持在上一个像素样本结束时的状态。为了做到这点，我们利用了pbrt中的RNG类的优势，即此类不仅允许定义$2^64$个序列的伪随机数，而且可以指定序列的偏移量。下方的实现代码基于像素点坐标和种子值，选取了一个确定的序列。然后，基于样本的索引的一个初始偏移量会加到序列里，这样就可以让像素点中不同的样本在序列中相距甚远。如果定义了一个非零的起始维度值，代码会给出一个额外的偏移量加到序列中，用于跳过之前的维度。

```c++
<<IndependentSampler Public Methods>> 
void StartPixelSample(Point2i p, int sampleIndex, int dimension) {
    rng.SetSequence(Hash(p, seed));
    rng.Advance(sampleIndex * 65536ull + dimension);
}
```

有了种子的RNG对象，实现返回1维和2维样本就很简单。注意，Get2D()利用了C++的统一初始化语法，用来保证两次Uniform()是按顺序调用的，这样才能在不同的编译器上有一致的结果

```c++
<<IndependentSampler Public Methods>>
Float Get1D() { return rng.Uniform<Float>(); }
Point2f Get2D() { return {rng.Uniform<Float>(), rng.Uniform<Float>()}; }
Point2f GetPixel2D() { return Get2D(); }
```

所有在8.2章用来分析采样模式的方法都对独立采样器达成一致意见：它一种糟糕的采样器，独立均匀的样本同等的包含了所有频率(它们就是白噪点的定义), 所以它们不会把失真推向更高的频率。此外，均匀随机样本的离散性是1，即最差的可能。(考虑所有样本的维度都是0或1的场景)这个采样器唯一可取之处是，对一个高频率(相对于采样率)下具有大量能量的函数进行积分时，这个采样器与其他更精致的采样器效果差不多。

## 8.5 分层采样器

## 8.6 halton采样器

## 8.7 Sobol采样器

## 8.8 图像重建

就如5.4.3所述，Film对象中的每个像素点，根据估计式计算出积分值，这个积分是从图像函数中采样，通过滤波函数取出的值的乘积。在章节8.1，我们会看到采样理论提供了一个数学基础，这个基础让我们直到为了达到抗锯齿的结果，滤波操作应该如何实现。我们应当遵从以下原则:

1. 从一组图像采样中重建一个连续的图像函数
2. 对该函数进行预滤波，以去除像素间距的奈奎斯特极限以外的任何频率
3. 在像素的位置把预滤波函数处理后的值做采样，用来计算最终像素值

### 8.8.1 Filter接口

Filter类定义了pbrt中像素的重建滤波器接口。在base/filter.h中。

<<Filter的定义>>

```c++
class Filter :
        public TaggedPointer<BoxFilter, GaussianFilter, MitchellFilter,
                             LanczosSincFilter, TriangleFilter> {
  public:
    // <<Filter Interface>> 
};
```

所有滤波器都是二维函数，原点在中央位置，并且定义了一个半径，超过半径的值为0。x和y方向的半径是不同的，但是被假设为互相同步的。一个滤波器通过Radius()方法提供它的半径，滤波器在每个方向上总体的范围是半径的两倍，见图8.49

![图8.49](img/fg8_49.png)
图8.49 在pbrt的滤波器范围根据每个像素点从原点到剪切点的半径来定的。滤波器支持的范围是总体非零的区域，在这里是等于半径的两倍

<<filter的接口>>

```c++
Vector2f Radius() const;
```

Filter的实现也必须提供一个方法，来计算他们的滤波函数，这个函数可能会被滤波器范围外的点调用，在这种情况下，实现类的责任是检测出这种情况，然后返回0。对于滤波器从Evaluate()返回的值来说，不要求积分到1，因为是使用公式5.13这样的估计式来计算像素点的值的，这个值是已归一化的。

<<Filter的接口>>

```c++
Float Evaluate(Point2f p) const;
```

滤波器也必须提供一个重要性采样方法，Sample(),这个方法取一个二维随机点u，值域在[0,1)

<<Filter的接口>>

```c++
FilterSample Sample(Point2f u) const;
```

返回的FilterSample结构体存储了采样后的p点位置和权重，这个权重值是滤波函数在p点的值，比上所使用的采样方法的PDF的值。因为一些滤波器能从它们的分布里准确的采样，直接返回这个比率能够让其根据计算两个值决定返回0还是比率的麻烦减少。

<<FilterSample的定义>>

```c++
struct FilterSample {
    Point2f p;
    Float weight;
};
```

给定了这个接口的特性后，我们能实现GetCameraSample()函数，这个函数在大部分积分器中，用于计算传入到Camera::GenerateRay()方法中的CameraSamples对象

<<Sampler的内联函数>>

```c++
template <typename S>
CameraSample GetCameraSample(S sampler, Point2i pPixel, Filter filter) {
    FilterSample fs = filter.Sample(sampler.GetPixel2D());
    CameraSample cs;
    // <<初始化CameraSample的成员变量>> 
    return cs;
}
```

这个函数里的一个细节是，这个函数被传入的采样器的类型模板化。若是值类型的Sampler对象传到了这个方法，那么这个函数会用pbrt的动态分派机制来处理，来调用Sampler实现类中对应的方法。然而，若是一个构造类型的采样器(比如Halton采样器)传入了进来，那么对应的方法会被直接调用(通常是在函数里以内联方式展开)。这种能力被用于改进pbrt在GPU上的渲染性能，详见15.3.3

在滤波器通过Sample()方法返回FilterSample后，图像采样的位置就能被找到，找到的方法是通过在像素坐标上加上滤波器采样样本的偏移值，然后在每个维度上加上0.5，来把离散的值映射到连续的像素坐标上(回顾8.1.4)。滤波器的权重随着CameraSample来传入，所以当Film调用AddSample()是可以被获取到。

<<初始化CameraSample的成员变量>>

```c++
cs.pFilm = pPixel + fs.p + Vector2f(0.5f, 0.5f);
cs.time = sampler.Get1D();
cs.pLens = sampler.Get2D();
cs.filterWeight = fs.weight;
```

### 8.8.2 FilterSampler类

不是所有Filter实现类都能轻松地根据它们的滤波函数的分布来采样。因此pbrt提供了FilterSampler类，来封装采样细节，封装是基于滤波器列表化的表示来实现的。

<<FilterSampler的定义>>

```c++
class FilterSampler {
  public:
    // <<FilterSampler Public Methods>> 
  private:
    // <<FilterSampler Private Members>> 
};
```

在构造器中，只需提供内存分配器和Filter类就可以了。我们发现让调用者指定滤波器函数的采样速率以构建用于采样的表格并不是特别有用，因此我们硬编码了一个采样速率，即在每个维度的单位滤波器范围内进行 32 次采样。

<<FilterSampler的成员定义>>

```c++
FilterSampler::FilterSampler(Filter filter, Allocator alloc)
    : domain(Point2f(-filter.Radius()), Point2f(filter.Radius())),
      f(int(32 * filter.Radius().x), int(32 * filter.Radius().y), alloc),
      distrib(alloc) {
    <<Tabularize unnormalized filter function in f>> 
    <<Compute sampling distribution for filter>> 
}
```

domain给出了滤波器的边界，f存储了列表化的滤波器函数的值

<<FilterSampler的private成员>>

```c++
Bounds2f domain;
Array2D<Float> f;
```

8.8.3 方框滤波器

在图形学中最普遍的滤波器就是方框滤波器(并且，事实上，如果每特别说明，默认就是方框滤波器)。方框滤波把图像矩形区域内的压样本点视为相同权重。虽然计算效率高，但是可能是最差的滤波器。
