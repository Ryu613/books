# 5 相机和胶片

在第一章中，我们介绍了小孔相机模型，此模型在计算机图形学中是被普遍使用的。虽然此模型的描述和模拟很简单，但是忽略了现实里光线穿过相机透镜时产生的重要的效果。例如，用小孔相机渲染的所有物体都是清晰聚焦的，这种效果在现实中使用了镜片系统的相机来说是不可能的。这也使图像像是为了完美而使用计算机生成出来的。更普遍地来说，光线在离开镜片系统的辐射分布，与进入镜片系统的辐射分布是完全不同的。为了精确模拟图像形成时的辐射亮度量，对透镜效果进行建模是很重要的。

> 为了获取更真实的图像，小孔相机是不够的，需要模拟相机镜片的效果

相机的镜片系统引入了在图像生成时的各种畸变效果；比如，vignetting(光晕)是由于通过胶片或感光元件边缘的光比起中心位置更少造成的图像边缘更暗的效果。镜片也可造成pincushion(枕形畸变)或barrel(桶形畸变)，使直线在成像后像条曲线。虽然，镜片的设计师的工作就是最小化畸变效果，但是这些效果依旧在图像上是有意义的。

> 镜片造成的图像畸变效果虽然本质是不期望发生的，但是也给人带来图像的真实感
>
> 枕形畸变: 画面向中心收缩，像个枕头
>
> 桶形畸变: 画面由中心膨胀，像个桶，与枕形畸变正好相反

本章将从Camera接口的描述开始，之后会用小孔相机模型作为出发点，来介绍此接口的实现类

在光被相机捕获后，其携带的辐射量，会被传感器测量出来。传统胶片利用化学过程测量，而大部分现代相机使用拆分为像素的多个固态传感器测量，每个传感器会把在一段时间的某个波长范围内打到传感器上的光子数量进行统计。对于传感器如何测量光进行精确建模，是成像过程模拟中重要的一部分。

> 相机捕获光，胶片利用传感器测量光，并最终成像，对此成像过程的模拟很重要

最后，pbrt的所有相机模型都用到了Film类的实例，此类定义了相关类的基本接口，代表了各种相机捕获的图像。在本章中我们会介绍两种胶片的实现类。二者都是用PixelSensor类来为特定胶片或数码图像传感器的光谱响应进行建模。胶片和传感器的类会在本章最后一节介绍

> 本章关键点就在Camera类和Film类，其中Camera类含有Film类的实例，Film类用PixelSensor来对成像过程进行建模
>
> 总结: 图像要真，就要模拟相机镜片造成的畸变效果，pbrt里，相机抽象为Camera类，负责模拟相机光的捕获和成像过程，其持有Film类的实例，Film类负责成像过程的模拟，其又通过PixelSensor模拟光辐射量的测量

## 5.1 相机接口

Camera类使用基于TaggedPointer的方式来把接口中的方法调用，根据具体的相机类型，动态分派给正确的实现类。(详见附录，此处略)，Camera接口在文件base/camera.h中定义

> TaggedPointer简单来讲是为了减少C++多态时虚表的内存开销(特别是复杂场景的渲染时，虚表会导致内存占用大)，并让同一个函数同时支持GPU和CPU的调用(代码在CPU和GPU运行时，函数存储在内存中不同的位置)

```c++
<<Camera Definition>>= 
class Camera : public TaggedPointer<PerspectiveCamera, OrthographicCamera,
                                    SphericalCamera, RealisticCamera> {
  public:
    <<Camera Interface>> 
};
```

相机类必须实现的首个方法就是GenerateRay(),此方法根据给定的图像样本计算并生成光线。返回的光线向量需要做归一化，因为系统中其他部分会用到此归一化的向量。对于给定的CameraSample，若由于某些原因没有可用的光线，那么pstd::optional返回未设置状态的值。光线对应的SampledWavelengths作为一个非常量引用来传入，以便各种相机可以对它们各自的畸变效果进行建模，在这种场景下，只会对光线的单个波长进行追踪，并且GenerateRay()方法会调用SampledWavelengths::TerminateSecondary()

```c++
<<Camera Interface>>= 
pstd::optional<CameraRay> GenerateRay(CameraSample sample,
                                      SampledWavelengths &lambda) const;
```

传入GenerateRay()的CameraSample结构体有定义相机光线时需要的所有样本值。其成员中:

- pFilm: 胶片上的点，对应就是生成出来的光线的原点，这条光线带着辐射量
- pLens: 这条光线穿过镜片时，经过镜片上的点
- time: 光线采样的时长,若相机本身正在运动，time的值决定了当相机光线生成时，相机的具体位置
- filterWeight: 当光线的幅射量被加到胶片存储的图像时，会使用此变量作为辐射量缩放因子，此因子在图像重建中，用于对各个像素点进行图像样本的滤波。详见5.4.3和8.8

```c++
<<CameraSample Definition>>= 
struct CameraSample {
    Point2f pFilm;
    Point2f pLens;
    Float time = 0;
    Float filterWeight = 1;
};
```

GenerateRay()返回的CameraRay结构体，包含了这个光线ray, 及其光谱权重weight。比较简单的相机模型权重默认是1

```c++
<<CameraRay Definition>>= 
struct CameraRay {
    Ray ray;
    SampledSpectrum weight = SampledSpectrum(1);
};
```

相机的实现类同时也必须提供GenerateRayDifferential()的实现，此方法不单像GenerateRay()那样计算并生成主光线，还为在胶片平面上的像素点生成在x,y方向偏移一个像素距离的光线。这些光线代表了相机光线在胶片位置的上的变化量，为系统中的其他部分提供有用的信息(某个特定相机光线的样本，表示多少胶片面积), 此信息对于纹理查找时的反走样处理会很有用

```c++
<<Camera Interface>>+=  
pstd::optional<CameraRayDifferential> GenerateRayDifferential(
    CameraSample sample, SampledWavelengths &lambda) const;
```

GenerateRayDifferential()返回CameraRayDifferential结构体的实例，与CameraRay基本等同，只是其存储的是RayDifferential实例

```c++
<<CameraRayDifferential Definition>>= 
struct CameraRayDifferential {
    RayDifferential ray;
    SampledSpectrum weight = SampledSpectrum(1);
};
```

Camera的实现必须能够访问其Film，方便系统其他部分使用，比如获取输出图像的分辨率等

```c++
<<Camera Interface>>+=  
Film GetFilm() const;
```

就如现实世界中的各种相机那样，pbrt的相机模型也包含了快门的概念，快门会在短时间内打开，以便把胶片暴露给光。若此值不为0,则代表有曝光时间，会带来动态模糊(motion blur)效果。在曝光时间内，与相机有关的运动中的物体会变模糊。时间也是按照点来采样的，并且也遵从蒙特卡洛积分，即给定一个合适的光线的(快门)采样时间，可以计算出图像的动态模糊效果

SampleTime()接口，在相机快门打开时，利用[0,1)间的均匀随机采样u，映射为采样时间。一般来讲，就是在快门打开和关闭的时间内做线性插值

```c++
<<Camera Interface>>+=  
Float SampleTime(Float u) const;
```

最后一个接口可让相机的实现类设置ImageMetadata类里的字段，用来定义与相机相关的转换矩阵。若输出的图片格式支持存储这些额外信息，那么这些信息将会在与最终图片一并写入磁盘

```c++
<<Camera Interface>>+=  
void InitMetadata(ImageMetadata *metadata) const;
```

### 5.1.1 相机的坐标空间

在正式开始介绍pbrt中的相机模型及实现之前，我们会为所用到的一些坐标空间进行定义。除了在3.1章节中介绍过的世界空间外，现在我们将会介绍额外四种坐标空间：物体空间，相机空间，相机-世界空间，和渲染空间，总结如下:

- **物体空间**： 几何图元的定义所基于的坐标系统，比如，pbrt中的球体，圆心就在物体空间的坐标原点
- **世界空间**： 虽然每个图元都有各自的物体空间，但场景中所有的物体是根据一个世界空间来摆放的。每个物体在世界空间中的位置，由一个"物体到世界"的变换来确定。世界空间是其他空间定义的基本依据
- **相机空间**： 相机以特定观察方向和朝向放置于世界空间的某个点上。此相机所在的点定义了一个新的坐标系的原点，这个新坐标系的z轴对应观察方向，y轴对应相机指向"上"的方向
- **相机-世界空间**： 与相机空间类似，此坐标系的原点即相机位置点，但是坐标轴朝向与世界空间下的坐标轴一致(就是说，与相机空间不同点就是此处的坐标空间不一定看向z轴方向)
- **渲染空间**： 即渲染时，场景需要变换的目标坐标系，在pbrt中，可以是世界空间，相机空间，或是相机-世界空间

基于光栅化的渲染器中，传统上大部分计算都是基于相机空间的：在投影到屏幕并光栅化前，三角形的顶点都会从物体空间转换到相机空间。在此场景下，哪些物体可能被相机看到会很容易判断。比如，若一个物体的整个相机空间包围盒是在z=0平面背后的(且相机视场角不超过180度)，物体将不可见

不同的是，许多光线追踪器(包括旧版pbrt代码)是在世界空间上渲染。相机是基于相机空间来生成光线的，但是生成后会把这些光线转换到世界空间，并执行后续的求交和着色计算。由此产生的问题是，由于靠近原点的浮点数相对比远离远点的浮点数更精确，那么若相机放置于远离原点的位置，相机观察到的这部分场景，在表示上的精度就会不足。

