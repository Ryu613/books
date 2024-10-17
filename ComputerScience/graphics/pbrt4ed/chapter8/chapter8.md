# 8 采样和重建

虽然渲染后最终输出的是一个二维像素颜色组成的点阵，入射光的辐射量是一个定义在胶卷平面上的连续函数。这些离散的像素点的值如何从这个连续函数上计算出来，是会影响图像的质量的。若没处理好，就会出现伪影。反过来说，若处理得好，相对多那么一点计算量就可以明显提高渲染图像的质量。

我们之前已经从蒙特卡洛积分的角度讨论过了，然而其他观点的见解也很有用

本章将从介绍采样理论(从连续区域里取离散样本值，然后使用这些样本来重建一个与原来函数近似的新的函数)开始。在pbrt中，积分比起函数重建，才是真正的目标，尽管我们会看到傅里叶分析(采样理论的基础)，这个理论也会为蒙特卡洛积分的错误给出见解。我们会在本章的第二部分讨论蒙特卡洛积分与傅里叶分析的联系，同时，也会讨论采样算法质量的衡量方法。

有了上述的想法，本章大部分内容都是围绕采样问题的解决，介绍了六种采样器类的实现，同时也会包括Filter类，这个类决定了多个在像素点附近采样如何被合成在一起，来计算最终的像素值。pbrt的两种Film的实现都用这些滤波器来为图像采样的贡献做累加，结果作为像素点的值

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

pbrt的Sampler接口让渲染中的各种采样生成算法能倍使用。采样器提供的采样点会被pbrt的积分器广泛使用，从确定相机光线在图像平面上的起点，到选择阴影射线跟踪到哪个光源，以及阴影光线应该在哪个点停止。

精心定制采样方法并不是只在理论上有用，而是会极大的改善渲染图象的质量。运行时长的成本不会因为使用了好的采样算法而提高很多，是相对代价比较小的，因为衡量每个样本的辐射量是比起计算采样分量来说，成本大得多的部分。所以做采样算法这件事很划得来。

采样器的任务是生成均匀d-维样本点，每个点的坐标值都在[0,1)间。每个点的维度数在前期是未设置的。采样器必须按要求生成额外的维度，维度的数量取决于光传输算法执行计算时所需的维度数量。虽然这种设计使实现一个采样器比一开始就生成每个采样点的所有维度要复杂一些，但是对于那些特定路径需要不同维度数量的积分器来说就更方便了。

> 这里的维度就是指采样值有几个值组成，一个值算一个维度

采样器的实现定义了每个像素点要取几个采样，通过samplesPerPixel()返回。大部分采样器已把此值作为成员变量，返回时直接把此值返回。

当某个积分器已经准备用给出的像素样本运行时，会从StartPixelSample()开始，这个函数提供了图像中像素点的坐标和像素点的样本的索引(索引值大于等于0，且小于SamplePerPixel()的值)。积分器可能也会提供一个起始维度，指定采样生成应该从哪个维度开始

这种方法是为了两个目的：

1. 一些采样器的实现，会利用正在采样的像素信息来改进它们生成的采样的整体分布情况，例如：为了保证相邻像素点不会取到两个太过接近的样本。这个细节很小，但是对图像质量影响很大。
2. 这种方法允许采样器把它们自己，在生成每个样本点前，置于确定的状态。这样做是让pbrt的操作确定性更强，这对于调试非常重要。所有采样器都希望实现这种确定性，这样，这些采样器会为给定的像素点，在多次渲染轮次中，生成接近相同的样本坐标值和样本索引。比如，pbrt在某次长时间渲染中途崩溃了，调试就可以在指定的像素和样本索引开始。利用有确定行为的渲染器，调试时，就不需要把崩溃前所有的渲染重新执行一遍。

积分器可以通过Get1D()和Get2D()同时请求d维的一个或多个样本点的维度值。虽然一个二维的样本值可以通过调用2次Get1D()获取，但是一些采样器在知道有2个维度值要生成的时候，能返回更好的样本值。然而，接口不支持直接返回3维或更高维度的样本值，因为一般来说，渲染算法的实现不需要。若有这种场景，可以通过调用低维度的样本值生成方法来构建高维度的样本点。

GetPixel2D()被用来获取二维的样本，用来确定被采样的点对应胶片平面上的哪个点。某些采样器的实现，在处理样本的胶片采样点的维度值的方式和处理在其他维度下的二维样本值的方式不同，所以才这样做。其他采样器的实现是用调用Get2D()函数完成的。

因为每个样本的坐标值都是严格小于1的，那么定义一个常数是很有用的，即OneMinusEpsilon,这个常数代表了最大的可表示的，小于1的浮点数。之后，采样器的实现有时会把样本值拢在不大于这个值的范围里。

要注意，这些接口在使用时，若利用了样本值，需要谨慎编写代码，这样才能保证请求的样本维度是同一个顺序下的。

```c++
sampler->StartPixelSample(pPixel, sampleIndex);
  Float v = a(sampler->Get1D());
  if (v > 0)
      v += b(sampler->Get1D());
  v += c(sampler->Get1D());
```
考虑以上代码，若if语句没进去，可能导致第二个样本值传给了c(),这样取样本值没保证每次都按照同一个顺序获取，有时会影响样本在每个维度的分布情况。

