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

设备上图像的显示方向，VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR 就是不旋转, 其他几个选项分别就是旋转多少度

## 获取交换链图像的句柄

驱动可能创建比创建交换链时提供的图象数更多的图像。要渲染到这些图像之前，要获取到这些图像的句柄，获取句柄前，需要先获取到这些图像的总数。这些图像需要创建一个image view来进行封装，以便在创建framebuffer时使用。

framebuffer就是一组正在渲染中的图像

## 信号量与栅栏

信号量(semaphore)用于对设备的队列做同步操作，当我们把处理指令提交时，这些指令可能需要某些其他工作先完成，这就需要真正执行这个指令前，要等待其他指令执行完，信号量就是用来做此工作的，它主要是对设备内部的队列做同步。

应用和提交的指令之间的同步，需要使用栅栏(Fence),栅栏可用来通知应用，某些工作已经完成了，应用刻意获取到栅栏的状态，来检查某些指令是在执行中还是已执行完。

一般来说，用semaphore就可以了，其在驱动层进行等待，fence是在应用层等待，性能降低一般更多

在获取到交换链的图像并使用前，要改变图像的布局(layout), 布局就是图像内部的内存组织结构，这个组织结构根据当前图像的使用目的而不同。若想用不同的方式使用这些图像，就需要改变其布局。

用于呈现的图像必须有VK_IMAGE_LAYOUT_PRESENT_SRC_KHR的布局，但若我们想往图像上渲染，就必须要有VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL布局，变换布局的操作就叫做变换(transition)

## 指令缓冲

Vulkan比起OpenGL来说能控制的细粒度更小，指令缓冲(command buffers)可以用来控制哪些/何时把指令送给硬件。是Vulkan最重要的对象之一。并且可在多线程下记录这些指令，并且支持自身的复用，这可以省下额外的处理时间。

由于细粒度较小，所以控制起来也比较麻烦，需要用到信号量或栅栏来同步指令在多线程下的执行前后顺序，还要管理指令的执行时间。

## 创建指令池和指令缓冲

指令缓冲需求的内存要从指令池里获取。故创建指令缓冲前，需要先创建指令池。指令池有参数flag，代表如何操作指令池里的指令缓冲

- VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: 代表这个缓冲只会存活很短是的时间，只会提交很少的次数，然后会被立即重置或者释放
- VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: 可以独立的重置任意池里的缓冲，若不设此flag,则只能把池里的缓冲全部一起重置

指令池还可以通过queue_family_index, 控制提交到哪个队列。同一时间下，池只能由一个线程访问，池里的缓冲不能同时记录多个线程的指令，所以要支持多线程指令记录，要把指令池分开。

指令缓冲分为主要和次要，在创建时通过level区分，主要缓冲可以直接把指令提交到队列，同时还能调用次级缓冲；次要缓冲只能执行主要缓冲的指令，不能提交指令到次要缓冲中

## 指令缓冲相关操作

要设置缓冲的使用方式，然后调用begin操作。这是必须的。usage值有三种:

- VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: 若是次级缓冲，并且是在渲染通道里，用此值
- VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: 若缓冲只提交一次然后会重置或者重新记录指令，用此值
- VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: 使用此值在指令还在执行时重新提交到队列里（由于影响性能，应该避免使用）

指令缓冲开始以后，就可以往里面提交指令，指令对应的对象就是vkCmd前缀，并且第一个参数总是VkCommandBuffer

指令的记录操作在vkBeginCommandBuffer()和vkEndCommandBuffer()之间进行，并且只有在end后才能把这个command buffer进行提交

为了保证对性能最小的影响，指令中如果有错误，只在end时报错，记录的中途不会报错

若指令缓冲完成了之前的记录操作，或者记录有报错，就需要在再次使用前重置，通过begin，或reset操作都可以达到重置指令缓冲的目的

指令缓冲可以通过所在的指令池一起进行重置，如果要单独对某个指令缓冲重置，要在创建时设置flag为reset command_buffer标识

使用begin是隐式地重置指令缓冲，reset的显式重置

指令缓冲通过vkSubmitInfo提交到queue，提交时要设定各种同步量，为了性能更好尽量减少提交调用次数。提交过，或者还没执行完的command buffer不能再次提交，除非设置了usage为simultaneous标志，但是一般来说要避免使用此标志

## 同步操作

信号量用于在指令缓冲之间进行同步，栅栏用于在应用和已提交的指令之间进行同步。
信号量会自动重置状态，但是栅栏在变为已通知后，需要应用自己把其重设为未通知状态。

### 信号量(semaphore)

可协调提交到队列的各种不同的指令，也可在同一个逻辑设备的不同队列间进行协调。把指令提交到队列时需要用到信号量，故在提交前，需要先创建信号量。其还可在交换链中获取图像时使用

信号量只有两种状态: 已通知和未通知状态。且其只能在图形的硬件内部层面协调，应用不能访问信号量的状态。若应用需要在已提交的指令时同步，需要用栅栏

### 栅栏(fence)

可用于在应用和已提交的指令之间进行同步。当提交的指令完成时，栅栏会通知应用。在使用之前需要先创建。也是有两种状态：已通知和未通知，但是状态的重设(从已通知到未通知)要通过应用来操作。

为了通知栅栏，需要在指令缓冲的提交操作前提供，这种情况下就类似信号量，当所有工作已提交时就会变为已通知状态。但是栅栏不能被用作指令缓冲的同步。

使用vkWaitForFences()来阻断应用代码运行，至多阻断到超时时间，若在时间内满足fence通知条件(全部或其中之一已通知)，则会返回VK_SUCCESS,否则超时会返回VK_TIMEOOUT。若要检查栅栏的状态，只需把timeout值设置为0，这样，vkWaitForFences()不会阻断程序，并会直接返回当前栅栏的状态值，VK_TIMEOUT代表未通知，VK_SUCESS代表已通知

栅栏在重用之前需要在代码中显式重置状态

## 