图5.1解释了在世界空间中渲染产生的精度问题，在图5.1(a)中，物体和相机都是用原始设定渲染的，在世界空间下，二者的位置都在每个坐标的±10范围内。在图5.1(b)中，相机和场景在每个维度上都移动了1000000个单位。理论上，两张图片应该是一样的，但是第二张图片明显精度低很多，在这个几何模型体上，可看到浮点数导致的离散的边界

> 在光线追踪中，若基于世界空间渲染，会导致一个问题：若相机和物体若离世界空间的坐标太远，那么图像渲染出来可能是撕裂的

在相机空间中渲染，由于此场景下物体是最靠近相机的，天然提供了最大的浮点精确度。若图5.1是在相机空间中渲染的，若把相机和场景中的几何体都平移1000000单位，就不会有撕裂效果，因为平移(带来的位置变化)被消除了。然而，若光线追踪使用相机空间渲染，会有问题。因为场景一般是以轴对齐的方式建模的(比如建筑物模型，地板和天花板很可能与y平面对齐)。此种情况下轴对齐的包围盒就退化为一个维度，因此减少了模型表面的面积。类似将在第七章介绍的BVH加速结构对此种包围盒会特别有效。反之，若相机相对于场景做旋转，轴对齐的包围盒就不那么有效了，渲染性能就会降低，对于图5.1的场景，渲染时间会增加27%

> 相机空间下做光追渲染，由于模型本身，和其包围盒一般是与坐标轴对齐的，旋转相机会导致相机空间的坐标系也转动，那么场景里的模型的包围盒在这个坐标系下就不是与相机空间的坐标系轴对齐的，类似BVH这样的求交加速结构效率就低，最终导致渲染性能降低

采用相机-世界空间来渲染可取二者之长，相机位于这个空间的原点，并且场景随着相机的位置对应做了平移。然而，相机的旋转则不会应用到场景中的几何体上，因此加速结构的包围盒的效果还是好的。采用相机-世界空间，不会增加渲染时间，并且同时保证了精度。如图5.1c，CameraTransform类对渲染时使用的坐标系的选取过程做了抽象，其会处理各种坐标空间的变换细节。

```c++
<<CameraTransform Definition>>= 
class CameraTransform {
  public:
    <<CameraTransform Public Methods>> 
  private:
    <<CameraTransform Private Members>> 
};
```

Camera的实现类必须使其CameraTransform对系统其他部分可用，故我们会在Camera接口中新增一个方法

```c++
<<Camera Interface>>+=  
const CameraTransform &GetCameraTransform() const;
```

CameraTransform类维护了两种变换，其一是把相机空间变换到渲染空间，其二是把渲染空间变换到世界空间。在pbrt中，后者的变换时，(相机)不可是动画化的，在相机变换过程中的任何动画会保持第一种变换(的)。这保证了正在移动的相机不会导致场景中的静态几何体也变成动画化，而造成性能损失

```c++
<<CameraTransform Private Members>>= 
AnimatedTransform renderFromCamera;
Transform worldFromRender;
```

CameraTransform的构造器取定义与场景描述文件中的"从相机到世界空间"的变换，并把其分成之前提到的两个变换。默认的渲染空间是相机-世界空间，然而，渲染空间的选择可利用命令行选项来覆盖

```c++
<<CameraTransform Method Definitions>>= 
CameraTransform::CameraTransform(const AnimatedTransform &worldFromCamera) {
    switch (Options->renderingSpace) {
    case RenderingCoordinateSystem::Camera: {
        <<Compute worldFromRender for camera-space rendering>> 
    } case RenderingCoordinateSystem::CameraWorld: {
        <<Compute worldFromRender for camera-world space rendering>> 
    } case RenderingCoordinateSystem::World: {
        <<Compute worldFromRender for world-space rendering>> 
    }
    }
    <<Compute renderFromCamera transformation>> 
       Transform renderFromWorld = Inverse(worldFromRender);
       Transform rfc[2] = { renderFromWorld * worldFromCamera.startTransform,
                            renderFromWorld * worldFromCamera.endTransform };
       renderFromCamera = AnimatedTransform(rfc[0], worldFromCamera.startTime,
                                            rfc[1], worldFromCamera.endTime);

}
```

当用相机空间渲染时，在worldFromRender会用worldFromCamera的变换来赋值；并且，对于renderFromCamera的变换，会用一个单位变换来完成，因为这两个坐标系是等价的。然而，由于worldFromRender无法动画化，其实现会取worldFromCamera会取帧的中点，然后把相机变换中的任何动画效果，和并入到renderFromCamera中

```c++
<<Compute worldFromRender for camera-space rendering>>= 
Float tMid = (worldFromCamera.startTime + worldFromCamera.endTime) / 2;
worldFromRender = worldFromCamera.Interpolate(tMid);
break;
```

对于默认的相机-世界空间下的渲染，worldFromRender的变换是由相机的位置在帧的中点处平移得到的

```c++
<<Compute worldFromRender for camera-world space rendering>>= 
Float tMid = (worldFromCamera.startTime + worldFromCamera.endTime) / 2;
Point3f pCamera = worldFromCamera(Point3f(0, 0, 0), tMid);
worldFromRender = Translate(Vector3f(pCamera));
break;
```

对于世界空间渲染，worldFromRender就是单位变换

```c++
<<Compute worldFromRender for world-space rendering>>= 
worldFromRender = Transform();
break;
```

一旦worldFromRender设置好后，在worldFromCamera中剩余的变换都会被解出并存储于renderFromCamera

```c++
<<Compute renderFromCamera transformation>>= 
Transform renderFromWorld = Inverse(worldFromRender);
Transform rfc[2] = { renderFromWorld * worldFromCamera.startTransform,
                     renderFromWorld * worldFromCamera.endTransform };
renderFromCamera = AnimatedTransform(rfc[0], worldFromCamera.startTime,
                                     rfc[1], worldFromCamera.endTime);
```

CameraTransform类提供了叫RenderFromCamera(),CameraFromRender()和RenderFromWorld()的各种重载方法，用来把点，向量，法线和射线在其管理的各个坐标系之间做变换。其他方法直接返回了对应的变换，在此就不详述

### 5.1.2 CameraBase类

本章中所有的相机实现类都共享一些通用的函数，我们将其重构到了单个类中，即CameraBase,所有相机实现类皆继承此类。CameraBase及其所有实现类，都定义于文件cameras.h和cameras.cpp中

```c++
<<CameraBase Definition>>= 
class CameraBase {
  public:
    <<CameraBase Public Methods>> 
  protected:
    <<CameraBase Protected Members>> 
    <<CameraBase Protected Methods>> 
};
```

CameraBase的构造器会取在所有pbrt的相机类中都可能用到的各种参数:

- 最重要的一个参数，是把相机放置在场景中的变换，以CameraTransform来表示，储存于cameraTransform的成员变量中
- 其次是一组浮点数，用来给出相机快门开关的时间
- Film的实例，此类存储了最终图像，并且对胶片传感器进行建模
- 最后是Medium实例，代表了相机基于的散射介质(若有)(Medium类在11.4章节进行介绍)

下方是一个小结构体，此结构体把这些参数绑定在一起，可为Camera的构造器缩短参数列表长度

```c++
<<CameraBaseParameters Definition>>= 
struct CameraBaseParameters {
    CameraTransform cameraTransform;
    Float shutterOpen = 0, shutterClose = 1;
    Film film;
    Medium medium;
};
```

我们会只在此处包含构造器的原型，因为其实现只是对其成员变量进行赋值

```c++
<<CameraBase Protected Methods>>= 
CameraBase(CameraBaseParameters p);


<<CameraBase Protected Members>>= 
CameraTransform cameraTransform;
Float shutterOpen, shutterClose;
Film film;
Medium medium;
```

CameraBase类能直接实现Camera接口要求的许多方法，因此节省了继承此类的camera实现类中，多余实现的麻烦

比如，此类有Film和CameraTransform的访问方法

```c++
<<CameraBase Public Methods>>= 
Film GetFilm() const { return film; }
const CameraTransform &GetCameraTransform() const {
    return cameraTransform;
}
```

SampleTime()方法是利用样本的u值，在快门开关时间之间以线性插值法实现

```c++
<<CameraBase Public Methods>>+=  
Float SampleTime(Float u) const {
    return Lerp(u, shutterOpen, shutterClose);
}
```

CameraBase提供了一个GenerateRayDifferential()方法，此方法通过多次调用camera接口的GenerateRay()来实现。这里有一个细节，使用此方法的相机实现类还是必须自己实现一个Camera接口的GenerateRayDifferential()方法，但是之后可以在各自的实现里来调用此方法(注意，此方法的函数签名跟camera接口里那个同名方法不同)相机的实现类把它们的指针作为Camera参数的指针传入，这就可以调用相机的GenerateRay()方法了。这种额外的复杂度引入是由于我们在相机接口没有使用虚函数导致的，这意味着CameraBase类本身不具有调用那个方法的能力，除非某个相机类提供了此方法的实现

```c++
<<CameraBase Method Definitions>>= 
pstd::optional<CameraRayDifferential>
CameraBase::GenerateRayDifferential(Camera camera,
        CameraSample sample, SampledWavelengths &lambda) {
    <<Generate regular camera ray cr for ray differential>> 
    <<Find camera ray after shifting one pixel in the  direction>> 
    <<Find camera ray after shifting one pixel in the  direction>> 
    <<Return approximate ray differential and weight>> 
}
```

主光线会在第一次调用GenerateRay()时被找到。若对于给定的样本没有可用的光线，那么也不会有对应的微分光线

