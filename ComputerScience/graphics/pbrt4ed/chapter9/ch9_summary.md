# 第九章总结

BSDF(双向散射分布函数)用来描述表面上某点的散射效果。散射效果分为反射和透射，分别对应BRDF和BTDF。即BSDF = BRDF + BTDF 

## 四种反射类型

1. 漫反射(diffuse):光线入射后，向法线同向的半球面均匀随机方向出射
2. 抛光反射(glossy): 光线入射后，反射方向附近的出射的多，半球面其他方向出射的少
3. 完美镜面反射(specular)：光线入射后，集中于反射方向出射
4. 逆反射(retroreflective): 光线入射后，朝着光线及其附近方向反向出射

![图9.1](img/fg9_1.png)

## 各向同性/各向异性

光的反射方向的分布，若会根据入射方向改变，叫各项异性，如拉丝金属表面；若不会根据入射方向改变，叫各项同性，比如一般的纸张，木头表面等