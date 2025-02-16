# Vulkan Cookbook

## 启用验证层的方式

书上只提到一种全局设置的方式，一般在工程里，是在代码层面配置

开启验证层的前提，是instance要支持验证层扩展，所以首先要看实例是否支持验证层，获取实例支持的扩展通过两趟调用vkEnumerateInstanceExtensionProperties完成，第一趟获取支持的扩展数量，第二趟把这些具体的扩展取出来

```c++
	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

	std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));
```

比如,在Vulkan-Samples中,是利用宏来决定是否启用验证层:

```c++
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	bool has_debug_utils = false;
	for (const auto &ext : available_instance_extensions)
	{
		if (strncmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME, strlen(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) == 0)
		{
			has_debug_utils = true;
			break;
		}
	}
	if (has_debug_utils)
	{
		required_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	else
	{
		LOGW("{} is not available; disabling debug utils messenger", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
#endif
```

详见:

1. [Vulkan特性文档](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#vkEnumerateInstanceExtensionProperties)
2. [Vulkan SDK文档](https://vulkan.lunarg.com/doc/sdk/1.3.250.1/windows/layer_configuration.html)

## Vulkan如何加载

首先，Vulkan本身，只是定义了如何操作GPU进行图形绘制或计算的API，这些API的具体实现，是在GPU驱动里， vulkan驱动里，要注意vulkan的版本号，不同版本的vulkan，接口或者实现是不同的。

Vulkan的加载器(Loader)负责把Vulkan自己的API转到驱动里对应的实现上，由于驱动很多，loader就负责决定vulkan的api究竟调用到驱动的哪个函数上，这种对应关系，写成了库，在windows上，叫vulkan-1.dll, linux上，叫libvulkan.so.1。

所以使用vulkan时，第一步应该要加载这个loader，这个Loader在windows平台，是包含在驱动里的

书中是手写了加loader库的过程，现代一般是两种，用volk的volkInitialize(),或者vulkan-hpp的VULKAN_HPP_DEFAULT_DISPATCHER.init()方法

## Vulkan的接口如何使用

API可以参考Vulkan Documentation，要使用这些函数，有两种方法:

1. 把vulkan的loader库静态连接到项目里，引入vulkan.h后就可使用里面定义的函数，相当于把整个vulkanAPI都引入了项目
2. 先禁用vulkan.h里的函数原型，然后在你的项目里动态加载函数指针，项目在运行时动态加载这些vulkan函数的实现。

    第一种方法简单但是影响性能，之所以影响性能，原因是程序运行时，由于设备和驱动不同，vulkan的加载器要把这些函数重新根据设备和驱动做重定向，这个过程是需要一些时间的

    第二种方式就要在代码层面做更多额外工作，但是就可不必对这些API做重定向，所以性能相对好一些。而且也可以自由选择要加载的函数，不需要的函数，就可以不加载。

    第二种方式的实现，一般用宏来做，根据操作系统和vulkan版本，手写要导出哪些vulkan函数，比较麻烦。具体见书或API without secret vulkan

    怎么知道vulkan函数指针的类型，若某个函数叫\<name\>,则指针类型就是PFN_\<name\>  

    现在一般要么用volk，要么vulkan-hpp，这样更方便

## 如何加载vulkan的函数

在windows上，利用GetProcAddress()函数，获取到某个已加载的动态链接库(vulkan-1.dll)的特定函数地址，唯一一个要公开导出的只有vkGetInstanceProcAddr()，这个函数用来平台无关的加载其他Vulkan函数。

导出了这个函数后，重写这个导出函数的宏，把要导出的Vulkan函数,根据平台（比如Windows用GetProcAddress），导出其他要用到的vulkan函数指针

## 导出全局函数

vulkan的函数分为三个级别，global，instance和device，利用之前导出来的vkGetInstanceProcAddr，加载其他vulkan函数(也是用宏)，先把global级别的函数导出来，主要包括实例创建相关的函数

## 检查可用的实例扩展

要先把instance级别用到的函数导出来，检查是否都支持，才能成功创建实例。通过调用vkEnumerateInstanceExtensionProperties两次，把支持的扩展取出来

## 枚举物理设备，检查设备支持的扩展，获取设备特性和属性

这一步是为了选取支持我们需求特性和属性的设备

- 属性(properties): 包括设备名称，驱动版本，VulkanAPI版本，设备类型(集成/独立显卡)等
- 特性(features): 是否支持几何，曲面细分着色，多视口等

## 检查队列家族

指令都要提交到队列里，对立隶属于不同的队列家族，每个家族对应不同的特性，指令提交到不同的队列，是独立执行的，因此，在必要的时候，还需要进行同步（synchronize）操作,以实现某些时候并行执行的同步需求

重要的设备属性之一是队列可执行的操作类型,包括:

- Graphics: 代表可创建图形管线和绘制
- Compute: 代表可以创建计算管线并且分派计算着色器
- Transfer: 用于超快的内存拷贝操作
- Sparse: 允许额外的内存管理特性

一个队列家族可以同时支持以上多种

队列家族属性中，还会告诉我们队列的数量，支持的时间戳，拷贝image时的最小部分等。

## 创建逻辑设备

指定要用到的家族，及其要用到的队列数量。大部分操作都是在逻辑设备上进行的，比如渲染，创建图像，缓冲，管线，加载着色器，指令的记录和提交等

## 导出设备级函数

使用vkGetDeviceProcAddr()传入逻辑设备指针和函数名，包括扩展的函数也需要导出

## 获取设备队列

不需要创建，而是通过逻辑设备对象来对其进行操作

## 图像的呈现

由于vulkan不单用于图形渲染，还包括数学和物理的计算功能，还得支持跨设备，跨操作系统，所以，图像的呈现要根据不同的操作系统对应的标准来开启对应的扩展才能支持。

关于图像呈现的扩展一般叫做WSI(Windowing System Integration)。最重要的扩展是创建交换链。交换链是一组可被直接显示的图像。

## 呈现界面

surface(界面)一词，就是对视窗再次抽象出来的高层概念，不管是哪个操作系统的视窗，都是一种界面。要在屏幕上显示图像，首先要创建一个surface，创建surface之前，要确认物理设备具备这种能力

## 检查物理设备是否支持界面

在选择物理设备时，需要加上

```c++
// presentation_surface为之前根据平台创建好的surface
vkGetPhysicalDeviceSurfaceSupportKHR( physical_device,index, presentation_surface, &presentation_supported);
```

返回VK_SUCCESS则可以获取该设备的队列家族索引

## 选择呈现模式

presentation_mode指如何在屏幕上显示交换链里的图像，目前有四种:

1. IMMEDIATE: 当图像被呈现时，立即替换当前展示的图像，不等待
2. FIFO： 所有驱动都得支持的模式，图像排队展示，队列长度是图像数量-1。若开启垂直同步，图像展示之间有一段空白时间方便图像在屏幕上展示完成，故无图像撕裂问题
3. FIFO RELAXED: 只在展示图片快到超过屏幕刷新率时，才在展示之间加入空白时间，当两个图像展示间隔两倍于刷新间隔时间时，图像立即呈现。
4. mailbox: 在三重缓冲时能够被觉察到。等待展示的队列只有一张图，若渲染出了新的图，则用新图替代这张图，故总是展示最新的预展示图

一般来说用的是FIFO

## 获取设备的surface相关能力

包括:

1. 最大最小交换链里的图像数量(创建交换链时需要此参数)
2. 最大，最小，和当前的界面范围
3. 支持的图像变换方法
4. 支持的最大图像层(layer)
5. 使用的场景
6. 支持的界面alpha值的组合模式

## 交换链图像的使用场景

交换链中的图像不单是用于渲染，还可以用于采样，复制和被复制等操作，这些使用场景要在交换链创建前定义好，但是要保证这些使用方式是可被支持的。

步骤:

1. 检查设备是否支持，在supportedUsageFlags字段，用位运算符判断
2. 创建交换链时，设置好对应的imageUsage值

此值若用于渲染，一般就是VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT

## 交换链图像的变换

设备上图像的显示方向，VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR 就是不旋转