```c++
<<Generate regular camera ray cr for ray differential>>= 
pstd::optional<CameraRay> cr = camera.GenerateRay(sample, lambda);
if (!cr) return {};
RayDifferential rd(cr->ray);
```

为了找到x方向的微分光线，会有两次尝试，一次向前差分，一次向后差分，差分量是像素的一小部分。这两次尝试对于真实感相机模型中图像边缘的光晕(vignetting)效果时很重要的。有时，主光线是存在的，但是在某个方向的移动会超出镜片系统的成像范围，这种情况下，尝试另一个方向就可能成功生成光线

```c++
<<Find camera ray after shifting one pixel in the x direction>>= 
pstd::optional<CameraRay> rx;
for (Float eps : {.05f, -.05f}) {
    CameraSample sshift = sample;
    sshift.pFilm.x += eps;
    <<Try to generate ray with sshift and compute x differential>> 
}
```

生成x的辅助射线也是可能的，那么就可通过差分来初始化对应的像素宽的微分量

```c++
<<Try to generate ray with sshift and compute  differential>>= 
if (rx = camera.GenerateRay(sshift, lambda); rx) {
    rd.rxOrigin = rd.o + (rx->ray.o - rd.o) / eps;
    rd.rxDirection = rd.d + (rx->ray.d - rd.d) / eps;
    break;
}
```

对于\<\<Find camera ray after shifting one pixel in the y direction\>\>这个代码片段的实现，与上文类似，此处略过

若x,y的射线都被找到，我们就能继续，并且把hasDifferentials设置为true。否则，主光线仍然能被追踪，只是没有可用的微分量

```c++
<<Return approximate ray differential and weight>>= 
rd.hasDifferentials = rx && ry;
return CameraRayDifferential{rd, cr->weight};
```

最终，为了让其子类更加便利，CameraBase提供了使用CameraTransform类的各种变换方法。此处只写了Ray的变换方法，其他方法也是类似的

```c++
<<CameraBase Protected Methods>>+= 
Ray RenderFromCamera(const Ray &r) const {
    return cameraTransform.RenderFromCamera(r);
}
```

## 5.2 投影相机模型

三维空间下的观察问题在三维图形学下是最基本的问题之一，即，如何把3D场景展示在二维图像上。最经典的解决方法是用$4\times 4$矩阵来实现。因此，我们会介绍一个投影矩阵的相机类，叫ProjectiveCamera，然后基于它，定义2个相机模型。第一个实现是正交投影，另一个实现是透视投影，这两种是最经典和广泛运用的投影类型。

正交和透视投影都需要定义2个与观察方向垂直的平面，近平面和远平面，当用光栅化渲染的时候，不在这两个平面内的物体会被剔除，最终图像中就没有这些物体。(剔除近平面前面的物体是非常重要的，这是为了防止物体深度为0时的奇点问题，同时避免把相机背后的物体错误地映射到了前面)

> 在光栅化渲染下，当物体深度接近或等于0时，计算时导致结果为无穷大或未定义，导致奇点问题(不可控现象，比如黑块，闪烁，消失)。深度值为负时，若计算没有加排除负值的判断，会导致相机后面的物体会跑到前面来

对于光追器来说，投影矩阵只是单纯用来确定离开相机的光线，这些问题并不适用，因此，在这种情况下，过分担心设置这些平面的深度值是没必要的。

如图5.2所示，还有三种坐标系，这些坐标系对于定义各种投影相机的类是很有用的：

- 屏幕空间： 屏幕空间是在胶片平面上定义的，相机在相机空间中把物体投影到胶片平面上，在屏幕窗口中的物体的那部分，会在生成的图像中可见。在近平面上的点会被映射到深度z为0的面上，远平面上的点映射到的深度z值就是1。注意，虽然其被称为"屏幕"空间，但还是一个3维坐标系，因为z值是有意义的
- 归一化的设备坐标(NDC)空间: 被渲染的图像真正的坐标系，对于x和y，范围从左上到右下，(0,0)到(1,1)，深度值与在屏幕空间中的值相同，可通过某种线性变换把屏幕空间转换为NDC空间
- 光栅空间：与NDC空间几乎相同，除了x和y是从(0,0)到图像分辨率下的x,y像素数

投影相机使用$4 \times 4$矩阵，来在上述空间之间做变换

![图5.2](img/fg5_2.png)

图5.2 为了简化相机类的实现，有几种相机相关的坐标空间被普遍使用。camera类持有这些空间之间的转换方法。在渲染空间下的场景中的物体会被相机观察到，这些物体在以相机空间的原点，指向+z轴方向上。在近平面和远平面之间的物体，会被投影到胶片平面，胶片平面即在相机空间中z=near的平面。胶片平面在光栅空间中就是z=0的面，在光栅空间中，x，y的范围就是图片分辨率的x,y像素数。NDC空间归一化了光栅空间，所以x,y值在(0,0)到(1,1)之间

除了CameraBase类需要的参数外，ProjectiveCamera也需拿到投影变换矩阵、图像在屏幕空间的范围、焦距、和透镜光圈大小的参数。如果光圈不是一个无穷小的孔，那么图像中的一部分可能会变得模糊(在真实的透镜系统中，聚焦范围外的物体会模糊)。这种效果的模拟会在后面的章节详述

<<ProjectiveCamera的public方法>>

```c++
ProjectiveCamera(CameraBaseParameters baseParameters,
        const Transform &screenFromCamera, Bounds2f screenWindow,
        Float lensRadius, Float focalDistance)
    : CameraBase(baseParameters), screenFromCamera(screenFromCamera),
      lensRadius(lensRadius), focalDistance(focalDistance) {
    <<计算投影相机的变换矩阵>> 
}
```

ProjectiveCamera的实现类，会把从相机到屏幕的投影矩阵传到此处这个基类的构造器中，因此，这个构造器能轻松的计算光栅空间到相机空间过程中的其他所有变换

<<计算投影相机的变换矩阵>>

<<计算投影相机的屏幕变换矩阵>>

```c++
cameraFromRaster = Inverse(screenFromCamera) * screenFromRaster;
```

<<ProjectiveCamera的protected成员>>

```c++
Transform screenFromCamera, cameraFromRaster;
```

唯一在构造器中值得注意的变换是从屏幕到光栅的投影变换，它是通过组合NDC到光栅和从屏幕到NDC变换，2个步骤计算出来的。一个重要的细节是，y坐标会被最终转换所反转，这是必要的，因为在屏幕坐标中，y增加是在图像中上移，但是在光栅坐标中是下移

<<计算投影相机的屏幕(空间)转换>>

```c++
Transform NDCFromScreen =
    Scale(1 / (screenWindow.pMax.x - screenWindow.pMin.x),
          1 / (screenWindow.pMax.y - screenWindow.pMin.y), 1) *
    Translate(Vector3f(-screenWindow.pMin.x, -screenWindow.pMax.y, 0));
Transform rasterFromNDC =
    Scale(film.FullResolution().x, -film.FullResolution().y, 1);
rasterFromScreen = rasterFromNDC * NDCFromScreen;
screenFromRaster = Inverse(rasterFromScreen);
```

<<ProjectiveCamera的protected成员>>

```c++
Transform rasterFromScreen, screenFromRaster;
```

### 5.2.1 正交投影相机

正交投影相机把场景的矩形区域投影到区域对应的盒子区域的正面上。物体在这种投影法上，没有近大远小的变化，平行的线还是平行的，并且这种方式保持了物体间相对的距离，如图5.3:

![图5.3](img/fg5_3.png)
图5.3 正交观察矩形体是一个在相机空间里与坐标轴对齐的盒子，在其中的物体会被投影到z=近平面上

正交投影的图片看上去会显得缺乏深度感。但是平行的线还是能保持平行

正交投影相机OrthographicCamera的构造器用Orthographic()函数生成正交变换矩阵

### 5.2.2 透视投影相机

与正交投影相似的是，透视投影也会把整个空间投影到二维胶片平面上。然而，透视投影会有近大远小的效果。较远的物体投影后会比近处同等尺寸的物体更小，与正交相机不同的是，透视投影不会保持距离和角不变，并且平行线也不再平行。透视投影与人眼或相机镜片为3维世界生成图像的方式很接近

```c++
<<PerspectiveCamera的定义>>
class PerspectiveCamera : public ProjectiveCamera {
  public:
    <<PerspectiveCamera的Public方法>> 
  private:
    <<PerspectiveCamera的Private成员>> 
};
```

```c++
<<PerspectiveCamera的Public方法>>
PerspectiveCamera(CameraBaseParameters baseParameters, Float fov,
                  Bounds2f screenWindow, Float lensRadius, Float focalDist)
    : ProjectiveCamera(baseParameters, Perspective(fov, 1e-2f, 1000.f),
                       screenWindow, lensRadius, focalDist) {
    <<为透视相机的光线计算原点位置的微分变化量>> 
    <<为透视相机计算cosTotalWidth>> 
    <<为透视相机计算z=1时的图像平面区域>> 
    <<为透视相机计算最小的微分量>> 
}
```

透视投影描述了场景的透视观察效果。场景中的一点会投影在垂直于z轴的观察平面上。Perspective()函数计算这个变换。这个函数取视场角fov、近平面z值、和远平面z值

![图5.6](img/fg5_6.png)

图5.6 透视变换矩阵把相机空间中的点投影到近平面上。投影后的坐标x'和y'等于投影前x,y坐标除以z坐标。上图中，用箭头表示了投影的效果。投影后的z'之后会计算出来，那么近平面上一点会映射到z'=0的面上,远平面一点会映射到z'=1的面上

