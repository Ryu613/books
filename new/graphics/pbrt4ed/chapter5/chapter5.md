# 5 相机和胶片

小孔成像忽略了透镜的聚焦效果，是全图清晰聚焦的，透镜只能部分聚焦，导致图像看起来不太真实，为了使图像更真实，需要模拟透镜的效果。

相机的透镜系统引入了各种图像畸变，比如图像边缘的晕影效果，和枕型，桶型失真等。

在pbrt中，使用Film类来表示相机捕捉到的图像，用Camera接口来代表相机

在本章中，会介绍2个Film的实现类，它们两个都使用PixelSensor类来代表特定传感器对光谱的响应效果，使其看起来像胶片拍的或是数码相机拍的吗，关于胶片和传感器的类会在本章最后一节介绍。

## 5.1 相机接口

Camera类继承自TaggedPointer

> TaggedPointer简单来讲是为了减少C++多态时虚表的内存开销(特别是复杂场景的渲染时，虚表会导致内存占用大)，并让同一个函数同时支持GPU和CPU的调用(代码在CPU和GPU运行时，函数存储在内存中不同的位置)

```c++
class Camera : public TaggedPointer<PerspectiveCamera,OrthographicCamera,
                                    SphericalCamera, RealisticCamera> {
  public:

    /*
        必须实现的方法，用于对应图像采样的光线的计算，返回的光线需要归一化
        若给定的CameraSample对象由于一些原因没有有效的光线，那么pstd::optional
        中的返回值需要被重置。

        传入进来的SampleWavelengths(样本波长)不是常量引用，故相机就可以模拟镜头
        的色散效果，在这种情况下，光线只追踪单一波长的光，并且 GenerateRay() 
        方法将调用 SampledWavelengths::TerminateSecondary()

        传入到此函数的CameraSample结构体，包含了相机光线需要的所有样本值。
    */
    PBRT_CPU_GPU inline pstd::optional<CameraRay> GenerateRay(
        CameraSample sample, SampledWavelengths &lambda) const;

    /*
        相机必须提供此方法的实现，这个方法不仅会类似GenerateRay()计算主光线，
        还会计算位于胶片沿x,y方向上移动一个像素的微分光线，
        用来代表特定相机光线的采样所对应的胶片的区域大小，
        用于计算抗锯齿时的纹理查找，提高图像质量
    */
    PBRT_CPU_GPU
    pstd::optional<CameraRayDifferential> GenerateRayDifferential(
        CameraSample sample, SampledWavelengths &lambda) const;

    /*
        相机的实现必须提供对应Film(胶片)的访问实现，用于获知诸如输出的图片的分辨率
        等信息
    */
    PBRT_CPU_GPU inline Film GetFilm() const;

    /*
        模拟现实相机的快门效果，让胶片暴露在光中一小段时间。若此值不为0，会有动态模糊效果
        相对于相机有移动的物体会被模糊
        可以根据快门开关之间的时间内的光分布情况，利用蒙特卡洛积分和点采样方法，
        可以得到动态模糊的效果

        此接口用一个在[0,1)间随机均匀分布的样本u，对应到快门的开启时间点，一般来讲
        只是用来在快门开启和关闭时间里进行线性插值，使动态模糊更真实
    */
    PBRT_CPU_GPU inline Float SampleTime(Float u) const;

    /*
        允许相机设置ImageMetadata对象的参数，比如相机的转换矩阵等，若输出的图片的格式支持存储这些额外的信息，那么这些信息会被写入到最终图像里
    */
    void InitMetadata(ImageMetadata *metadata) const;

    /*
        Camera接口的实现类必须使CameraTransform类可以用于其他坐标空间
    */
    PBRT_CPU_GPU inline const CameraTransform &GetCameraTransform() const;
}
```

### 5.1.1相机的坐标空间

除了世界空间外，还有物体空间，相机空间，相机-世界空间，和渲染空间

- 物体空间： 几何图元定义所在的坐标系统，比如，在pbrt中，球体圆心就在物体空间的坐标原点
- 世界空间： 所有物体摆放在一个世界空间里，需要把物体从物体空间坐标转换为时间空间坐标，世界空间是其他空间的标准框架。
- 相机空间： 相机被放置于世界空间的某一点，有一个观察方向和摆放的朝向，相机的位置看成坐标原点，这个坐标系的z轴对应观察方向，y轴对应相机摆放的向上的方向
- 相机-世界空间： 类似相机空间，这个坐标系的原点是相机的位置，但是保持了世界空间的方向，相机不一定沿着z轴观察
- 渲染空间： 场景根据渲染需求做了坐标系转换，在pbrt中，可以是世界空间，相机空间，或是相机-世界空间

> 相机空间中，坐标系原点在相机位置，三个轴根据相机移动和转动变化，相机-世界空间中，坐标原点在相机位置，但是三个轴方向与世界空间一致

基于光栅化的渲染中，传统上都是在相机空间中进行各种计算，三角形的顶点坐标，在投影到屏幕和光栅化前，会从物体空间中全部转换到相机空间中，方便判断哪些物体能被相机看到。

与此相对的是，许多光线追踪器(包括pbrt之前的版本)是在世界空间上渲染。当生成光线时，相机是在相机空间中实现的，但是这些相机会把那些需要求交和着色的光线，转换到世界空间中。这种方式存在一个问题，即转换过程中，离原点近的，精度高，远的精度低，若相机的位置离原点很远，这个相机看向的场景时呈现的图像就会存在误差。

