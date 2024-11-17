# 13 光的传播I: 表面反射

本章把光追理论，辐射度量学理论，蒙特卡洛采样算法结合起来，来实现两种不同的积分器，用来计算场景中物体表面散射的辐射量。这些积分器比第一章所说的RandomWalkIntegerator更有效率，用这两种积分器可以使错误数降低几百倍。

我们会以光传播等式的派生来开始，这个方程是在1.2.6章节介绍的。然后我们能正式介绍路径追踪算法，此算法应用了蒙特卡洛积分来计算这个等式。然后，我们会对SimplePathIntegrator的实现做描述，它是路径追踪的精简版实现，对于理解基本算法和调试样本算法是很有用的。然后本章会用PathIntegrator做概括总结，它是路径追踪更完整的实现。

这两个积分器会找到从相机开始的光的传输路径，并且考虑到了这些形状表面的散射效应。第14章会扩展路径追踪，使其包含介质传播的效果(本书在线版还包括了一章，用来描述双向路径追踪)

## 13.1 光传播方程(LTE)

光传播方程(LTE)描述了场景中辐射量分布的平衡性，这个方程给出了在表面上某点的总反射辐射量，这个辐射量是根据表面发光，BSDF和入射光照分布到达此点的量得出的。在此我们不考虑光在介质中的传播场景，这部分会在14章介绍

让LTE的计算困难的原因是，某个点接收到的辐射量是受场景中所有物体的几何和散射特性所影响。比如，某个很亮的光照射到一个红色物体上，可能会导致红色的光映到旁边的物体上，或者玻璃可能会在桌面上形成焦散效果。处理这种光照复杂性的渲染算法称为全局光照算法, 为了与局部光照算法区分开，局部光照算法在着色计算中只使用与某一个表面属性相关的信息。

> 全局光照算法考虑场景光在所有物体相互作用的效果，局部光照算法只考虑单独的物体的着色，不会受其他物体影响

### 13.1.1 LTE的基本推导

我们已经选择基于用辐射度量学来描述光，故我们的光传输方程把这个条件作为基本假设，那么光的波动性就不重要了，故场景中的辐射分布就是均匀的。

光传输方程(LTE)的重要定律就是能量守恒，能量的改变必须在某种过程中被"归因"，同时，我们必须记录所有能量的变化。因为我们假设了光照是直线的过程，从某个系统中出来和进入的能量的差值，必须和发射出和吸收的能量的差值相等。这种想法在多个空间尺度下都相通。在宏观尺度下，我们有能量守恒定律：

$$
\Phi_o - \Phi_i = \Phi_e - \Phi_a
$$

即，离开物体的辐射功率$\Phi_o$减去进入物体的辐射功率$\Phi_i$，与物体辐射出的辐射功率$\Phi_e$减去吸收的辐射功率$\Phi_a$的差，是相等的

为了使某个平面上的的能量守恒，离开平面的辐射度$L_o$必须等于辐射出的辐射度加上入射光散射后的辐射量。辐射出的辐射量用$L_e$给出，散射后的辐射量是用散射方程给出，即下式:

> $L_o$等于这个点自己辐射出的辐射量加上照到这个点的光散射后的辐射量

$$
 L_o(p, \omega_o) = L_e(p, \omega_o) + \int_{S^2}f(p, \omega_o,\omega_i)L_i(p,\omega_i)|\cos \theta_i|d\omega_i
$$

> 这里的p点是场景中任意一点，这个式子就是说场景中任意一个p点(物体表面的点只是其中一种情况)，如何求得它的光辐射量

![图13.1](img/fg13_1.png)

图13.1 在自由空间中，沿着光线方向的辐射量是不变的。因此，为了计算从p点在$\omega$方向的入射光辐射量，我们能找到这条射线的第一个与物体表面的交点，然后计算$-\omega$方向的出射光辐射量。光线投射函数$t(p,\omega)$给定了射线$(p,\omega)$在第一个表面的交点p'

由于我们已经假设光传播过程中没有介质参与，辐射量就是一个不会变化的数。因此，我们能把p点的入射光辐射量与p'的出射光辐射量关联起来，如上图13.1。 若我们定义了一个光线投射函数$t(p, \omega)$，用来计算从p射到第一个碰到的表面一点p'，方向为$\omega$,我们就能通过p'的出射辐射量，来写出p点的入射光辐射量

> 即，由能量守恒假设，可以用p'在$-\omega$上的的出射辐射量来反推p在$\omega$方向上的入射辐射量

$$
L_i(p, \omega) = L_o(t(p,\omega), -\omega)
$$

由于场景并不是封闭的，我们会让光线投射函数在光$(p, \omega)$没有与场景物体相交的时候，返回一个特殊值$\Lambda$，使得$L_o(\Lambda, \omega)$总为0

为了简洁，把$L_o$的下标舍弃，那么LTE就可写成:

$$
 L(p, \omega_o) = L_e(p, \omega_o) + \int_{S^2}f(p, \omega_o,\omega_i)L(t(p, \omega), -\omega_i)|\cos \theta_i|d\omega_i
$$

上式中的关键是，只有一个我们感兴趣的量，即表面上某点的出射辐射量。当然这个量在上式中的两侧都存在，所以我们的任务不简单，但是肯定更简单点了。记住，我们可以通过强制利用能量守恒来让这个式子更简单，这是很重要的。

> 就是说，在原版LTE中，有两个量需要关注，$L_i$和$L_e$，但是通过利用能量守恒定律，把$L_i$的部分替换为交点处反向的出射辐射量，改成了上式以后，只有一个$L_e$需要关注，也就降低了LTE的计算难度