```c++
<<变换函数的定义>>
Transform Perspective(Float fov, Float n, Float f) {
    <<为透视投影执行投影的除法>> 
    <<把正则透视的视野缩放到这个fov>> 
}
```

变换过程非常简单，如下两步:

1. 相机空间中的点p投影到观察平面上。下方这些代数式表示了投影后的x',y'在观察面上能以被z除x,和y的方式来计算出来。投影后的z已被重新映射了，所以z的值在近平面上是0，远平面上是1.相关的计算如下:

    $$
    x'=x/z\\
    y'=y/z\\
    z'=\frac{f(z-n)}{z(f-n)}
    $$

    所有的这些计算可以用$4 \times 4$的矩阵编码，这个矩阵可以用在齐次坐标上。

    $$
    \begin{bmatrix}
    1 & 0 & 0 & 0\\
    0 & 1 & 0 & 0\\
    0 & 0 & {\frac{f}{f-n}} & {-\frac{fn}{f-n}}\\
    0 & 0 & 1 & 0\\
    \end{bmatrix}
    $$

    > $\frac{f}{f-n}$表示对z坐标的缩放
    >
    > $-\frac{fn}{f-n}$表示从z坐标的透视投影偏移

    <<为透视投影执行投影的除法>>

    ```c++
    SquareMatrix<4> persp(1, 0,           0,              0,
                        0, 1,           0,              0,
                        0, 0, f / (f - n), -f*n / (f - n),
                        0, 0,           1,              0);
    ```

2. 投影平面上的(x,y)会根据用户设定的视场角（fov）进行缩放，以便把视场内的点投影到观察平面[-1,1]坐标之间。对于方形图像来说，x和y都落在屏幕空间的[-1,1]之间。否则，更窄方向上的图像映射到[-1,1]，更宽的方向的图像按比例映射到更大的屏幕空间范围内。回想一下，正切是等于直角三角形的对边与邻边的比值。在此处邻边边长是1，故对边边长为$\tan(fov/2)$。用这个长度的倒数来缩放是把fov映射到[-1,1]上。

<<把正则透视的视野缩放到这个fov>>

```c++
Float invTanAng = 1 / std::tan(Radians(fov) / 2);
return Scale(invTanAng, invTanAng, 1) * Transform(persp);
```

如同OrthographicCamera那样，投影相机的构造器计算相机生成的光线在像素点上的偏移量的信息。在这种场景下，光线的原点是不变的，并且光线的微分量只在方向上不同。在此处，我们计算相应像素点位置在相机空间上的近投影面位置的改变量。

<<为透视相机的光线计算原点位置的微分变化量>>

```c++
dxCamera = cameraFromRaster(Point3f(1, 0, 0)) -
           cameraFromRaster(Point3f(0, 0, 0));
dyCamera = cameraFromRaster(Point3f(0, 1, 0)) -
           cameraFromRaster(Point3f(0, 0, 0));
```

<<PerspectiveCamera的Private成员>>

```c++
Vector3f dxCamera, dyCamera;
```

透视相机的fov最大角的余弦值有时候会很有用。尤其是在fov外的物体做快速剔除，可以用观察方向向量来与此值点乘，再与这个值比较来达成。余弦值可以通过相机的观察向量和图像某个角落的向量来计算出来(见图5.7)。这个角落的位置需做微小的调整(因为要考虑以每个像素为中心的滤波函数的宽度，这个宽度是用来根据它们的位置来做采样的权重值(详见章节8.8))

![图5.7](img/fg5_7.png)

图5.7 计算透视相机的最大观察角的余弦值。代表了PerspectiveCamera在观察方向上的边界的一个锥体，可以用相机的观察方向作为中心轴，计算这个轴与图像角落某点的夹角$\theta$的余弦来找到。在相机空间中，这个余弦值简化为该向量经过归一化的z分量值

<<为透视相机计算cosTotalWidth>>

```c++
Point2f radius = Point2f(film.GetFilter().Radius());
Point3f pCorner(-radius.x, -radius.y, 0.f);
Vector3f wCornerCamera = Normalize(Vector3f(cameraFromRaster(pCorner)));
cosTotalWidth = wCornerCamera.z;
```

<<PerspectiveCamera的Private成员>>

```c++
Float cosTotalWidth;
```

运用了透视投影后，相机空间的光线都从(0,0,0)的原点出发。一个光线的方向是由原点到近平面的点的向量给出。pCamera对应入参的CameraSample对象的pFilm的位置。换句话说，光线的向量方向的每个分量都相应等于这个点的位置量，所以，为了计算方向而做减法是没用的，我们直接用pCamera来初始化方向即可。

<<PerspectiveCamera的方法定义>>

```c++
pstd::optional<CameraRay> PerspectiveCamera::GenerateRay(
        CameraSample sample, SampledWavelengths &lambda) const {
    <<计算光栅量和相机样本的位置>> 
    Ray ray(Point3f(0, 0, 0), Normalize(Vector3f(pCamera)),
            SampleTime(sample.time), medium);
    <<为了景深效果而修改光线>> 
    return CameraRay{RenderFromCamera(ray)};
}
```

GenerateRayDifferential()也遵从了GenerateRay()的实现，除了下方这个额外的代码片段，此片段计算光线的微分量

<<为PerspectiveCamera的光线微分量计算偏移光线>>

```c++
if (lensRadius > 0) {
    <<在考虑镜头效果的情况下计算PerspectiveCamera的光线微分量>> 
} else {
    ray.rxOrigin = ray.ryOrigin = ray.o;
    ray.rxDirection = Normalize(Vector3f(pCamera) + dxCamera);
    ray.ryDirection = Normalize(Vector3f(pCamera) + dyCamera);
}
```

### 5.2.3 薄透镜模型和景深

理想化的针孔相机只允许光线通过单个点，然后到达胶片上，在现实是不可实现的。然而，让相机拥有极小的光圈是可行的，小的光圈允许相对更少的光照射到胶片感光器上，在这种场景下，需要更长时间的光照来捕获足够的光子来精确的拍到图像，代价是，当物体在快门打开的期间移动，会导致物体模糊。

真实的相机有镜片系统，会把光聚焦在一个有限尺寸下的光圈中，光线穿过此光圈照到胶片上。相机的设计师们(和摄影师们利用可调节大小的光圈)面临一个抉择：光圈越大，照射到胶片上的光越多，需要曝光的时间就越短。然而，镜头只能聚焦在单个平面上(焦距面)，离这个平面距离越远的物体，就越模糊，越大的光圈，这个效应越明显。

RealisticCamera类实现了一个对真实镜头系统的很精确的模拟。对于我们之前介绍的简单的相机模型来说，我们能应用一个经典的的光学近似方法，即薄透镜近似法，这种近似法利用传统计算机图形学投影模型，来对有限光圈的效应做建模。薄透镜近似法用一个球形轮廓的镜片的光学系统来建模，此透镜的厚度相对于镜片的曲率半径要小

在薄透镜近似法下，与光轴相平行的入射光会穿过透镜，并聚焦于透镜后的一点，这个点叫焦点。焦点到透镜的距离f叫焦距。如果胶片平面被安放在焦点处，那么无限远的物体会被聚焦，因为它们会在胶片上变成一个点

## 5.3 球形相机

此相机会在相机的一个点上收集所有方向的光，然后把点映射到图像对应的方向上，对应类是SpericalCamera

## 5.4 胶片和成像

在相机投影或镜头把在胶片上的场景的图像形成后，有必要对胶片如何测量光进行建模，来用渲染器生成最终图像。本章先总览辐射度量学中如何在胶片上测量光，然后继续讨论光谱能量如何被转换为三原色(RGB)，引出PixelSensor类，这个类不单做此处理，还做一些在相机中常见的其他处理。然后，考虑到胶片上的图像样本如何被累加到最终图像的像素点上，我们会引出Film接口和它的两个实现类，这两个实现类把这个模型做了实践。

### 5.4.1 相机测量方程

在模拟真实图像的形成过程时，精心定义胶片或相机感光器如何测量辐射亮度是很有必要的。从镜头的背面到达胶片的光线，携带了场景中的辐射量。对于胶片上的一点，也因此有了各种方向上的入射光的辐射量。离开镜头的辐射量的分布，受胶片上那个点看到的离焦模糊的量影响，图5.17分别展示了胶片上两个点看到从镜头射过来的辐射量的图像

![图5.17](img/fg5_17.png)

图5.17 从胶片两个点往镜头看去的场景图像 （a）从清晰聚焦的点看过去的样子，入射光辐射量在面上均匀恒定分布。(b)从非聚焦的区域看过去，可以看到场景的一小部分图像，也就是辐射量变化非常快。

给出了入射辐射量的函数，我们就能定义胶片上某点的入射光的辐照度。先从方程4.7开始，用辐射亮度来得到辐照度的定义，然后，我们可以利用方程4.9，把在立体角上的积分转换为在面积上的积分(在这种情况下，平面上某个区域$A_e$，与镜头的背面相切)。这样就可以得到在胶片平面上的点p的辐照度方程:

