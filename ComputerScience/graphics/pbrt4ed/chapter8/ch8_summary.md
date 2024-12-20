# 第八章总结

## 采样理论

能代表一个图像的函数称为成像函数。真实世界中，理想的成像函数的图像是平滑连续的。由于计算机只能通过对图像某个地方的颜色进行有限次数的采样，所以其对应的成像函数是用离散的点表示的。

把采样后的这些离散点的值的集合，重新转换成近似原连续成像函数的过程叫做**图像重建**, 若重建与原函数越近似，一般渲染效果就越好。由于这个近似过程不可避免会有误差，这种误差就会造成**走样(aliasing)**现象,对走样现象的处理就叫做**反走样(antialiasing)**

如何衡量成像函数与原始成像函数的近似程度，方法就是**傅里叶分析**

### 图像像素和显示像素的区分

- **图像像素**: 对某个位置的点进行采样，通过计算得出的理论像素值，像素点是无限小的
- **显示像素**: 基于图像像素，并根据显示设备特性，得出的像素值，像素点是有大小的

### 傅里叶分析基础

### 傅里叶变换

一切波都可以通过多个不同频率的正弦波合成来近似。

**时域**就是这些多个正弦波合成后，随着时间变化的图像。

**频域**就是某个时间点上，这个波通过多个频率正弦波的振幅组成的图像

若某个图像再某些地方的颜色变化剧烈，它就是图像高频的部分，反之就是低频的部分。

高频部分对应的频域图像就比较平滑，是由于这种图像变化快，需要多个频率的波合成来近似，所以横轴频率上的振幅都是要做更多贡献的，频域图像看起来就比较平滑。反之，低频的频域图像只需要几个频率贡献就可以合成，所以要做贡献的频率没那么多，频域函数图像就忽上忽下。
