# 第五章 呈现

1. 生成并处理图像，最终把结果展示给用户的过程，叫做呈现，但是呈现不属于Vulkan的核心API
2. 呈现是被一组扩展支持，称为WSI扩展，在Vulkan中此种扩展必须显式启用，且每个平台稍有不同，需要根据平台来启用对应的扩展
3. Vulkan里的呈现是被一小组扩展来处理的。其功能在所有平台都是同一个扩展，这个扩展下，根据平台不同，又可细分为多个更小的根据平台区分的扩展
   
## 呈现界面(presentation surface)

要被呈现的图像渲染数据对象，叫做界面，用VkSurfaceKHR表示，其是通过VK_KHR_surface引入的，此扩展为界面对象添加了通用的功能，可支持多个平台

## 交换链(swap chain)

1. 交换链就是一组用于在surface上展示的image，通常是用ring buffer来管理一组图像
2. 应用可以请求交换链获取可用的图像，渲染，然后返回交换链，展示出来。里面的其中一个用于展示，其他图像可以同时在绘制中
3. 通过vkCreateSwapchainKHR()创建交换链