$$
E(p)=\int_{A_e}L_i(p,p')\frac{\vert\cos \theta \cos \theta'\vert}{\|p'-p\|^2}dA_e
$$

图5.18展示了这种场景下的几何关系:

![图5.18](img/fg5_18.png)

图5.18：辐照度测量方程的几何设置。当光线通过点p'到胶片上的点p时，点p'在镜头背部透镜相切的面上，辐射亮度可以被测量。z是从胶片平面到镜头背部相切的平面的距离，$\theta$ 是从 p' 到 p 的向量与光轴之间的夹角。

由于胶片平面平行于镜片平面,即$\theta = \theta'$。因此我们可利用p和p'的距离等于镜头到胶片面的轴距离(用z轴表示)除以$\cos \theta$求得，整理一下，可得:

$$
E(p)=\frac{1}{z^2}\int_{A_e}L_i(p,p')\vert\cos^4\theta\vert dA_e \tag{5.3}
$$

对于胶片范围比距离z相对较大的相机，$\cos \theta$项可以显著降低入射光辐照度，此项也会导致黑边现象。大部分现代数码相机利用预设的矫正因子来增加感光器边缘的像素值，纠正了此效果。

在快门打开的时间内，对胶片上某点的辐照度进行积分，可得出辐射曝光量，即是单位面积能量的辐射单位，$J/m^2$:

$$
H(p)=\frac{1}{z^2}\int_{t_0}^{t_1}\int_{A_e}L_i(p,p',t')\vert\cos^4\theta\vert dA_e dt' \tag{5.4}
$$

(辐射曝光量也叫辐射功率)。在某点测量辐射曝光量可反映出：胶片平面收到的能量的多少，部分取决于相机快门打开的时间长度。

摄影用的胶片(或数码相机里的CCD, CMOS感光器)是在微小面积下测量辐射能量。基于方程5.4，且在感光器像素面积$A_e$下积分,我们可得:

$$
J(p)=\frac{1}{z^2}\int_{A_p}\int_{t_0}^{t_1}\int_{A_e}L_i(p,p',t')\vert\cos^4\theta\vert dA_e dt' dA_p \tag{5.5}
$$

上式代表了到达某个像素点的焦耳，此式被称为相机测量方程

虽然这些因子应用到了本章介绍的所有相机模型，但是它们只包含在RealisticCamera实现类中。原因很务实：大部分渲染器不会对此效果做建模，所以在更简单的相机模型忽略此效果，能让pbrt渲染的图像与其他系统渲染的图像对比时更容易。

### 5.4.2 感光器响应的建模

传统的胶片是基于一种化学反应，当卤化银晶体暴露在光照下时，会生成溴化银。卤化银对蓝色十分敏感，但是可以把晶体分成多层，中间用彩色滤光片和染料把每层分开，来捕捉彩色的图像

现代数码相机使用CCD或者CMOS感光器，这些感光器使每个像素在光照下会把一定数量的光子转换为电能。捕获彩色图像的方法各种各样，但是最常见的是用彩色滤光片在每个像素上覆盖，只计算彩色滤光片允许通过的红绿蓝三种颜色的光子数。每个像素一般也会有一个微镜片，增加感光器能捕获到的光量。

对于胶片和数码感光器，像素的颜色测量都可以使用光谱响应曲线来建模，该曲线可以把彩色滤光片或胶片对光的化学响应与波长的关系表现出来。

给定一个入射光的光谱分布$s(\lambda)$, 一个像素的红光部分如下式:

$$
r = \int s(\lambda)\overline{r}(\lambda)d\lambda \tag{5.6}
$$

数码感光器的像素点一般是以马赛克方式排布，绿色像素比红蓝像素多2倍，因为人类的视觉系统对绿光更敏感。为了把感光器像素转换为图像像素的颜色，需要采用去马赛克算法，这也是像素马赛克的一个体现。

采用四边形马赛克像素并按原样使用其颜色值的简单方法效果不佳，因为组成感光器像素的位置略有不同

> 1. 一个感光器对应一种颜色，感光器上面有彩色滤光片，只能通过红绿蓝三种光中的一种
> 2. 感光器的每个像素点对应一种颜色，多个感光器像素点的排布方式就叫马赛克排布，由于人眼对绿色更敏感，绿色像素点是红蓝数量的两倍
> 3. 多个感光器像素点最终通过解马赛克算法合成一个图像像素点的颜色，也就是多个感光器像素点对应图像中一个像素点
> 4. 在马赛克分布里，感光器的像素点的位置不是均匀的，分布方式有好几种，具体请查阅资料

设计数码感光器有诸多挑战，大部分挑战是来自像素点的尺寸需要极小，因为图像需要高分辨率。像素点越小，打到其上的光子越少，导致精确衡量光照量就越难。像素阵列也会遭遇各种类型的噪点，其中，散粒噪点(shot noise)是最主要的一种，这种噪点是由于光子的离散性导致的：捕获到的光子中有一些随机扰动，会导致捕获到的光子一会多一会少。散粒噪点可以用泊松分布来建模

为了使传统胶片产生足够的化学反应，或者使光子被感光器充分捕获，每个像素必须接收到足够的光量。在方程5.5中，我们可知每个像素捕获的能量取决于入射光的光辐射量，像素面积，出瞳面积，和曝光时间。对于特定相机的实现，像素面积是固定的，为了增加光照而增加镜片光圈面积和曝光时间都可能导致非预期的副作用。更大的光圈减少了景深效应，这可能导致非预期的失焦模糊。更长的曝光时间也会由于场景中物体的运动或相机在快门开启的时候的运动导致模糊。感光器和胶片因此提供了在ISO配置下的额外控制。

对于现实中的胶片，ISO把对光的感应度进行了量化(ISO值越高，图像需要的光越少)。在数码相机中，ISO控制了增益大小-即从感光器读出的像素值的缩放因子。对于现实中的相机，增加增益值会导致噪点增多(像素点初始的光量加了倍率)。由于pbrt不会依据现实中的感光器的噪点现象来建模，ISO值会根据需要的曝光度来设置一个值

在pbrt的感光器模型中，我们不会对马赛克和噪点做建模，也不会对其他现象比如泛光效果(当曝光时间足够长时，这个像素点的光会增加，并且周围的像素点光也会增加)。我们也不会模拟从感光器中读出图像的处理过程(许多相机使用滚动快门，逐行读取扫描线)。对于高速移动的物体，这会导致意想不到的效果。本章末的练习会以不同方式修改pbrt，以讨论这些效果

PixelSensor类实现了pbrt的半理想的像素色彩度量模型。定义于film.h和film.cpp下

<<PixelSensor的定义>>

```c++
class PixelSensor {
  public:
    // <<PixelSensor Public Methods>> 
    // <<PixelSensor Public Members>> 
  private:
    // <<PixelSensor Private Methods>> 
    // <<PixelSensor Private Members>> 
};
```

PixelSensor对感光器的像素色彩度量进行了半理想化的建模:

1. 曝光控制: 图像明暗可以由用户控制
2. RGB感应：基于光谱感应曲线，模拟光谱辐射量到三原色的转换
3. 白平衡： 相机对捕获的图像进行处理，包括对初始RGB值根据光照色彩来调整，
   来模拟人类视觉系统中的色彩适应过程。因此，捕获的图像在视觉上看起来与
   人类观察者在拍照时记忆中的图像相似

pbrt包含了一个真实感相机模型，也是基于投影矩阵的理想模型。因为针孔相机有一个无限小的光圈。我们在PixelSensor在实现上做了程序上的折衷，这样的话用针孔模型渲染的图像就不是完全黑的。我们留下了Camera的响应作用，用来对光圈尺寸效应的建模。理想化的模型不会考虑此，尽管RealisticCamera在<<为RealisticCamera光线计算权重>>里确实这样做了。PixelSensor之后只会考虑快门时间和ISO的设置。这两个因素被包含在一个量里，叫做成像比(imaging ratio)。

PixelSensor构造器把感光器的RGB匹配函数-$\bar{r},\bar{g}$和$\bar{b}$, 和成像比作为参数。同时，它也会取用户要求的颜色空间，来最终输出RGB值，同时，根据把哪种颜色视为白色，会取这个光照的光谱。把这些参数合在一起，就能够把感应器测量出来的光谱能量，转换为RGB,然后根据输出的颜色空间，转换为RGB值。

图5.19表示了对相机响应建模的效果，对比用XYZ匹配函数来计算初始像素值来渲染，和用确切相机感光器的匹配函数来渲染。

```c++
<<PixelSensor Public Methods>>
PixelSensor(Spectrum r, Spectrum g, Spectrum b,
       const RGBColorSpace *outputColorSpace, Spectrum sensorIllum,
       Float imagingRatio, Allocator alloc)
    : r_bar(r, alloc), g_bar(g, alloc), b_bar(b, alloc),
      imagingRatio(imagingRatio) {
    <<从相机RGB矩阵计算XYZ>> 
}
```

```c++
<<PixelSensor Private Members>>
DenselySampledSpectrum r_bar, g_bar, b_bar;
Float imagingRatio;
```

感光器像素记录光的RGB颜色空间与用户定义的最终图像的RGB颜色空间一般来说不一样。颜色空间的形式一般来说是针对某种相机的，并且受它的像素的颜色滤光片的物理属性决定，最终图像的RGB颜色空间，类似sRGB的颜色空间，或在4.6.3中的其他颜色空间的一种，一般来说，是根据设备不同而不同。因此，PixelSensor的构造器计算一个$3 \times 3$的矩阵，用来把RGB空间转换为XYZ。从这里，能很轻松的转换为特定的输出颜色空间。

这个矩阵的发现是从解决一个优化问题中得来的。它开始于超过二十种光谱分布，表示来自标准色卡上各种颜色色块的反射率。这个构造器会计算这些色块的RGB颜色(在相机颜色空间中，也包括输出颜色空间光照下的XYZ颜色)。如果这些颜色被对应列向量所代表，那么我们能考虑一个$3 \times 3$的矩阵M下的问题:

$$
M
\begin{bmatrix}
r_1 & r_2 & {\cdots} & r_n \\
g_1 & g_2 & {\cdots} & g_n \\
b_1 & b_2 & {\cdots} & b_n \\
\end{bmatrix} \approx
\begin{bmatrix}
x_1 & x_2 & {\cdots} & x_n \\
y_1 & y_2 & {\cdots} & y_n \\
z_1 & z_2 & {\cdots} & z_n \\
\end{bmatrix}
$$

同时，只要有超过三种反射率值，问题就会成为一个超定问题，可以通过线性最小二乘法来求解。

```c++
<<从相机RGB矩阵计算XYZ>>
<<计算用于训练的色块的相机RGB值>>
<<为用于训练的色块计算xyzOutput值>>
<<利用线性最小二乘法来初始化XYZFromSensorRGB>>
```

给定感光器的光照量，为每个被ProjectReflectance()处理的反射率计算RGB的相关系数

```c++
<<计算用于训练的色块的相机RGB值>>
Float rgbCamera[nSwatchReflectances][3];
for (int i = 0; i < nSwatchReflectances; ++i) {
    RGB rgb = ProjectReflectance<RGB>(swatchReflectances[i], sensorIllum,
                                      &r_bar, &g_bar, &b_bar);
    for (int c = 0; c < 3; ++c)
        rgbCamera[i][c] = rgb[c];
}
```

为了得到比较好的结果，被用于改良问题的光谱应该代表了一个比较好的表示现实世界的光谱的量。在pbrt中使用的是基于标准色谱表光谱测量量。

```c++
<<PixelSensor Private Members>>
static constexpr int nSwatchReflectances = 24;
static Spectrum swatchReflectances[nSwatchReflectances];
```

ProjectReflectance()工具方法取某个反射率取其光谱分布，光照量，还有三个光谱匹配函数$\bar{b_i}$(对应某种三原色的颜色空间)。这个方法返回三个相关系数$c_i$，由下式给出:

$$
c_i = \int r(\lambda)L(\lambda)\bar{b_i}(\lambda)d\lambda
$$

r是光谱反射率函数，L是光照的光谱分布，$\bar{b_i}$是某个光谱匹配函数。在第二个匹配函数$\bar{b_2}$为流明(光照度)或至少为绿色的假设下(人眼最敏感的颜色)，返回的颜色的三个项，都会被$\int L(\lambda)\bar{b_2}(\lambda)d\lambda$归一化。在这种方法下，线性最小二乘法的拟合结果，至少是根据视觉上的重要程度粗略地对每对RGB/XYZ进行了加权的结果。

ProjectReflectance()函数取三色类型的色彩空间作为模板参数，因此能够返回RGB或XYZ的值。其实现与Spectrum::InnerProduct()类似，以间隔1nm的波长来计算出黎曼和，此处略

```c++
<<PixelSensor Private Methods>>= 
template <typename Triplet>
static Triplet ProjectReflectance(Spectrum r, Spectrum illum,
                                  Spectrum b1, Spectrum b2, Spectrum b3);
```

在输出色彩空间里计算XYZ相关因子的代码片段\<\<Compute xyzOutput values for training swatches\>\>中，大部分与RGB的类似，只是其输出的是光照量和XYZ光谱匹配函数，并且初始化了xyzOutput数组，此处略

给出这两个颜色相关因子矩阵后，调用LinearLeastSquares()函数来解决公式5.7的优化问题

```c++
<<Initialize XYZFromSensorRGB using linear least squares>>= 
pstd::optional<SquareMatrix<3>> m =
    LinearLeastSquares(rgbCamera, xyzOutput, nSwatchReflectances);
if (!m) ErrorExit("Sensor XYZ from RGB matrix could not be solved.");
XYZFromSensorRGB = *m;
```

由于RGB和XYZ颜色是利用对应光照度的颜色空间来计算的，矩阵M也会进行白平衡

```c++
<<PixelSensor Public Members>>= 
SquareMatrix<3> XYZFromSensorRGB;
```

PixelSensor的第二个构造器的光谱响应曲线使用了XYZ匹配函数。若某个特定相机传感器没有在场景描述文件中定义，默认就是用此构造。注意，此种情况下的成员变量r_bar,g_bar,b_bar实际代表的是X,Y,Z

```c++
<<PixelSensor Public Methods>>+=  
PixelSensor(const RGBColorSpace *outputColorSpace, Spectrum sensorIllum,
       Float imagingRatio, Allocator alloc)
    : r_bar(&Spectra::X(), alloc), g_bar(&Spectra::Y(), alloc),
      b_bar(&Spectra::Z(), alloc), imagingRatio(imagingRatio) {
    <<Compute white balancing matrix for XYZ PixelSensor>> 
}
```

默认情况下，当PixelSensor转换到XYZ相关系数时，是不会有白平衡的。这个过程是在后处理中完成。然而，若用户确实指定了某个色温，那么白平衡就会被XYZFromSensorRGB矩阵处理(否则其就是一个单位矩阵)。此处的WhiteBalance()函数简单介绍如下：其会取两种颜色空间中白色点的色度，然后返回一个矩阵，把第一种颜色空间映射到第二种颜色空间上。

```c++
<<Compute white balancing matrix for XYZ PixelSensor>>= 
if (sensorIllum) {
     Point2f sourceWhite = SpectrumToXYZ(sensorIllum).xy();
     Point2f targetWhite = outputColorSpace->w;
     XYZFromSensorRGB = WhiteBalance(sourceWhite, targetWhite);
}
```

PixelSensor提供的主要函数是ToSensorRGB()，此函数把一个以点采样,且在某个SampledSpectrum中的的光谱分布$L(\lambda_i)$转换为在感应器颜色空间中的RGB相关因子。感应器的相应积分是根据公式5.6，通过蒙特卡洛法计算出来的。估计式形式如下:

$$
r \approx \frac{1}{n}\sum_i^n\frac{L(\lambda_i)\bar{r}(\lambda_i)}{p(\lambda_i)} \tag{5.8}
$$

此处的n等于NSpectrumSamples。对应的PDF值可从SampledWaveLengths取得，并且根据波长的总和然后除以n的操作是由SampledSpectrum::Average()方法处理。这些相关因子会被成像率缩放，最终完成转换过程

```c++
<<PixelSensor Public Methods>>+= 
RGB ToSensorRGB(SampledSpectrum L,
                const SampledWavelengths &lambda) const {
    L = SafeDiv(L, lambda.PDF());
    return imagingRatio *
        RGB((r_bar.Sample(lambda) * L).Average(),
            (g_bar.Sample(lambda) * L).Average(),
            (b_bar.Sample(lambda) * L).Average());
}
```

#### 色彩适应和白平衡

#### 感光器响应的采样

由于在PixelSensor中的感光器响应函数描述了感光器根据辐射量给出的基于波长的响应，当对光的波长进行采样时，至少近似地估计辐射量的变化是有意义的。最少，辐射量为0的光线波长应该不被采样，这样的话，波长就对最终图像没有贡献了。更普遍的来说，根据感光器响应函数来做重要性采样是必要的，这种方式提供了减少5.8估计式的错误。

然而，选取采样所用的分布具有挑战性，因为目标是最小化人类观察到的错误，而不是严格地最小化数字上的错误。图5.21(a)展示了CIE Y匹配函数和X,Y,Z的总和的匹配函数，两者都可能被使用。实际中，根据Y来采样会给出超量的色彩噪点，但是基于三个匹配函数的和来采样会导致在400~500nm间波长的样本过多，这些样本在视觉呈现上相对不太重要。

一个参数化的概率分布函数，用于平衡这些关注点，并且对于在可见波长段中采样挺好用的,即下式:

$$
p_v(\lambda)=(\int_{\lambda_{min}}^{\lambda_{max}}f(\lambda)d\lambda)^{-1}f(\lambda) \tag{5.9}
$$

其中:

$$
f(\lambda)=\frac{1}{\cos h^2(A(\lambda - B))}
$$

$A = 0.0072nm^{-1}, B=538nm$,图5.21(b)展示了$p_v(\lambda)$的图

![图5.21](img/fg5_21.png)
图5.21 (a) CIE Y 匹配函数的归一化的PDF，和X,Y,Z总的的匹配函数 (b) 根据方程5.9推出的参数化的分布$p_v(\lambda)$

### 5.4.3 图像采样的滤波

Film的实现主要负责把每个像素点的光谱样本聚合在一起，以便计算最终的像素值。在现实中的相机，每个像素会在很小的区域上聚集光。在这个小区域的空间上，像素的感应可能随空间变化，这取决于感光器的物理设计。在第八章，我们会从信号处理的角度来考虑此效果，并且我们会了解图像函数的采样细节和样本的加权如何极大的影响图像的最终质量

为了处理这些细节，现在我们会假设一些滤波函数$f$，用来定义感光器在图像像素点周围随空间的响应变化量。这些滤波函数很快就会趋于0，这样做是模拟现实里胶片上的像素点只对接近这个点的光做出响应。同时，这些滤波器也模拟了像素点响应的空间上的其他变化量。利用此方式，若我们有一个图像函数$r(x,y)$，用于在胶片任意位置给出红色值(比如利用方程5.6中，利用感光器响应函数$\bar{r}(\lambda)$来测量的值)，然后，滤波后在点(x,y)的红色值$r_f$就被下式给出:

$$
r_f(x,y) = \int f(x-x', y-y')r(x',y')dx'dy' \tag{5.10}
$$

上式中，假设$f$是积分到1

> $f(x-x', y-y')$表明f这个滤波函数是根据实际点到像素点距离来求得具体响应多少量
>
> $r(x',y')$代表感光器对于像素区域内某点，应该给出红色量的多少
>
> **为什么是积分?**
>
> 像素点在中心，滤光片盖在上面，一般是矩形的，会对矩形范围内的光，根据位置做出响应，负责对像素点周围的光进行采集，所以实质上是对滤光器范围内的面上做积分

照例，我们会利用该点的采样，来估计积分值，估计式如下:

$$
r_f(x,y) \approx \frac{1}{n}\sum_i^n\frac{f(x-x', y-y')r(x',y')}{p(x_i,y_i)} \tag{5.11}
$$

> 还是使用蒙特卡洛来估计积分值，分母p就是由采样法给出某点的pdf

为了对被积函数进行采样，此处用了2种方法，第一种在之前的pbrt版本也用到了，即对图像均匀采样，每个图像样本可能会对多个像素点的最终值产生贡献，这取决于所使用的滤波函数的范围。此方法给出了如下估计式:

$$
r_f(x,y) \approx \frac{A}{n}\sum_i^n f(x-x', y-y')r(x',y') \tag{5.12}
$$

> 均匀采样的pdf为$\frac{1}{A}$，所以可以放到分母上

上式中A是胶片的面积，图5.23解释了此方法:某个位于(x,y)的像素点，像素点有一个像素滤波器，范围是radius.x(代表x方向的长度)，radius.y(代表y方向的长度).所有在滤波器范围内的$(x_i,y_i)$样本，会对像素点的值做出贡献，贡献量的多少取决于滤波函数$f(x-x_i,y-y_i)$的值

![图5.23](img/fg5_23.png)

图5.23 **二维图像的滤波**.为了计算在(x,y)的实心圆点的滤波后的像素值，在这个矩形区域radius.x，radius.y里的所有的图像样本都需要考虑。每个图像样本$(x_i,y_i)$使用空心圆点表示，使用二维滤波函数$f(x-x_i,y-y_i)$来得到权重值,所有样本的总的平均权重就是最终像素的值

虽然方程5.12给出了像素值的无偏估计，但是，滤波函数中的变化会导致估计值的变化。考虑一个常量图像函数r的场景，在这种场景下，我们期望最终图像像素的结果都是r。然而，滤波函数$f(x-x_i,y-y_i)$的累加值并不是等于1的，其只在期望值上等于1。因此，这个图像甚至在简单的场景下，都会存在噪点。如果改为用下式来估计:

$$
r_f(x,y) \approx \frac{\sum_i^n f(x-x', y-y')r(x',y')}{\sum_i^n f(x-x', y-y')}
$$

那么滤波函数带来的变化就会被消除，代价是微小的估计偏差(这就是带权重的重要性采样的蒙特卡洛估计)，在实际场景中，这种取舍是值得的。

公式5.10也可以独立估计出每个像素的值。此方法就由本版本pbrt所使用。在这种情况下，利用基于滤波函数分布来在胶片上采样点是值得的。此方式被称为滤波重要性采样。利用此方式，滤波器在空间中的变化量是用像素的样本位置的分布来计算的，而不是根据滤波器的值来缩放每个样本的贡献量来计算的。

若$p\propto f$,那么在方程5.11的这两个因子就可以消除，那么只剩下$r(x_i,y_i)$的平均值，采样值被一个常数比例缩放。然而，我们必须处理少见(对于渲染来说)的积分估计值位负的情况，就如在第八章所见，滤波函数中的一部分为负，比起非负来说，能得出更好的结果。在那种情况下，我们有$p \propto \vert f\vert$,可得:

$$
r_f(x,y) \approx (\int\vert f(x',y')\vert dx'dy')(\frac{1}{n}\sum_t^n sign(f(x-x_i,y-y_I))r(x_i,y_i))
$$

此处sign(x)当x>0时等于1，x=0时等于0，其他情况等于-1。然而这个估计式与方程5.12有相同的问题，即便有一个常数的函数r，这个估计式依然会有变化，这个变化取决于有多少sign函数得出1或-1

因此，本版本的pbrt继续是用加权的重要性采样估计法来计算像素值，用下式:

$$
r_f(x,y) \approx \frac{\sum_i w(x-x_i,y-y_i)r(x_i,y_i)}\sum_i{w(x-x_i,y-y_i)} \tag{5.13}
$$

上式中$w(x,y)=\frac{f(x,y)}{p(x,y)}$

最开始的两种方式具有一种优势，即每个图像样本可以对多个像素的最终滤波后的值产生贡献。这对于渲染效率是有益处的，因为所有涉及到为某个图像样本进行的计算，都能被用于改善多个像素点的准确度。为其他像素点生成的样本不总是有用，在第八章中某些样本生成算法的实现，以保证在像素内良好覆盖为目的来精心选择样本的位置。如果其他像素的样本混在其中，那么一个像素的所有样本集就不再有那种结构，这会导致误差增加。由于滤波重要性采样不在像素间共享样本，所以不会有此问题。

滤波重要性采样还有更多优势，这种方式让渲染并行化更简单，若渲染器是以不同线程处理不同像素的方式并行，那么多个线程就没有机会来并发修改同一个像素的值。最后一个优点是，若有任何由于使用很差的采样积分造成的差异尖峰导致的一些样本非常亮，那么这些样本只会对一个像素做贡献，而不是在多个像素上涂抹开。修复单个像素的人造痕迹比起被这种样本影响周遭一片像素来说还是更简单。

### 5.4.4 Film接口

胶片接口定义在film.h文件中

SpectralFilm在代码里没有提到，这个类用于记录在某个特定波段的光谱图像，这个图像被离散化为不重叠的范围内。详见pbrt的文件格式的文档

样本可通过两种方式提供给胶片。第一种方法是在积分器估计辐射亮度时，让Sampler从胶片上选择某点。这些样本通过AddSample()函数提供给Film对象，addSample()包含如下参数:

- 样本的像素点坐标,pFilm
- 样本的光谱辐射亮度，L
- 样本的波长, lambda
- 一个可选的VisibleSurface，用来描述第一个可视的点的几何特征
- 权重值：由Filter::Sample()返回，用于计算时使用

Film的实现假设多个线程不会并发时，用同一个点的pFilm调用AddSample()(然而Film的实现应该假定线程会同时在不同的pFilm点并行调用)。因此，不必担心这个函数的实现存在互斥问题，对于某个像素，一些数据并不独有的时候除外

Film接口也包含了一个函数，返回所有可能生成的样本的边框。注意，这个边框跟图像像素点的边框不一样，通常情况下，像素滤波器范围比单个像素更宽

VisibleSurface持有某个表面上的某点的各种信息，除了点，法线，着色法线，时间外，还储存了每个像素点的偏导数$\frac{\partial z}{\partial x}$和$\frac{\partial z}{\partial y}$, x和y是在光栅空间，z在相机空间。这些值在图像的去噪算法中很有用，因为它们使得检测平面中相邻像素是否共面成为可能。表面的反射率(albedo)是在均匀光照下的反射光的光谱分布，这个量在去噪前把纹理从光照中分离是很有用的

我们不会在此包含VisibleSurface的构造器，这个构造器的主要功能就是从SurfaceInteration对象拷贝值到自己的成员变量上。

成员变量set是用来检测VisibleSurface是否已被初始化

Film的实现能调用usesVisibleSurface()来检测是否传了*VisibleSurface到addSample()里。若VisibleSurface没传，这个函数能允许积分器避免初始化VisibleSurface的昂贵开销

从光源开始对路径进行采样的光传输算法(比如双向路径算法), 要求把贡献值“溅射"到任意像素上的能力，比起把计算最终像素值当成一个加权的溅射贡献平均值来说，这里的贡献值是被简单累加的。一般来说，在给定像素点周围"溅射"到的贡献值越多，那么像素就越亮。addSplat()把某个值溅射到图像中的某个位置

对比AddSample(), addSplat()函数可以被多个最终会更新同一个像素的线程同时并行调用，因此，Film的实现，在实现这个函数的时候，必须实现互斥，或者原子操作。

Film的实现必须提供SampleWavelengths()函数，来从胶片感光器能响应的波段采样。(比如，通过SampledWavelengths::SampleVisible())

此外，这些实现也必须提供一些方便操作的函数，来获取图像的范围，感光器的对角线的长度，单位是米

WriteImage()函数会处理并生成最终图像，然后把其存到文件里。由于相机空间变换，这个函数取了一个缩放因子，应用到样本上，提供给addSplat()函数

### 5.4.5 胶片的通用功能

Film的实现类继承自FilmBase类，这个类里有通用的成员函数和Film的部分接口的函数实现

FilmBase的构造器入参有这些值:

- 图像的整体分辨率
- 整个图像的边界框
- 滤波器
- 像素感光器
- 胶片的对角线长度
- 输出图片的文件名

这些参数都被打包放到了FilmBaseParameters结构体中，用来减少入参长度

```c++
struct FilmBaseParameters {
    Point2i fullResolution;
    Bounds2i pixelBounds;
    Filter filter;
    Float diagonal;
    const PixelSensor *sensor;
    std::string filename;
};
```

FileBase的构造器之后会从这个结构体里复制这些值，把胶片的对角线长度从毫米转换为米，因为在pbrt里，测量距离的单位是米

<<FilmBase的public方法>>

```c++
FilmBase(FilmBaseParameters p)
    : fullResolution(p.fullResolution), pixelBounds(p.pixelBounds),
      filter(p.filter), diagonal(p.diagonal * .001f), sensor(p.sensor),
      filename(p.filename) {
}
```

<<FilmBase的protected成员>>

```c++
Point2i fullResolution;
Bounds2i pixelBounds;
Filter filter;
Float diagonal;
const PixelSensor *sensor;
std::string filename;
```

拥有这些值可以用来直接实现Film接口的一些函数

<<FilmBase的public方法>>

```c++
Point2i FullResolution() const { return fullResolution; }
Bounds2i PixelBounds() const { return pixelBounds; }
Float Diagonal() const { return diagonal; }
Filter GetFilter() const { return filter; }
const PixelSensor *GetPixelSensor() const { return sensor; }
std::string GetFilename() const { return filename; }
```

SampleWavelengths()的一个实现，根据方程5.9的分布值进行采样

<<FilmBase的public方法>>

```c++
SampledWavelengths SampleWavelengths(Float u) const {
    return SampledWavelengths::SampleVisible(u);
}
```

SampleBounds()方法只需给定Filter，也可以被简单实现。计算样本的边界范围涉及到滤波器半径和半个像素的偏移量(此偏移量来自pbrt中对于像素坐标的例行处理方法)。详见8.1.4

<<FilmBase的方法定义>>

```c++
Bounds2f FilmBase::SampleBounds() const {
    Vector2f radius = filter.Radius();
    return Bounds2f(pixelBounds.pMin - radius + Vector2f(0.5f, 0.5f),
                    pixelBounds.pMax + radius - Vector2f(0.5f, 0.5f));
}
```

### 5.4.6 RGBFilm类

RGBFilm记录图像呈现的RGB色彩值

<<RGBFilm的定义>>

```c++
class RGBFilm : public FilmBase {
  public:
    // <<RGBFilm的public方法>> 
  private:
    // <<RGBFilm::Pixel的定义>> 
    // <<RGBFilm的Private成员>> 
};
```

为了处理从FilmBase传过来的参数，RGBFilm取了一个颜色空间用来输出图像，这个参数允许定义一个RGB颜色分量的最大值，同时有一个参数，用来控制输出图像的浮点精确度

<<RGBFilm的方法定义>>

```c++
RGBFilm::RGBFilm(FilmBaseParameters p, const RGBColorSpace *colorSpace,
                 Float maxComponentValue, bool writeFP16, Allocator alloc)
    : FilmBase(p), pixels(p.pixelBounds, alloc), colorSpace(colorSpace),
      maxComponentValue(maxComponentValue), writeFP16(writeFP16) {
    filterIntegral = filter.Integral();
    // <<计算outputRGBFromSensorRGB矩阵>> 
}
```

滤波器的积分函数用于归一化通过AddSplat()提供的样本的滤波器的值，所以它也被缓存到了成员变量里

<<RGBFilm的private成员>>

```c++
const RGBColorSpace *colorSpace;
Float maxComponentValue;
bool writeFP16;
Float filterIntegral;
```

最终图像的颜色空间是用用户定义的RGBColorSpace类来给出，这个类与感光器的RGB颜色空间不太可能相同。这个构造器因此计算一个$3 \times 3$的矩阵，用来把感光器的RGB值转换为输出的颜色空间值

<<计算outputRGBFromSensorRGB矩阵>>

```c++
outputRGBFromSensorRGB = colorSpace->RGBFromXYZ *
    sensor->XYZFromSensorRGB;
```

<<RGBFilm的private成员>>

```c++
SquareMatrix<3> outputRGBFromSensorRGB;
```

给定图像(可能是被裁剪过的)的像素分辨率，构造器分配一个二维像素数组结构，每一个元素就是一个像素点。运行中的加权像素贡献量的总和，是在rgbSum这个成员变量里以RGB的颜色的方式呈现。 weightSum持有了像素点采样的光贡献量的滤波器加权后的值的总和。这些分别对应了方程5.13中的分子和分母。最后，rgbSplat持有了一个(未加权)的样本溅射量的总和。

所有的这些量都是用双精度浮点数实现的。单精度浮点数一般是很高效，但是当用于大量样本的图像渲染时，关联的总量值的精度就不太行。虽然在视觉上能看出来的错误很少见，但是在衡量蒙特卡洛采样算法的错误的时候，容易引起问题。

图5.24展示了这个问题的例子，我们为每个像素取了四百万样本来渲染一个测试场景的图像， 是用32位或64位浮点值作为RGBFilm的像素值，然后，我们根据样本数量画出了方差(MSE)。对于无偏的蒙特卡洛估计式，当取n个样本时，MSE是O(1/n)，在双对数图中，应该是斜率-1的直线。然而，我们可见当n>1000时，32位浮点的参考图像,MSE就平了，
也就是更多的采样不会减少错误了。如果用64位浮点数，还是曲线会如期望那样继续往下走。

![图5.24](img/fg5_24.png)
图5.24，MSE根据样本数量的变化情况。当渲染场景时，用无偏蒙特卡洛估计式时，我们期望MSE与样本数n相关，算法复杂度O(1/n)。在双对数图里，这个错误率对应了一条斜率为-1的直线。考虑我们使用的测试场景，我们能看到参考图像是用32位浮点数时，当n大于1000时，报错率就下不去了。

> 也就是说，当每个像素采样率约大于1000后，由于单精度浮点数的原因，会导致样本数量继续增加的时候，错误率的减小就大不如前，这种场景下，就需要考虑用双精度浮点数

<<RGBFilm::Pixel的定义>>

```c++
struct Pixel {
    double rgbSum[3] = {0., 0., 0.};
    double weightSum = 0.;
    AtomicDouble rgbSplat[3];
};
```

<<RGBFilm的private成员>>

```c++
Array2D<Pixel> pixels;
```

RGBFilm不会把VisibleSurface指针传给AddSample()

<<RGBFilm的public方法>>

```c++
bool UseVisibleSurface() const {return false;}
```

AddSample()在根据pFilm点更新像素前，把光谱辐射转换为感光器的RGB值

<<RGBFilm的public方法>>

```c++
void AddSample(Point2i pFilm, SampledSpectrum L,
       const SampledWavelengths &lambda,
       const VisibleSurface *, Float weight) {
    // <<把样本的辐射量转换为PixelSensor的RGB值>>  
    // <<(可选)感光器的RGB值做夹拢>> 
    // <<用已被滤波器处理的样本贡献量来更新像素值>> 
}
```

辐射量首先被感光器转换为RGB值

<<把样本的辐射量转换为PixelSensor的RGB值>>

```c++
RGB rgb = sensor->ToSensorRGB(L, lambda);
```

用蒙特卡洛积分渲染的图片，当使用的采样分布与被积函数匹配不够好时，能表现出亮噪点像素，是由于当$\frac{f(x)}{p(x)}$用蒙特卡洛估计时，$f(x)$非常大，$p(x)$非常小导致的(这种像素点俗称萤火虫)，对于这种像素点，可能需要许多其他的样本来得到更精确的估计。

去除萤火虫点的广泛做法是，把样本的贡献值都夹拢在最大量中。这么做会一引入一些错误：能量丢失了，然后图像不再是无偏于真实图像的。然而，当图像更关注美学而不是数学时，这种方式还是挺有效的。

RGBFilm的maxComponentValue参数能被设置为夹拢的阈值，默认是无穷大，即不夹拢

<<可选操作：夹拢感光器的RGB值>>

```c++
Float m = std::max({rgb.r, rgb.g, rgb.b});
if (m > maxComponentValue)
    rgb *= maxComponentValue / m;
```

给定一个可能被夹拢的RGB值，在其中的像素点会通过把光贡献量加到方程5.13的分子分母上

<<用滤波器处理过的样本贡献量更新像素值>>

```c++
Pixel &pixel = pixels[pFilm];
for (int c = 0; c < 3; ++c)
    pixel.rgbSum[c] += weight * rgb[c];
pixel.weightSum += weight;
```

AddSplat()方法首先重用从AddSample()的前两个部分，来计算对应辐射量L的RGB值

<<RGBFilm方法定义>>

```c++
void RGBFilm::AddSplat(Point2f p, SampledSpectrum L,
                       const SampledWavelengths &lambda) {
    // <<把样本辐射量转换为PixelSensor的RGB值>> 
    // <<可选：夹拢感光器的RGB值>> 
    // <<对影响到的像素计算splat和splatBounds的边界>> 
    for (Point2i pi : splatBounds) {
        // <<在pi处应用滤波器，且添加溅射贡献量>> 
    }
}
```

### 5.4.7 GBufferFilm类

此类不但存储每个像素的RGB值，也存储了第一个可见相交点的额外几何信息。这个额外信息对于各种应用是很有用的，比如图像去噪算法，为机器学习提供训练数据等。

<<GBufferFilm的定义>>

```c++
class GBufferFilm : public FilmBase {
  public:
    // <<GBufferFilm的public方法>> 
  private:
    // <<GBufferFilm::Pixel的定义>> 
    // <<GBufferFilm的private成员>> 
};
```

除了Pixel结构体外，我们不会介绍此类的实现。Pixel被用于RGBFilm中，带了一些额外的字段，用于存储几何信息。同时也利用VarianceEstimator类，存了每个像素点的红绿蓝三色值，详见B.2.11。其他部分的实现是对RGBFilm的直接泛化，同时也更新了这些额外的值。

<<GBufferFilm::Pixel的定义>>

```c+=
struct Pixel {
    double rgbSum[3] = {0., 0., 0.};
    double weightSum = 0., gBufferWeightSum = 0.;
    AtomicDouble rgbSplat[3];
    Point3f pSum;
    Float dzdxSum = 0, dzdySum = 0;
    Normal3f nSum, nsSum;
    Point2f uvSum;
    double rgbAlbedoSum[3] = {0., 0., 0.};
    VarianceEstimator<Float> rgbVariance[3];
};
```
