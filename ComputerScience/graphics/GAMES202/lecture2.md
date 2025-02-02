# 第二课 图形学回顾

## OpenGL介绍
工作过程可以用画油画来打比方：

1. 放置物体/模型
    - VBO（顶点缓冲对象）：GPU里的一块区域，存放用户定义的顶点，法线，纹理坐标等信息。类似obj文件
    - 变换矩阵已经内置在OpenGL中了，比如glTranslate, glMultMatrix等，不需要自己写变换矩阵
2. 设置画架的位置(设置相机)
    - 视图变换（比如透视关系），比如gluPerspective()指定视场角，长宽比，近平面，远平面
    - 创建或指定一个framebuffer帧缓冲
3. 在画架上摆上画布
    - 选定使用的framebuffer
    - 选定一个或多个作为输出的纹理（着色，深度等）
    - 渲染一次(one pass)场景
4. 在画布上画画
    - 比如，确定如何着色
    - 到这里会使用顶点、片段着色器
    - 对于顶点着色器，是用来并行处理每个顶点的矩阵转换(MVP矩阵变换)，插值等操作
    - OpenGL自动光栅化，把三角形变成一堆像素片段
    - 对于每个片段，调用片段着色器，进行着色和光照计算等操作，也可以自己写深度测试
5. (在画架上放上其他画布然后继续画画)
6. (把之前的画拿来当参考)

**总结**:
在每一次渲染的处理流程(pass)如下:
- 定义物体，相机，MVP等
- 定义framebuffer，输入和输出的纹理(图像)
- 定义顶点和片段着色器
- 开始渲染

## 着色语言

### 着色语言历史
早期GPU变成是写汇编，由于难度过高，发明了SGI语言，后面英伟达出了Cg语言，再往后在directX出了HLSL语言，在OpenGL里叫GLSL,这些语言最终还是会编译为汇编语言。

### 着色的配置
1. 初始化
    - 创建着色器（顶点，片段）
    - 编译着色器
    - 把着色器绑定到program上（这个program就是自定义的着色器包含在一起的一个地方）
    - 链接着色器（检查program里的着色器是否能对应得上）
    - 使用这个program
  
### shader的调试

1. 早年: 用Nvidia Nsight
   - 需要多个GPU来调试GLSL
   - 在HLSL上必须用软件模拟来运行
2. 用Nishgt Graphics(只支持Nvidia)，或RenderDoc(跨平台)
3. 闫调试法：把要输出的值以颜色的值打印出来，用像素点颜色去debug

### 渲染方程

$$
L_o(p,\omega_o) = L_e(p, \omega_o) + \int_{H^2}f_r(p,\omega_i \rightarrow \omega_o)L_i(p, \omega_i)\cos \theta_i d\omega_i
$$

RTR中的渲染方程:

$$
L_o(p,\omega_o) = \int_{\Omega^+}L_i(p, \omega_i)f_r(p,\omega_i,\omega_o)\cos \theta_i V(p, \omega_i)d\omega_i
$$

> V函数代表可见性, $L_i$和$f_r$对调这里是代表这个BRDF是被$\cos$加权的
>
> $\Omega^+$代表朝外与法线同侧的半球面

### 全局光照问题

通常就是多bounce一次，因为后续再加bounce数不如从0到1的变化大