### 13.1.2 LTE的解析解

LTE式子的简洁性掩盖了一个事实，除非是非常简单的场景，否则得到这个式子的解析解是不可能的。式子的复杂性来源于基于物理的BSDF模型，还有任意场景的几何特征，还有物体间错综复杂的可见关系，合起来的这一切，都需要一种数值上的解决方法。幸运的是，把光线追踪算法结合蒙特卡洛积分，就给出了一个强力的组合工具，能够在不需要在各种LTE分量中设置限制条件（比如： 要求所有BSDF都是朗伯材质，或者大幅限制物体的几何形状）的场景下，处理这种复杂性。

对于求解LTE，只需要简单设置一下即可。虽然对于更通用性质的渲染没有太大帮助，但是这种方法能帮助调试积分器。若某个积分器想求解完整的LTE时，不能计算出对应的解析解，那么很显然积分器就有问题。例如：考虑一个球面内部，都是朗伯型的BRDF,$f(p, \omega_o, \omega_i) = c$, 同时在所有方向上也发出常数性质的辐射量。那么我们就有：

$$
 L(p, \omega_o) = L_e + c\int_{H^2(\vec{n})}L(t(p, \omega), -\omega_i)|\cos \theta_i|d\omega_i
$$

在球面内部任意一点的出射辐射度的分布，与其他的点都相同。这个场景下，不同的点之间不会引入变化量。因此入射光辐射分布在所有点都是相等的，并且以余弦加权的入射光辐射量也必须在所有地方都相等。故，我们可以把辐射量函数替换为一个常数，那么LTE就可以写作下式:

$$
L = L_e + c\pi L
$$

> $L\cos\theta$处处相等，看作一个固定值L, 单位半球面积分求得$2\pi$,把常数项合并，即可得此式

尽管我们可以直接求得方程中的L，但有趣的是，可以考虑把右边的L项递归地替换回它自己，若我们把$\pi c$替换为$\rho_{hh}$, 表面的朗伯反射就有:

