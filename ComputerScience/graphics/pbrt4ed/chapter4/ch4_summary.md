# 第四章重点总结

## 光在PBR里的性质

参考了物理学->光学->几何光学的部分内容，目的是用于模拟和计算光的特性和传播过程，最终渲染逼真图像

主要假设如下:

1. 光沿直线传播，看作一条射线
2. 光只有粒子性，由光子组成，光子携带了能量
3. 光的辐射量是恒定的
4. 多个光合并看作是线性叠加
5. 遵守能量守恒，反射/折射前的光辐射量，永远比之后的大

## 什么是辐射度量学

光是一种电磁辐射，可见光指的是人眼能观察到的电磁辐射，约在[360,830]nm波长范围内。辐射度量学用于衡量光的各种辐射量及其相互作用，在PBR中，利用这些物理量来描述光的特性和计算光的辐射量，以便最终得出像素点的颜色

## 五个基本量

能量(energy), 辐射功率(flux/power), 辐射强度(intensity), 辐照度(irradiance), 辐射度(radiance)

1. 能量(Energy)

    $$
    Q=\frac{hc}{\lambda}
    $$

    - c是光速(299472458 m/s)
    - h是普朗克常数$h\approx 6.626 \times 10^{-34} m^2kg/s$
    - λ是光波的波长,这里的单位是m，但是后文为了方便描述，用nm

2. 辐射功率(flux, 或power)

    极小时间下的能量($\Phi$): 用于描述光源发光量。单位是焦耳每秒(J/s)或者瓦特(W)
3. 辐射强度(intensity)

    即极小立体角(omega:$\omega$)下的辐射功率(I), 单位是W/sr

4. 辐照度(irradiance)

    即极小面积上的辐射功率(E)，单位是$W/m^2$

5. 辐射度(radiance)

    即光源上极小面积上的极小立体角上的发光量(L), $L(p,\omega)$代表某点p在入射光立体角$\omega$下的辐射度。下标o代表出射光，i代表入射光
    $$
    L(p,\omega)=\lim_{\Delta\omega\to 0}\frac{\Delta E_\omega(p)}{\Delta\omega} = \frac{dE_\omega(p)}{d\omega}=\frac{d^2\Phi}{d\omega dA^{\bot}}
    $$
    $dA^{\bot}$表示垂直于$\omega$立体角的面,是根据朗伯定律得到的($A^{\bot}=A\cos\theta$)

## 光谱分布

光谱分布即某个关注的波长范围内，上述这些基本量的变化情况，由能量公式$Q=\frac{hc}{\lambda}$可知，变化是由λ(波长)导致的。现实物体有着不同且复杂的光谱分布，最终带来了物体不同且丰富的色彩表现

## 什么是光度学

光度学是研究人类能够通过视觉系统感知到的可见的电磁辐射的一门学科。光度学中的量，与辐射度量学的量有对应关系。人眼对不同波长的光的敏感度不同，用光谱响应曲线$V(\lambda)$表示，最终人眼感知到的光照Y可由下式表示:

$$
Y = \int_\lambda L_\lambda(\lambda)V(\lambda)d\lambda
$$

$L_\lambda(\lambda)$代表在λ波长下的光的辐射量(radiance)，需要乘以这个波长下的响应值，然后在整个波长范围内积分

## 辐射度的应用

在渲染中，计算表面某点的的辐射量就是求辐照度，一般看成是在半球上对各个方向的辐射度做积分:

$$
E(p,\vec{n})=\int_{H^2(\vec{n})}L_i(p, \omega)\vert\cos \theta\vert d\omega
$$

这里的$H^2(\vec{n})$是指Hemisphere(半球面)，n向量指与法线同向的半球，平方是指有两个自由度,$\cos$取绝对值是因为在本书中法线不一定跟入射光同侧，避免负值

## BSDF

BSDF即BRDF+BTDF,所谓BSDF,即根据物体材质，光的入射方向，出射方向给出的一个类似反射(BRDF)或透射(BTDF)的系数，用于表示入射光从某个方向$-\omega_i$,打到这个材质的物体上，在某个方向($\omega_o$)出射的光辐射量L比率是多少,$f_r$表示reflectance(即B"R"DF的函数)，$f_t$代表transmisson(即B"T"DF的函数)，二者合起来就是f,其中$f_r$定义如下:

$$
f_r(p,\omega_o,\omega_i)=\frac{dL_o(p,\omega_o)}{dE(p,\omega_i)} = \frac{dL_o(p, \omega_o)}{L_i(p, \omega_i)\cos \theta_i d\omega_i}
$$