> 若相机和场景离原点过远，相机空间到世界空间的转换过程的浮点数造成的误差会导致图像失真

在相机空间中渲染，对于离相机最近的物体，由于没有了相机空间到世界空间的转换过程，能原生提供最大的浮点计算精确度。但是在光线追踪中，这样做有个问题。场景一般会把主要特征沿着坐标轴建模(比如建筑物模型的地板和天花板可能就是对齐y轴的)，轴对称包围盒在这种情况下会退化得只剩一个维度，这样就减小了包围盒的表面积。

类似BVH的加速结构会在第七章介绍，在这种包围盒下影响比较大，若相机在这样的场景中旋转，轴对称包围盒的包围效果就不好，会影响渲染性能。

> 轴对称包围盒(AABB): 一个矩形框，其边与坐标轴平行，用于包围三维空间中的物体。它是最小的矩形体积，可以包含所有物体的顶点
>
> 个人理解：由于模型建模时为了方便，会把模型的主要特征，沿着坐标轴方向建模(比如高楼的高度会沿着y轴方向向上建模)。在相机空间中，由于坐标系根据相机的观察方式做了转换，模型在这个空间里可能就是“歪着的”，用轴对称包围盒这样的方式去包裹，会造成盒子空出来的空间通常比世界空间下的要大，在光线求交时，增加了大量本来不会相交的点的判断，影响了性能

使用相机-世界空间来渲染会更好，相机是在坐标原点，场景坐标也被相应转换，然而，转动相机不会影响场景的几何坐标点，因此加速结构的包围盒还是很有效。使用相机-世界空间，不会有更快的渲染速度或更精确的渲染结果。

CameraTransform类抽象了在各个空间之间的坐标系转换过程,这个类维护了两类转换，从相机空间到渲染空间的转换，和从渲染空间到世界空间的转换。在pbrt中，后者的转换不能动画化，所有动画都是用相机空间来转换，这是为了保证移动相机时，不会造成场景中静态物体也需要动态化，这会造成性能损失。

> 移动相机不会造成性能损失，但是移动物体由于会导致包围盒变大，会降低加速结构的效率，所以要避免物体不必要的移动

```c++
/*
    封装各个坐标空间之间的转换过程
    该类维护了两类转换：从相机空间到渲染空间，从渲染空间到世界空间
    Camera的实现类必须使此类支持其他系统的坐标空间
*/
/*
    传入相机到世界空间转换后的对象，根据配置里渲染基于的渲染空间做转换
    默认的渲染空间是相机-世界空间，但是也可以在命令行里面配置成其他空间
*/
CameraTransform::CameraTransform(const AnimatedTransform &worldFromCamera) {
    switch (Options->renderingSpace) {
    case RenderingCoordinateSystem::Camera: {
        // <<对于相机空间的渲染，计算worldFromRender>>

        /*
            对于相机空间的渲染，从相机到世界空间的转换过程会被worldFromRender使用
            对于从相机空间到渲染空间的变换过程，用了恒等变换(identity transformation),
            故这两个坐标系统是等价的。
            由于worldFromRender是不能被动画化，所以取了动画帧时间的中点(tMid)，然后
            把这个点在相机变换中的动画，并入renderFromCamera
        */
        Float tMid = (worldFromCamera.startTime + worldFromCamera.endTime) / 2;
        worldFromRender = worldFromCamera.Interpolate(tMid);
        break;
    }
    case RenderingCoordinateSystem::CameraWorld: {
        // <<对于相机-世界空间的渲染，计算worldFromRender>>
        /*
            对于相机-世界空间上的渲染(默认)，渲染空间到世界空间的坐标系变换时基于动画帧
            的中点来转换到相机的位置
        */
        Float tMid = (worldFromCamera.startTime + worldFromCamera.endTime) / 2;
        Point3f pCamera = worldFromCamera(Point3f(0, 0, 0), tMid);
        worldFromRender = Translate(Vector3f(pCamera));
        break;
    }
    case RenderingCoordinateSystem::World: {
        // <<对于世界空间的渲染，计算worldFromRender>>
        /*
            对于世界空间的渲染，就是做恒等变换
        */
        worldFromRender = Transform();
        break;
    }
    default:
        LOG_FATAL("Unhandled rendering coordinate space");
    }
    LOG_VERBOSE("World-space position: %s", worldFromRender(Point3f(0, 0, 0)));
    // <<计算renderFromCamera>>
    /*
        一旦worldFromRender设置完后，worldFromCamera剩余的变换过程会在这里处理，
        存入到renderFromCamera
    */
    Transform renderFromWorld = Inverse(worldFromRender);
    Transform rfc[2] = {renderFromWorld * worldFromCamera.startTransform,
                        renderFromWorld * worldFromCamera.endTransform};
    renderFromCamera = AnimatedTransform(rfc[0], worldFromCamera.startTime, rfc[1],
                                         worldFromCamera.endTime);
}
```



### 5.1.2 CameraBase类

Camera接口的通用函数放到CameraBase中，其他相机类皆继承此类。关于camera的实现的相机类都放在cameras.h下面

## 5.2 投影相机的模型

### 5.2.1 正交投影相机

### 5.2.2 透视投影相机

### 5.2.3 薄透镜模型和景深

## 5.3 球形相机

## 5.4 胶片和成像

### 5.4.1 相机计算公式

### 5.4.2 传感器响应的建模

### 5.4.3 图像采样的过滤

### 5.4.4 胶片接口

### 5.4.5 胶片的通用功能

### 5.4.6 RGBFilm类

### 5.4.7 GBufferFilm类
