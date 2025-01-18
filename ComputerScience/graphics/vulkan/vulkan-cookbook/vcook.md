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

