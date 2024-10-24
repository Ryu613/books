# 第一章总结

## 基本概念

1. 渲染: 计算机利用代码/数学描述的3d场景，来处理并生成图片
2. pbrt: 基于物理学的渲染系统，这个渲染系统是基于光线追踪算法,可以用来渲染复杂场景下具有真实感的图片。
3. 光: 光在物理学中具有波粒二象性，本书主要模拟的是光的粒子性
4. 光线追踪: 模拟光的传播路径，主要是模拟光与其他物体相交和散射出来的这个过程

    基于光线追踪的渲染系统需要考虑的基本因素:

    - 相机: 考虑如何看到场景
    - 光与物体的相交：考虑光与物体如何相交，比如获取交点，处理物体材质，法线等
    - 光源：考虑光在整个场景中的分布情况，包括光源位置和光在空间中能量的分布情况
    - 光在物体上的散射： 考虑光在物体上的反射，折射等现象
    - 间接光的传播：考虑光打到物体上还会再次散射出来的现象
    - 介质中光线的传播：考虑光在介质中传播的现象

5. 光线定义
    $$r(t) = o + t\vec{d}$$
    o是光线原点，d是光的方向向量，t是光传播的时间($[0,+\infty)$),确定t就可以确定光与物体的交点
6. 光与物体的相交求解

    即光线函数与对应物体的函数联立后，解出t，根据t的根是否存在和存在情况来求出交点。为了提高程序计算效率，需要用加速结构进行优化。另外，求解后还需要根据物体材质做进一步处理。
7. 光的分布情况

    需要考虑光线到达交点后，会有多少光作用其上。涉及辐射度量学知识，详见第四章

8. 可见性的判定

    在某点上构造一个射线，这种特殊的射线叫做阴影射线。如果我们跟着阴影射线走，直达光源，比较这个t和光源碰撞的第一个物体的t就可以判断出这个点是否被遮挡。若没有被遮挡，这一点的着色就需要加上该光源对其的贡献。

9. 相机和胶片

    pbrt对相机和胶片进行了抽象，模拟光线到达相机和在胶片成像的过程，详见第五章
10. 光的散射

    包括反射，折射等效应，详见第四章

11. 间接光的传播

    - 光的传播具有递归性
    - 光传播方程(渲染方程)
    $$
    L_o(p, \omega_o) = L_e(p, \omega_o) + \int_{S^2}f(p, \omega_o,\omega_i)L_i(p,\omega_i)|\cos \theta_i|d\omega_i
    $$
    > $L_o(p, \omega_o)$: 从p点沿$\omega_o$方向射出的光的辐射度(亮度)，这个$\omega_o$即从交点射向相机点的方向
    >
    > $L_e(p, \omega_o)$: p点若自己是光源，要加上它自己的光辐射度
    >
    > $\int_{S^2}$: 表示从p点为球心所在的球面$S^2$上，要汇总所有入射光的光辐射
    >
    > $f(p, \omega_o,\omega_i)$: 这个就是BRDF函数，指从入射光方向$\omega_i$到出射方向$\omega_o$的光的反射辐射量是多少
    >
    > $L_i(p,\omega_i)$: 光源入射到p点的辐射度
    >
    > $\vert \cos \theta_i\vert$: 入射光与p点法线的夹角的余弦，描述点p由于入射光照射角度影响p点能接收到多少$L_i(p,\omega_i)$的量，加绝对值是说大于90°角的场景取正
    >
    > $d\omega_i$: 入射方向的微分立体角，用于对所有入射方向进行积分
12. 光在介质中的传播

    若光在类似烟雾，雾气或者尘埃里面传播，光辐射的分布就与光在真空中传播的均匀分布的情况不同。
    对于在介质中传播的光，有两种效应：
    1. 光的衰减：介质会吸收光，或者散射到其他的方向。
    2. 光的增益： 光传播过程中参与的介质，也可能会增强光，比如介质是发光体(比如火焰)，或者介质散射了其他方向的光，正好散射方向与光传播方向一致时。

## pbrt系统总览

大部分pbrt的系统实现都会包含这14种关键类型:

1. Spectrum(光谱)类：

    描述光在不同波长下的强度和能量分布，用于精确模拟光的颜色和光谱特性，如材质反射和折射

2. Camera(相机)类:

    表示渲染系统中的相机，用于定义视点、视角和图像的投影方式，控制图像的采样和最终的图像生成过程

3. Shape(几何体)类:

    表示几何体的具体形状，负责光线的交叉检测和几何属性的计算

4. Primitive (图元)类：

    表示场景中的图元,把shape和material组合在一起, 用于处理物体的几何形状和材质信息，进行光线追踪和光照计算

5. Sampler(采样器)类:

    用于在渲染过程中生成随机或准随机的样本点，决定如何对场景进行采样

6. Filter(滤波器)类：

    用于对采样后的图像进行过滤，通常是为了解决锯齿和采样噪声问题，改善最终渲染的图像质量

7. BxDF(双向分布函数相关的)类：

    表示双向反射分布函数，计算表面的光反射、散射、折射等复杂光线交互行为
8. Material（材质）类：

    表示物体表面的材质特性，用于计算光线与物体表面的相互作用，决定最终的光照效果

9. FloatTexture (浮点纹理)类:

    表示使用浮点数值的纹理，用于在材质或光照计算中使用可变的数值。为材质提供标量数据，如表面粗糙度、凹凸细节等

10. SpectrumTexture(光谱纹理):

    表示光谱分布的纹理，为材质提供与颜色或光谱相关的数据，用于精确控制光反射或折射时的光谱行为。

11. Medium(介质)类:

    表示场景中光线传播的介质，如空气、水、玻璃等，处理光线在介质中的散射、吸收和折射，模拟雾、烟、液体等复杂效果

12. Light (光源)类:

    表示场景中的光源，用于发出光线并照亮场景，定义不同类型的光源（点光源、环境光、平行光等），影响场景的照明效果

13. LightSampler(光源采样器)类:

    用于对场景中的光源进行采样，决定如何选择和使用光源进行光照计算

14. Integrator (积分器)类:

    负责执行光线追踪的核心算法，将场景的几何形状、光源、材质结合在一起计算最终的图像，控制光线传播的方式，决定如何累积和计算光线对图像的贡献

## pbrt系统执行的阶段

pbrt理论上可以分为三个阶段：

1. 场景描述处理阶段：

    把用户定义好的关于场景的描述信息(如物体几何形状，材质，光源，相机等)进行处理。

2. 场景加载阶段：

    之前步骤的场景描述处理好后，生成对应的各种物体

3. 主渲染循环阶段：

    根据第二步生成的各种物体信息，使用循环结构，来进行反复渲染，实质就是渲染公式的求解。这个阶段占了大部分运行时间和代码内容