$$
L = L_e + \rho_{hh}(L_e + \rho_{hh}(L_e + ...\\
= \sum_{i=0}^{\infty}L_e\rho_{hh}^i
$$

> 替换即基于之前说的入射光辐射量等于某点出射光辐射量
>
> $\rho_{hh}$是半球面到半球面的反射率，为BSDF的一种简化形式

换句话说，出射辐射量等于此点发出的辐射量加上同时由BSDF散射的辐射量，再加上第二次被散射的辐射量，以此类推。

根据能量守恒定律，$\rho_{hh} < 1$， 故这个级数会收敛，并且在所有方向和所有点上，反射后的辐射量是:

$$
L=\frac{L_e}{1-\rho_{hh}}
$$

(这个级数也叫Neumann级数)

> 这里是通过把$\sum_{i=0}^{\infty}L_e\rho_{hh}^i$用无穷级数求解得到的
>
> 由于$\rho_{hh} < 1$，即光线会在多次反射后逐渐趋于0，即收敛

> **为何文中不直接求得$L=\frac{L_e}{1-\rho_{hh}}$,而是要用递归提代L后算无穷级数，这不是绕了个弯吗?**
>
> 主要是为了解释物理现象，帮助我们理解光的多次反射和累积效应，也表明了由于能量守恒，反射率$\rho_{hh} < 1$故求得的数不会越来越大，而是会收敛

对于把LTE中右侧的入射光辐射量，进行反复代入的过程，对于把LTE推导出更普遍的场景是具有指导意义的。例如，在下式中，仅仅考虑了直接光照，实际上是通过一次提代计算得到的结果:

$$
 L(p, \omega_o) = L_e(p, \omega_o) + \int_{S^2}L_d(p, -\omega_i)|\cos \theta_i|d\omega_i
$$

其中: $L_d(p, \omega_i) = L_e(t(p, \omega_i), -\omega_i)$

后面的散射过程被忽略了。

在后面的章节中，我们会看到如何在这个式子中进行递归提代，然后重新整理LTE的表达式，把其变为更自然的方式，用于渲染算法的开发。

### 13.1.3 在表面形式的LTE

在之前LTE式子比较复杂的原因是在几何物体之间的关系是隐含在光追函数$t(p, \omega)$中。将被积函数中的式子写明白，有助于阐明这个方程的结构。为了做到这点，我们会把之前的LTE方程重写成对于面积的积分而不是对于球面方向上的积分

首先，我们定义从p'点到p的出射辐射度:

$$
L(p' \rightarrow p) = L(p',\omega)
$$

若p'和p是相互可见的，并且$\omega=\widehat{p - p'}$。我们也能把在p'点的BSDF写成:

$$
f(p'' \rightarrow p' \rightarrow p) = f(p', \omega_o, \omega_i)
$$

这里$\omega_i = \widehat{p'' - p'}$，$\omega_o=\widehat{p - p'}$。这个式子有时候也被称为BSDF的三点形式。如图13.2

![图13.2](img/fg13_2.png)

图13.2 LTE的三点形式，把对球面方向上的积分变为对面上的点所在的域上的积分。这种变换对于推导LTE在路径上的积分形式来说是很重要的

> 这里$\omega=\widehat{p - p'}$，上面的这个折线帽子指的是单位向量

然而，用这种方式写出的LTE还不太够。为了把LTE从对方向的积分转为对面上的积分，我们也需要乘上立体角的雅可比式子，这个式子是$\frac{|\cos \theta'|}{r^2}$

把这个变化后的式子结合起来，原来的$\vert \cos \theta\vert$是来自LTE，双向的可见性函数V(V=1时两点互相可见，否则V=0)，这个单一的几何耦合项函数$G(p \leftrightarrow p')$为:

$$
G(p \leftrightarrow p') = V(p \leftrightarrow p')\frac{|\cos \theta||\cos \theta'|}{||p-p'||^2}
$$

> 这个式子描述了p到p'的变化关系，若两点互相可见，则把LTE里的余弦量带过来，在乘以雅可比量(用于把方向积分转为面积分)，定义这个G是用于替换掉LTE里$\vert \cos \theta|d\omega$的部分

把这些式子代入到LTE，然后把其转换为面上的积分，我们可以得到LTE的三点形式方程:

$$
L(p' \rightarrow p) = L_e(p' \rightarrow p) + \int_A f(p'' \rightarrow p' \rightarrow p)L(p'' \rightarrow p')G(p'' \leftrightarrow p')dA(p'')
$$

这里的A是场景中所有的表面

虽然LTE原始式子和上式是等效的，但是二者使用了不同的表达方法。为了用蒙特卡洛方法计算LTE的原始式子，我们会在球面上根据方向的分布来采样，然后投射出光线来估计这个积分。然而，对于上式，我们会在表面的区域的分布里采样点，然后计算这些点的耦合关系函数，用于计算这个被积函数，同时对光线进行追踪来计算V的可见性

> 也就是说，LTE原式和上式的采样方法是不一样的。

### 13.1.4 在路径上积分

有了之前形式的LTE，我们就能推导更灵活形式的LTE，被称为光线传播的路径积分式，用来表示辐射量在传播路径上的积分，在这个路径上的点是在一个高维度的路径空间中。用路径空间的其中一个主要动机是，它为要测量的值提供了一个明确的关于路径的积分，而不是根据能量守恒方程推出的繁琐的递归形式方程。

这种显式的式子让考虑路径如何被找到变得自由，特别是对于在可实践的渲染算法中，是用任何随机选取路径的技术，可以在给定足够样本数的情况下，计算出正确的答案。LTE的这种形式提供了双向光传输算法的基础。

把对面积的积分变为根据不同光波长的路径的路径积分的总和，我们可以从把LTE的三点形式做扩展开始，把式子中右侧的积分式做递归替换成$L(p'' \rightarrow p')$形式。下式中是从p1到p0时，给定入射光辐射量情况下的的一部分式子，p1是沿着光路径p1-p0中的表面上的第一个点:

$$
L(p_1 \rightarrow p_0) = L_e(p_1 \rightarrow p_0)\\
+ \int_A L_e(p_2 \rightarrow p_1)f(p_2 \rightarrow p_1 \rightarrow p_0)G(p_2 \leftrightarrow p_1)dA(p_2)\\
+ \int_A \int_A L_e(p_3 \rightarrow p_2)f(p_3 \rightarrow p_2 \rightarrow p_1)G(p_3 \leftrightarrow p_2)\\
 \times f(p_2 \rightarrow p_1 \rightarrow p_0)G(p_2 \leftrightarrow p_1)dA(p_3)dA(p_2) + \cdots
$$

> 上式就是LTE的三点形式下的递归代入式子，重点在于对LTE积分式里的入射光辐射度L用下一个交点的LTE代入，递归下去就是此形式

式子右侧中的每一项代表了路径长度增加时的光辐射度。例如，第三项就如图13.3,这条路径中，有4个顶点，由三条线段相连。所有路径下的辐射量由这四条路径给出(即相机点，2个面上的交点，光源的点)。其中，p0和p1是根据相机所在的原点和光线可以预先得出，但是p2和p3可能会在场景中的面上的不同位置变化，在所有的p2和p3点上做积分，就给出了到达相机点上，总的路径上的光辐射贡献量。

![图13.3](img/fg13_3.png)

图13.3 对于从LTE中给定的场景中面上的所有p2,p3点，光在经过两次弹射后，从p1离开且沿着p0方向，就可得p0接收到的总辐射量。被积函数中乘积的分量就在此处表示，$L_e$表示光发出的辐射量，两点之间的几何关系式G, 和多个BSDF代表的散射效果f

把这个无穷累加的式子合并起来，可得:

$$
L(p_1 \rightarrow p_0) = \sum_{n=1}^{\infty}P(\bar{p_n})
$$

$P(\bar{p_n})$给出了从路径$\bar{p_n}$射出的辐射量，它经过了n+1个顶点

$$
\bar{p_n}=p_0,p_1,\cdots,p_n
$$

此处，$p_0$就在胶片平面或前镜片元件上，$p_n$就在光源上，且:

$$
P(\bar{p_n}) = \underbrace{\int_A \int_A \cdots \int_A}_{n-1}L_e(p_n \rightarrow p_{n-1}) \\
\times \left(\prod_{i=1}^{n-1}f(p_{i+1} \rightarrow p_i \rightarrow p_{i-1})G(p_{i+1} \leftrightarrow p_i)\right) dA(p_2)\cdots dA(p_n)
$$

继续下文之前，我们会定义一个额外的式子，帮助后文中的讨论，一个路径上的BSDF和几何项的乘积叫做吞吐量，用于描述从光源开始，经过所有的散射，最终打到相机上的光辐射的分式。如下式:

$$
T(\bar{p_n}) = \prod_{i=1}^{n-1}f(p_{i+1} \rightarrow p_i \rightarrow p_{i-1})G(p_{i+1} \leftrightarrow p_i)
$$

故有:

$$
P(\bar{p_n}) = \underbrace{\int_A \int_A \cdots \int_A}_{n-1}L_e(p_n \rightarrow p_{n-1})T(\bar{p_n})dA(p_2)\cdots dA(p_n)
$$

给定一个特定路径长度n, 要计算路径n上，到达p0的蒙特卡洛估计值，就是用合适的采样密度来采样一组顶点， $\bar{P_n} \sim p$, 来生成一条路径，然后利用这些顶点，使用$P(\bar{P_n})$来计算估计值:

$$
L(p_1 \rightarrow p_0) \approx \sum_{n=1}^{\infty}\frac{P(\bar{p_n})}{p(\bar{p_n})}
$$

> 这个式子就是说，要求得从p1点到p0点的辐射量，就是把所有可能到p0的路径上的辐射量累加起来，这里有个分母，是用来平衡选取的那条路径的概率，相当于让低概率的路径加大影响(因为难以取到)，高概率的路径减小影响(因为容易取到)。
>
> 理论上来说，对p点采样的路径的辐射量累加，跟理想上的LTE积分肯定不是相等的，这里用约等号就是这个意思，即蒙特卡洛积分的核心思想，采样越多，越接近积分真实值。

不论是从相机开始生成这些路径上的点，还是从光源，或者从两头同时生成，亦或是从中间开始生成，这些方式都只会影响到该路径的采样概率密度$p(\bar{P_n})$。我们会在接下来的章节中看到这个式子如何推导为可实践的光传输算法。

### 13.1.5 被积函数中的delta分布

delta函数可能在$P(\bar{p_i})$项中表现，不仅由于光源的类型(比如：点光源和双向光源)，也由于BSDF分量是被delta分布所描述的。如果delta寒素出现，这些分布就需要在光线传输算法中被明确地处理。比如，从表面上某点随机选择一个会与光源相交的出射方向是不可能的。而是，当需要把某个光源作为光贡献者的时候，要显式地选择一条路径方向，从某点射到光源(带有delta分量的BSDF的采样也是同理)。然而处理这种场景在积分器中引入了一些额外的复杂性，但是一般来说这种做法还是受欢迎的，因为减少了积分式中需要计算的维度，把其中的一些项转为了直接累加的式子。

比如，考虑直接光照项,$P(\bar{p_2})$, 在场景中一点$P_{light}$上有点光源，可以被一个delta分布所描述:

$$
P(\bar{p_2}) = \int_A L_e(p_2 \rightarrow p_1)f(p_2 \rightarrow p_1 \rightarrow p_0)G(p_2 \leftrightarrow p_1)dA(p_2)\\
= \frac{\delta(p_{light} - p_2)L_e(p_{light} \rightarrow p_1)}{p(P_{light})}f(p_2 \rightarrow p_1 \rightarrow p_0)G(p_2 \leftrightarrow p_1)
$$

换句话说，$p_2$必须是在场景中的光源的位置。由于$p(P_{light})$点明显的delta分布性，这个式子的每项积分可以被化掉(回忆在12章的采样狄拉克delta分布)，然后我们留下了可以被直接计算的一项，不需要蒙特卡洛法。一个类似的场景是在经过$T(\bar{p_n})$的delta分布的BSDF，估计值计算时，可以消除积分式中的每一个对于面上的积分。

### 13.1.6 拆分被积函数

许多已有的渲染算法，在某些情况下，解LTE方程效果不错，但是对于另一些情况就不太好，比如Whitted原始的光追算法只处理从BSDF的delta分布中的镜面反射，忽略各种从漫反射来的散射光，和抛光材质的BSDF

因为我们想要推导出正确的光传播算法，能够支持所有可能的散射方式，而且不能忽略任何场景，并且也不能计算两次，那么关注LTE是哪一种场景的结局方法是很重要的。解决此问题一种比较好的方法就是把LTE按多种方式拆分。比如，我们可能想在路径上扩展一个累加项到如下式子:

$$
L(p_1 \rightarrow p_0) = P(\bar{p_1}) + P(\bar{p_2}) + \sum_{i=3}^\infty P(\bar{p_i})
$$

计算p1点发出的光辐射量这一项就变得微不足道，第二项是用精确的直接光照方法计算的，但是剩余的项是一个累加式，使用一种更快但是不太精确的方法计算。若这些额外的式子算来的贡献量，对于总的反射辐射量相对较小，那么这种方式就是合理的。唯一一个细节式，省略$P(\bar{p_1})$和$P(\bar{p_2})$要多小心，尤其是算法会处理$P(\bar{p_3})$或者更多的项的时候。

同样的，分割成独立的$P(\bar{p_n})$项也是很有用的。比如，我们可能想把每个光线发出的量的项分成多个小光源发出的光的量。$L_{e,s}$和从大的光源拆分的量$L_{e,l}$,可得下式:

$$
P(\bar{p_n}) = \int_{A^{n-1}}(L_{e,s}(p_n \rightarrow p_{n-1}) + L_{e,l}(p_n \rightarrow p_{n-1}))T(\bar{p_n})dA(p_2)\cdots dA(p_n)\\
= \int_{A^{n-1}}L_{e,s}(p_n \rightarrow p_{n-1})T(\bar{p_n})dA(p_2)\cdots dA(p_n)\\
= \int_{A^{n-1}}L_{e,1}(p_n \rightarrow p_{n-1})T(\bar{p_n})dA(p_2)\cdots dA(p_n)
$$

这两部分积分可以被独立开来计算，有可能两部分用的是完全不同的算法，或者不同数量的样本，或者根据不同场景选择处理方式等。也包括$L_{e,s}$的估计，忽略了任何从巨大的光源的发光亮，对$L_{e,l}$的估计忽略了小光源的发光量，并且所有光都被大或小进行了分类。最终计算出正确的结果。

最后，BSDF项也可以被拆分(事实上，就如9.1.2所述，这就是为什么用BxDFFlags来区分BSDF的原因)。例如：若$f_{\Delta}$代表了用delta分布描述的BSDF分量，并且$f_{\lnot\Delta}$代表剩余的分量

$$
P(\bar{p_n}) = \int_{A^{n-1}}L_e(p_n \rightarrow p_{n-1})\\
\times \prod_{i=1}^{n-1}(f_{\Delta}(p_{i+1} \rightarrow p_i \rightarrow p_{i-1}) + f_{\lnot\Delta}(p_{i+1} \rightarrow p_i \rightarrow p_{i-1})) \times G(p_{i+1} \leftrightarrow p_i)dA(p_2)\cdots dA(p_n)
$$

注意，乘式中有n-1项BSDF，小心不要只带$f_{\Delta}$或只带$f_{\lnot\Delta}$。所有混合的项，若分项都用到，比如$f_{\Delta}f_{\lnot\Delta}$就必须都计算上

## 13.2 路径追踪

我们已从LTE推导出了路径积分式，现在我们将用路径追踪积分器，来展示路径追踪光传输算法的推导该如何使用。

路径追踪曾是图形学中首个通用的无偏蒙特卡洛光传输算法。kajiya在描述LTE的同篇论文中引入了此算法。路径追踪以渐进的方式生成从相机到光源的散射路径。

虽然从基础LTE直接推导路径追踪更容易，但是我们会从路径积分的形式来达到此目的，这种做法能帮助我们理解路径积分方程和双向路径采样算法。

### 13.2.1 总览

给出LTE的路径积分形式，我们就能估计相机光线的交点$p_1$的出射辐射量:

$$
L(p_1 \rightarrow p_0) = \sum_{i=1}^\infty P(\bar{p}_i)
$$

对于给定的$p_0$点出发的相机光线，首个交点是$p_1$,为了计算这个估计值，有两个问题必须要解决:

1. 如何估计无限数量的$P(\bar{p}_i)$的总和？
2. 给定特定一项$P(\bar{p}_i)$,为了计算这个多维积分的蒙特卡洛估计值，如何生成一条或多条路径$\bar{p}$?

对于路径追踪，我们可以借鉴现实中可行的一个场景。总的来说，除开特定场景，经过更多顶点的光线路径最终散射出更少的光，反之则相对较多。这是由于BSDF下的能量守恒带来的自然结果。因此，我们总是估计前几项$P(\bar{p}_i)$,然后，在有限项之后，采用俄罗斯轮盘赌法，来停止采样，且不引入估计偏差(回顾章节2.2.4，此章节展示了如何使用俄罗斯轮盘赌法在累加过程中按概率停止计算，并且适当地为没跳过的项重新加权)。比如，若我们总是计算$P(\bar{p}_1)$,$P\bar{p}_2$，和$P(\bar{p}_3)$的估计值，但是以概率q来不再计算之后的项，那么一个累加的无偏估计就如下式:

$$
P(\bar{p}_1)+P(\bar{p}_2)+P(\bar{p}_3)+\frac{1}{1-q}\sum_{i=4}^\infty P(\bar{p}_i)
$$

如上文所述的俄罗斯轮盘赌的使用方法，不能解决计算无限项累加的问题，但是进步了一步。

若我们根据此想法更进一步，并且在每一项都可能以q的概率随机结束计算，那么有下式:

$$
\frac{1}{1-q_1}(P(\bar{p}_1)+\frac{1}{1-q_2}(P(\bar{p}_2)+\frac{1}{1-q_3}(P(\bar{p}_3)+...
$$

我们最终会停止计算这个累加式子。然而，由于对于特定的i，都有大于0的概率来计算$P(\bar{p}_i)$,并且，由于我们若确实计算了此值，那么就会适当加权，最终得到的结果就是此累加式的无偏估计

### 13.2.2 路径的采样

给出了无线累加式以有限项求解的方法后，我们也需要一种方法，取估计某个特定项$P(\bar{p}_i)$的贡献值。我们需要i+1个顶点来定义这条路径，其中最后一个顶点$p_i$在光源上，并且第一个顶点$p_0$是在相机胶片(或镜片)上。观察$P(\bar{p}_i)$项，是一个在表面上的多个积分式，最自然的方式是在场景中某个物体的表面采样顶点$p_i$,那么场景中表面上的所有点就有了相同的概率(在积分器实现中，我们不会真的使用这种方式，原因之后会说明，但是此方法可能用来改进我们的基本实现的效率，并且帮助澄清LTE路径积分的意义)

运用这种采样方法，我们可以定义一个在n个物体上离散的概率，若每个物体有一个表面$A_i$,那么第i个物体的某条路径的被采样的顶点的概率就是:

$$
P_i=\frac{A_i}{\sum_j A_j}
$$

> 第i个物体的面的面积比上总的物体面积，即为采样到这个物体上的点的概率

那么，给定一个方法来在第i个物体上均匀采样，那么第i个物体上采样特定点的PDF为$\frac{1}{A_i}$,因此，采样某点的总体概率密度是:

$$
\frac{A_i}{\sum_j A_j}\frac{1}{A_i}
$$

那么所有样本点$p_i$就有了相同的PDF值:

$$
p_A(p_i) = \frac{1}{\sum_j A_j}
$$

令人安心的是，这些权重的全都是相同的，因此，我们在场景中的所有表面上采样点都有相同的概率

按这样的方式采样的一组顶点$p_0,p_1,\cdots p_{i-1}$, 那么最后一个点$p_i$就在光源上，此点的PDF也相同。虽然我们能使用与采样路径顶点相同的方式来采样光源上的点，但是这样经常会导致较高的方差，因为对于所有不在光源表面上的$p_i$路径，路径的贡献就会为0。此积分的期望值还是正确的，但是收敛会变得极其慢。更好的方式是，只在发光物体的表面上采样，并且这些表面的概率已经被相应更新。给定一个完整的路径，我们就有足够的信息来计算$P(\bar{p}_i)$的估计值，此计算只需要计算每一项的值即可。

对于如何利用此通用方法来设置采样概率，很容易灵活调整，比如，若我们知道了从某些少数物体贡献了大部分间接光照，我们就能设置一个较高的概率来生成路径上的顶点$p_i$,这些顶点就在那些物体上，并适当地更新样本的权重

然而，这样的采样方法会带来2个互相关联的问题。第一个问题是，此方法会导致高方差，第二个问题是，会导致不正确的结果。第一个问题是当有两个相邻顶点互不可见时，许多路径会没有贡献量。考虑在一个复杂建筑模型中应用此面积采样方法，在这种路径下的相邻顶点之间会几乎总是有一两堵墙，这会导致不对路径做出贡献，并且为估计带来高方差

第二个问题是，若被积函数中有delta函数(比如一个点光源或者完美镜面的BSDF),此种采样方法永远不能选取到使得delta分布为非零的路径上的顶点。甚至当没有delta分布时，由于BSDF变得越来越光亮，几乎所有路径会有很低的贡献值，因为在$f(p_{i+1}\rightarrow p_i \rightarrow p_{i-1})$上的点会导致BSDF有一个很小或0的值，同样会导致高的方差值

### 13.2.3 渐增路径构建

解决这两个问题的方法是，从相机点$p_0$开始，渐增地构造路径。在每个顶点的BSDF被采样出来，以生成一个新的方向，下一个顶点$p_{i+1}$是从$p_i$开始，根据在采样的方向上找到的最近的交点。我们通过在局部贡献比较重要的方向做一系列选择，有效地试出一条具有总体贡献较大的路径。虽然此方法有时会无效，但是通常都是一种好的策略。

由于此种利用BSDF采样来构造路径的方法是基于立体角的，并且路径的LTE是一个在面上的积分，我们需要做出修正，来把基于立体角$p_w$的概率密度转换为基于面积$p_A$的概率密度。若$\omega_{i-1}$是$p_{i-1}$归一化的向量，那么:

$$
p_A(\mathsf{p_i})= p_\omega(\omega_{i-1})\frac{\vert\cos\theta_i\vert}{\Vert \mathsf{p_{i-1}-p_i}\Vert ^2}
$$

这种修正会导致所有对应的几何函数$G(P_{i+1}\leftrightarrow P_i)$因子抵消了除$\cos\theta_{i+1}$外的$P(\bar{p}_i)$项。此外，由于已经追踪了一条光线到$p_i$, 就已知$p_{i-1}$和$p_i$是必须互相可见的，那么可见项就总是为1。换个角度来考虑，就是光线追踪提供了在G的可见分量上进行重要性采样的一种操作。

利用此路径追踪方式，路径中最后的那个在光源面上的顶点，就会有特殊对待。比起被渐增地采样，光源是从面上按照某种分布来采样的。(用这种方式采样最终点一般被称为下一事件估计NEE,这是根据一个具有相同名称的蒙特卡罗方法命名的)。现在，我们会假设在发光体上有一个采样分布$p_e$,在13.4章节中，我们会看到一个利用了多重重要性采样，更具效率的估计器能够被构建出来。

通过此种方法，某条路径上的蒙特卡洛估计值就是:

$$
P(\bar{p}_i)\approx \frac{L_e(\mathsf{p_i} \rightarrow \mathsf{p_{i-1}})f(\mathsf{p_i}\rightarrow \mathsf{p_{i-1}} \rightarrow \mathsf{p_{i-2}})G(\mathsf{p_i} \leftrightarrow \mathsf{p_{i-1}})}{p_e(\mathsf{p_i})}\\
\times \bigg(\prod_{j=1}^{i-2}\frac{f(\mathsf{p_{j+1}} \rightarrow \mathsf{p_{j}} \rightarrow \mathsf{p_{j-1}})\vert\cos\theta_j\vert}{p_\omega(\omega_j)}\bigg)
$$

由于此采样式，在构建长度为i的路径时，重复使用了i-1个顶点(发光体上的顶点除外)，确实在多项$P(\bar{\mathsf{p_i}})$引入了关联性，然而，这不会影响蒙特卡洛估计式的无偏性。在实际操作中，这种相关性带来的影响会被更高的效率所抵消，因为相比于使 $P(\bar{\mathsf{p_i}})$ 项独立所需的光线数量，这种方法追踪的光线更少

#### 与随机行走积分器的关系

## 13.3 一个简单的路径追踪器

在方程13.7中的路径追踪估计器让应用BSDF(第9章)和光线采样技术(第12章)成为可能。如图13.6，比在RandomWalkIntegrator中的均匀采样更有效的重要性采样方法能显著减少误差。虽然SimplePathIntegrator在相同采样数时耗时更长，耗时长是由于在RandomWalkIntegrator中路径追踪结束的更早。由于此积分器是在交点处的球面上以均匀的出射方向采样，一般的采样方向会导致路径在非透明表面结束。从SimplePathIntegrator的蒙特卡洛总体效率是其12.8倍

PathIntegrator名字里的"simple"是有深意的，简单来说，此积分器添加了一些额外的采样优化，并且可在渲染效率更重要的场景下优先使用，故此积分器不单是用于教育示例，同时，此积分器对于调试和验证采样算法的实现也很有用。比如：此积分器能配置以使用BSDF的采样方法，或使用方向上均匀的采样，给出足够数量的样本，两种方法都应收敛于相同的结果(假设BSDF不是完美镜面)。若它们结果不一致，那么误差很可能是在BSDF采样代码中。光线的采样方法也可以用相同的方式验证。

```c++
<<SimplePathIntegrator Definition>>= 
class SimplePathIntegrator : public RayIntegrator {
  public:
    <<SimplePathIntegrator Public Methods>> 
  private:
    <<SimplePathIntegrator Private Members>> 
};
```

构造器根据提供的参数设置了下列的成员变量，故不在此赘述，与RandomWalkIntegrator相似，maxDepth限制了最大路径长度。

sampleLights决定了光源的SampleLi()方法是否应该使用来采样直接光照，或者就如RandomWalkIntegrator那样，只在光线随机与发光体表面相交时采样光照。同理，sampleBSDF决定了BSDF的Sample_f()方法是否应该用来采样方向，或采样均匀的方向。它们默认都是true的。对于采样光源，总是使用UniformLightSampler,此积分器同样是一个实例，以更低的效率来简化操作和更少的bug。

```c++
<<SimplePathIntegrator Private Members>>= 
int maxDepth;
bool sampleLights, sampleBSDF;
UniformLightSampler lightSampler;
```

作为一个RayIntegrator,此积分器提供了Li()方法，返回了一个对于提供的光线估计出来的辐射量值。此值不提供在初次相交时初始化VisibleSurface的能力，故响应的参数已被省略。

```c++
<<SimplePathIntegrator Method Definitions>>= 
SampledSpectrum SimplePathIntegrator::Li(RayDifferential ray,
        SampledWavelengths &lambda, Sampler sampler,
        ScratchBuffer &scratchBuffer, VisibleSurface *) const {
    <<Estimate radiance along ray using simple path tracing>> 
}
```

一些变量记录了路径当前的状态，L是从$\sum P(\bar{p}_i)$运行中的总合得来的当前估计后的散射辐射量，ray在每个表面相交后会被更新，作为下一个被追踪的光线。specularBounce记录了最后出射路径采样方向是否来自镜面反射，为何需要记录此数据会在之后简短解释。

beta变量持有路径的吞吐权重，此权重作为吞吐函数$T(\bar{p_{i-1}})$的因子，也就是生成的顶点的BSDF和余弦项到此为止的乘积，除以相应的采样PDF:

$$
\beta = \prod_{j=1}^{i-2}\frac{f(\mathsf{p_{j+1}} \rightarrow \mathsf{p_j} \rightarrow \mathsf{p_{j-1}})\vert\cos\theta_j\vert}{p_\omega(\omega_j)}
$$

因此，带有路径的最终顶点的直接光散射出来的光照的beta的乘积，给出了这条路径的贡献量(此量会在后续几章中重复出现多次，并且一直视作beta)。由于之前的路径顶点的影响也以相似方式聚合，所以不需要存储路径上所有顶点的位置和BSDF，而只需要存储最近的一个点。

```c++
<<Estimate radiance along ray using simple path tracing>>= 
SampledSpectrum L(0.f), beta(1.f);
bool specularBounce = true;
int depth = 0;
while (beta) {
    <<找到下一个SimplePathIntegrator的顶点，并且累加其贡献值>> 
}
return L;
```

while循环的每次迭代是为了路径上每次多出来的部分，对应就是$P(\bar{p}_i)$项的和。

```c++
<<找到下一个SimplePathIntegrator的顶点，并且累加其贡献值>> = 
<<光线与场景物体相交>> 
<<若光线没有相交，算出无限远光源的值>> 
<<若光线未被采样，算出发光体的表面>> 
<<若达到最大深度，结束此路径>> 
<<拿到BSDF并且跳过介质的边界>> 
<<若sampleLights为true，对直接光照进行采样>> 
<<在交点处对出射方向采样，以便继续此路径>> 
```

第一步是为当前路径的部分找到光线的交点

```c++
<<Intersect ray with scene>>= 
pstd::optional<ShapeIntersection> si = Intersect(ray);
```

若没有交点，那么此光线路径就结束了。在把路径累加后的辐射量估计值返回前，一些情况下，无限远光源的辐射量会被加到这条路径的辐射量估计值中，此贡献量会被累加后的beta因子进行缩放。

若sampleLights为false，那么发光量只在光线与发光体相交时找到，这种情况下，无限远的面光源的贡献量必须把其加到那些没有与任何物体相交的光线上。若为true，那么积分器会调用Light对象的SampleLi()方法来估计每个路径顶点的直接光照量。在这种情况下，无限远的光已经被计算在内了，除了上一个顶点是镜面反射的BSDF的情况外。因此，specularBounce记录了是否最后的BSDF是完美镜面，这种情况下，面光源必须被包含在内。

```c++
<<Account for infinite lights if ray has no intersection>>= 
if (!si) {
    if (!sampleLights || specularBounce)
        for (const auto &light : infiniteLights)
            L += beta * light.Le(ray, lambda);
    break;
}
```

若光线与一个发光表面相交，也是与前文所述相同的逻辑。

```c++
<<Account for emissive surface if light was not sampled>>= 
SurfaceInteraction &isect = si->intr;
if (!sampleLights || specularBounce)
    L += beta * isect.Le(-ray.d, lambda);
```

下一步，是找到交点的BSDF，有一种特别的情况要注意，当SurafaceInteraction的GetBSDF()返回了一个未设置的BSDF时，当前面应该对光没有效果。pbrt使用这种面来表示介质之间的过渡效果，这种介质的边界的光学效果是忽略的(比如它们在面的两侧有相同的折射索引)。由于SimplePathIntegrator忽略了介质，所以跳过了这种表面的相关计算

```c++
<<Get BSDF and skip over medium boundaries>>= 
BSDF bsdf = isect.GetBSDF(ray, lambda, camera, scratchBuffer, sampler);
if (!bsdf) {
    isect.SkipIntersection(&ray, si->tHit);
    continue;
}
```

否则，我们有一个可用的表面交点，并且继续执行，depth+1.然后路径若达到最大深度则停止

```c++
<<End path if maximum depth reached>>
if (depth++ == maxDepth)
    break;
```

若显式的光照采样被执行，那么第一步就是使用UniformLightSampler来选择一个光源(回顾12.6，之采样场景的一个光源点，在给定合适权重的情况下，也能估计出所有光源的效果)

```c++
<<Sample direct illumination if sampleLights is true>>= 
Vector3f wo = -ray.d;
if (sampleLights) {
    pstd::optional<SampledLight> sampledLight =
        lightSampler.Sample(sampler.Get1D());
    if (sampledLight) {
        <<Sample point on sampledLight to estimate direct illumination>> 
    }
}
```

给定一个光源，调用SampleLi()获得一个在光源上的样本，若光的采样是可用的，就会执行直接光照算法

```c++
<<Sample point on sampledLight to estimate direct illumination>>= 
Point2f uLight = sampler.Get2D();
pstd::optional<LightLiSample> ls =
    sampledLight->light.SampleLi(isect, uLight, lambda);
if (ls && ls->L && ls->pdf > 0) {
    <<Evaluate BSDF for light and possibly add scattered radiance>> 
}
```

以公式12.7返回的路径追踪估计式，我们就有了路径的吞吐权重beta，这个值对应了式子中括号里的部分。调用SampleLi()会生成一个样点。由于光采样方法是根据立体角采样的，而不是面上，我们有必要用雅各比纠正项，估计式会变成:

$$
P(\vec{p_i})=\frac{L_e(p_i\rightarrow p_{i-1})f(p_i \rightarrow p_{i-1} \rightarrow p_{i-2})\vert\cos\theta_i\vert V(p_i \leftrightarrow p_{i-1})}{p_l(\omega_i)p(l)}\beta \tag{13.9}
$$

此处$p_l$是给定光源l在$\omega_i$的立体角密度，$p(l)$是采样更远l的离散概率(回顾等式12.2).它们的乘积给出了光源采样的总体概率

在对阴影光线追踪以计算出可视因子V前，有必要检查对于采样的方向的BSDF是否是0，在这种情况下，计算的开销是没必要的

```c++
<<Evaluate BSDF for light and possibly add scattered radiance>>= 
Vector3f wi = ls->wi;
SampledSpectrum f = bsdf.f(wo, wi) * AbsDot(wi, isect.shading.n);
if (f && Unoccluded(isect, ls->pLight))
    L += beta * f * ls->L / (sampledLight->p * ls->pdf);
```

Unoccluded()是由Integrator基类提供的遍历方法

```c++
<<Integrator Public Methods>>+= 
bool Unoccluded(const Interaction &p0, const Interaction &p1) const {
    return !IntersectP(p0.SpawnRayTo(p1), 1 - ShadowEpsilon);
}
```

为了采样出下一个路径顶点，通过调用BSDF的采样方法或均匀采样，来找到光线离开表面时的方向，这取决于sampleBSDF这个参数

```c++
<<Sample outgoing direction at intersection to continue path>>= 
if (sampleBSDF) {
    <<Sample BSDF for new path direction>> 
} else {
    <<Uniformly sample sphere or hemisphere to get new path direction>> 
}
```

若使用的是BSDF采样，Sample_f()方法能给出方向和关联的BSDF，PDF值。根据方程13.8,能够更新beta的值

```c++
<<Sample BSDF for new path direction>>= 
Float u = sampler.Get1D();
pstd::optional<BSDFSample> bs = bsdf.Sample_f(wo, u, sampler.Get2D());
if (!bs)
    break;
beta *= bs->f * AbsDot(bs->wi, isect.shading.n) / bs->pdf;
specularBounce = bs->IsSpecular();
ray = isect.SpawnRay(bs->wi);
```

否则，就按照代码片段<<在(半)球面均匀采样以获取心得路径方向>>执行。这段代码比RandomWalkIntegrator更精细，比如：若表面会反射，但是不会透射，代码会确认采样的方向是根据光散射所在的半球面上的。我们不会在此包含这段代码片段，因为此片段必须处理一堆场景，没有太大必要关注其内部工作细节。

## 13.4 更好的路径追踪