Clone(), 这个接口最后一个必要的函数，返回Sampler的拷贝体。因为Sampler的实现存储了某个像素点的当前样本的各种状态值，样本维度被利用了多少，等等。如果是多线程，单个Sampler被并行访问是不安全的。因此，积分器调用Clone()来获取初始Sampler的拷贝，这样每个线程有自己的一份。各种Clone()函数的实现在这里并不关注，所以本文不会详述。

Sampler接口类如下:

```c++
class Sampler
    : public TaggedPointer<  // Sampler Types
          PMJ02BNSampler, IndependentSampler, StratifiedSampler, HaltonSampler,
          PaddedSobolSampler, SobolSampler, ZSobolSampler, MLTSampler, DebugMLTSampler

          > {
  public:
    // Sampler Interface
    using TaggedPointer::TaggedPointer;

    static Sampler Create(const std::string &name, const ParameterDictionary &parameters,
                          Point2i fullResolution, const FileLoc *loc, Allocator alloc);

    // 每个像素点要取几个采样
    PBRT_CPU_GPU inline int SamplesPerPixel() const;

    /*
        当某个积分器已经准备用给出的像素样本运行时，会从StartPixelSample()开始，
        这个函数提供了图像中像素点的坐标和像素点的样本的索引(索引值大于等于0，
        且小于SamplePerPixel()的值)。积分器可能也会提供一个起始维度，指定采样生成
        应该从哪个维度开始
    */
    PBRT_CPU_GPU inline void StartPixelSample(Point2i p, int sampleIndex,
                                              int dimension = 0);
    /*
        积分器可以通过Get1D()和Get2D()同时请求d维的一个或多个样本点的维度值。
        虽然一个二维的样本值可以通过调用2次Get1D()获取，但是一些采样器在知道有
        2个维度值要生成的时候，能返回更好的样本值。然而，接口不支持直接返回3维
        或更高维度的样本值，因为一般来说，渲染算法的实现不需要。若有这种场景，
        可以通过调用低维度的样本值生成方法来构建高维度的样本点。
    */
    PBRT_CPU_GPU inline Float Get1D();
    PBRT_CPU_GPU inline Point2f Get2D();

    /*
        GetPixel2D()被用来获取二维的样本，用来确定被采样的点对应胶片平面上的
        哪个点。某些采样器的实现，在处理样本的胶片采样点的维度值的方式和处理在
        其他维度下的二维样本值的方式不同
    */
    PBRT_CPU_GPU inline Point2f GetPixel2D();

    /*
        如果是多线程，单个Sampler被并行访问是不安全的。因此，积分器调用Clone()
        来获取初始Sampler的拷贝，这样每个线程有自己的一份
    */
    Sampler Clone(Allocator alloc = {});

    std::string ToString() const;
};
```

## 8.4 独立采样器

独立采样器也许是Sampler接口最简单的采样实现。它为每个样本返回独立均匀的样本值，不考虑样本的分布质量。若图像的质量要求高的时候，千万别使用。但是这个采样器可以帮助与其他采样器效果进行比较时，作为一个基准。

就像其他的采样器一样，独立采样器会取一个种子用于初始化伪随机数生成器，用来生成采样值。设置不同的种子值可以在多次渲染轮次里保持生成的数是独立的，这个特性用于衡量其他各种采样算法的收敛性的时候是比较有用的。

所以, 为了使独立采样器对于给定的像素样本总是给出相同的样本值，把RNG重设为确定的状态是重要的，而不是比如让其保持在上一个像素样本结束时的状态

> 为了保证像素的采样不与之前的像素采样有关联，每个像素采样完要重设种子数，保证独立性

为了实现此要求，我们利用了pbrt中的RNG类允许定义$2^64$个序列的伪随机数，并且可以指定序列的偏移量。下方的实现选取了确定的序列，基于像素点的坐标和种子数。然后，一个初始偏移量会加到序列里，基于样本的索引值，因此那个像素点中不同的样本会在序列中相距甚远。如果一个非零的维度值被指定，会给出一个额外的偏移量到序列中，这会跳过早期的维度

```c++
/*
    用p点坐标和种子数做哈希，作为伪随机数的序列，然后为序列加入初始偏移
*/
PBRT_CPU_GPU
void StartPixelSample(Point2i p, int sampleIndex, int dimension) {
    rng.SetSequence(Hash(p, seed));
    rng.Advance(sampleIndex * 65536ull + dimension);
}
```

给定了带种子的RNG对象，返回1D和2D的函数的实现就会很简单。注意，Get2D()利用了C++的均匀初始化语法，用来保证Uniform()的调用是按顺序进行的，这样在不同的编译器上才会有一致的结果

所用在8.2章用来分析采样模式的方法都一致的认为：独立采样器是一种糟糕的采样器，独立均匀的样本在所有频率上都是等同的,所以它们不会把失真推向更高的频率。此外，均匀随机样本的离散性是1（最差的可能）。

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