也就是说，某个极小立体角(方向)上出射的比率，等于极小的出射L除以极小的入射E,它俩成正比

$\rho_{hd}$就是$f_r$在半球面上出射比率的总和,称为反射率，h代表半球，d代表某方向($\omega$)

$$
\rho_{hd}(\omega_o)=\int_{H^2(\vec{n})}f_r(p, \omega_o, \omega_i)|\cos \theta_i|d\omega_i \tag{4.12}
$$

同理:$\rho_{hh}$就是$f_r$从半球到另一个半球的反射率

$$
\rho_{hh} = \frac{1}{\pi}\int_{H^2(\vec{n})}\int_{H^2(\vec{n})}f_r(p, \omega_o, \omega_i)\vert\cos \theta_o \cos \theta_i\vert d\omega_o d\omega_i \tag{4.13}
$$

BTDF描述光在透射时，某方向的反射比率，注意,BRDF是在半球上，BTDF是在整个球面上，所以，若要得出BSDF最终会把入射光，出射了多少辐射量出来，就要在球面上积分, 这个式子就叫散射方程:

$$
L_o(p, \omega_o) = \int_{S^2}f(p, \omega_o, \omega_i)L_i(p, \omega_i)\vert\cos \theta_i\vert d\omega_i \tag{4.14}
$$

同理，只计算半球面的辐射量就叫做反射方程

BSSRDF描述的是半透明材质比如人的皮肤上的次表面散射的反射比率，入射点和出射点不是同一个点，就是特殊情况下的BRDF，简单了解即可

## 发光原理

发光即光辐射量里，波长在人眼能识别的可见波段里的辐射量，可见光总辐射量与总辐射量的比值叫做发光效能:

$$
\frac{\int \Phi_e(\lambda)V(\lambda)d\lambda}{\int \Phi_i(\lambda)d\lambda}
$$

这里的$\Phi_e(\lambda)$代表某波长下发光的辐射功率,$\phi_i(\lambda)$代表光源在某波长消耗的辐射量功率，积分就是求所有波长下的总辐射功率

## 黑体

物理上的一种理想物体，这种物体能把辐射量以物理最大上限的方式转换为电磁辐射量，这种物体会吸收全部的入射光，不反射光，所以很"黑"。黑体用于预测和分析实际物体在特定温度下的辐射特性

## 标准光源

定义一系列光源的光谱分布应该是什么样的,由CIE标准委员会设定，比如:标准光源A被用来表示白炽灯的光谱分布

## 光谱的建模

在pbrt里，光谱有一个接口:Spectrum类，此类提供了一组通用方法，核心方法是operator()，可根据波长lambda来取出对应的辐射量I的值。其有多个实现类，包括:

- 常数化光谱:ConstantSpectrum, 即所有波长下，辐射量都为一个常数
- 密集采样光谱:denselySampledSpectrum, 以一个std:array方式存储了某个波长范围内，以1纳米为间隔给出对应的辐射量
- 线性分段光谱: PiecewiseLinearSpectrum: 只定义某个波长下的辐射量作为点，两个顶点之间的值用插值法求得，比密集采样光谱更节省内存

## 光谱的计算

假设某一个光源照射到物体上，光源有其光谱分布，由于不同波长在物体表面的辐射量是不同的，每个波长都计算辐射量，会导致计算量过大的问题。在PBRT中，解决此问题是利用了蒙特卡洛积分估计法，即只采样某几个波长在某几个方向上的辐射度，最终估计出辐照度。把下式

$$
E=\int_\Omega\int_{\lambda_0}^{\lambda_1}L_i(p, \omega,\lambda)|\cos \theta |d\omega d\lambda
$$

利用蒙特卡洛估计法,可得:

$$
E \approx \frac{1}{n}\sum_{i=1}^n\frac{L_i(p, \omega_i,\lambda_i)|\cos \theta_i|}{p_\omega(\omega_i)p_\lambda(\lambda_i)}
$$

$L_i(p,\omega,\lambda)$就是入射光在波长$\lambda$下的辐射量

为了对波长采样，抽象出了两个类SampledSpectrum和SampledWavelength,分别代表采样出来的一组光谱量(辐射量)，和对应的一组波长和pdf(概率分布)，把辐射量和波长的定义拆开，是为了方便在计算辐射量的时候，对不同类型的光谱分布类做计算。

## 色彩的表示

