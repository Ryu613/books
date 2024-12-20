# 简明PBR

个人总结，只写精华，尽量简洁易懂，层层递进

## PBR

传统的计算机渲染是基于经验公式建模和计算的，随着图形学的发展和用户对图像逼真度需求不断提高，为了达到此目的，其中有一种主流理念就是基于物理学现有的理论来对渲染过程进行建模和计算，最终得出更逼真的结果。以此种理念来渲染的方式，称为基于物理的渲染(PBR(Physically Based Rendering))。

## 光线追踪介绍

光线追踪是PBR中一种处理全局光照的主要的方式，其主要基于物理学中光学的理论(特别是几何光学部分)，来对光的传播和与物体的作用进行建模和计算，并模拟人眼或摄像设备的成像过程，最终得到逼真图像。

在这个思路下，产生了多种具体的实现方法，但是无论何种方法，核心的假设基本都包含了以下几个特点:

1. 能量守恒：光线与物体作用后的剩余的能量总数，总是小于作用前的能量，且发出的能量总数减去进入的能量总数，等于物体辐射出的能量总数减去物体吸收的能量总数
2. 光的粒子性假设：光本质是波粒二象性的，但是由于光的波动性效果在大部分宏观场景效果并不明显，一般情况下为了减轻建模和计算不必要的复杂性，会被忽略，只对光的粒子性中，较为重要的特性进行建模。故光只看作由光子构成，光子携带了光的能量
3. 光沿直线传播假设：相对论作用只在超大尺度时空下效果明显，为了减轻建模复杂度和计算量，假设了光线是沿直线传播的一条射线，射线的原点就是某个发出光的点的位置
4. 光的递归性：光照射到物体上发生作用后的散射过程，可看作从照射到物体上的那个点射出一条新的光线。故光传播的整个传播过程可看作递归反复的多个光线传播组合起来的过程
5. 光的线性叠加假设：实际中，光在高能级时，能量叠加并不是1+1=2。由于绝大部分场景不涉及此情况，故假设光的能量是以线性叠加方式计算的

关于光线最终被人眼或摄像设备捕获并呈现出来的过程，也有几个特点：

1. 人眼或摄像设备都是对最终落到眼睛或摄像设备镜头里的光进行捕获，并用某种方式，根据光的特性(如波长，能量，照射时间等)，对应到不同的颜色上，最终把颜色合成出图像
2. 人眼的色彩感受，主要是生物学上的三种视锥细胞(感应三原色)和视杆细胞(感应光的亮度)的生物作用形成的视觉感受。摄像设备，比如相机，是利用镜头捕获光，感应器把收集到的光，最终对应到某种颜色，并把这些颜色合并起来，最终形成图像。

由于人眼的生物学建模更为复杂，一般来说，PBR实际上一般是基于相机的光线捕获和成像过程进行建模的。

### 简单光线追踪实现

#### 相机的建模

相机模型的建模，是PBR的核心要素，关乎光的捕获和成像的过程。现实里的相机，构成的元件是比较复杂的，我们先从最简单的模型开始，然后层层递进，逐渐构造更复杂的相机模型。

##### 针孔相机

#### 光线及其传播的建模

#### 物体的建模

#### 成像的建模

### 基础光线追踪实现

#### 理论补充

##### 辐射亮度学

##### 采样

##### 蒙特卡洛积分

#### 相机的建模2

##### 镜头及快门

#### 光线及其传播的建模2

##### 光传播算法

#### 物体的建模2

##### 物体材质

#### 成像的建模2

##### 滤波器

##### 颜色空间

### 进阶光线追踪实现

#### 光源

#### 材质纹理

#### 介质

#### 算法优化

#### 并行及异步程序

#### 代码优化

### 高级光线追踪实现

#### GPU编程

#### 架构设计

#### 代码结构