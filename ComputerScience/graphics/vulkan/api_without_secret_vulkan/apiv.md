# API without Secrets: introduction to Vulkan

> 个人整理精华部分

## 前言

要配合源码来学习，地址: https://github.com/GameTechDev/IntroductionToVulkan

## 开始学习Vulkan

在我们的应用中，有三种方式使用VulkanAPI:

1. 动态加载提供了VulkanAPI实现的驱动库，我们自己从这个库里请求函数的指针来使用
2. 用Vulkan SDK提供的Vulkan运行时(Vulkan Loader)的静态库，链接到我们的应用
3. 在应用运行时，动态加载Vulkan SDK的Vulkan Loader,把函数指针进行加载

第一种方式不推荐，因为硬件商可能会修改他们的驱动，这会影响我们应用的兼容性。而且若程序有错误，驱动可能没有更多的信息，不方便调试

推荐的方式是用Vulkan SDK里的Vulkan Loader,更好配置也更灵活，同时可以开启验证层方便调试

用Vulkan Loader静态链接或者动态链接看自己的喜好，本文使用动态链接。此种方式只会加载某些基本函数，其余的函数必须用wglGetProcAddress()或windows的GetProcAddress()来加载

## 加载Vulkan的运行时库并且对一个已导出的函数请求其指针

Vulkan的动态库在windows里叫vulkan-1.dll，在linux里叫libvulkan.so

```C++
#if defined(VK_USE_PLATFORM_WIN32_KHR)
VulkanLibrary = LoadLibrary( "vulkan-1.dll" );
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
VulkanLibrary = dlopen( "libvulkan.so", RTLD_NOW );
#endif

if( VulkanLibrary == nullptr ) {
  std::cout << "Could not load Vulkan library!" << std::endl;
  return false;
}
return true;
```

### 1. tutorial01.cpp的函数LoadVulkanLibrary()

VulkanLibrary的变量类型在Windows中是HMODULE，在linux中是void*。若加载库的函数返回非0，我们就可以加载所有已导出的函数。Vulkan库及其实现被要求只根据我们的操作系统拥有并且能用基本技术加载的唯一的函数。Vulkan API的其他函数也可通过这个方法进行导出，但是不保证能正确导出,也不推荐这样做。必须被导出的唯一一个函数是vkGetInstanceProcAddr()

这个函数被用来加载其他所有的Vulkan函数。在头文件写成宏更方便使用，避免在多个地方重复这个函数名

```c++
#if !defined(VK_EXPORTED_FUNCTION)
#define VK_EXPORTED_FUNCTION( fun )
#endif

VK_EXPORTED_FUNCTION( vkGetInstanceProcAddr )

#undef VK_EXPORTED_FUNCTION
```

### 2. ListOfFunctions.inl

