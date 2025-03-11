# 第七章 图形管线

> 待补跳转:
>
> - [待补1](#待补1)
> - [待补2](#待补2)
> - [待补3](#待补3)

你会学到的内容:

- Vulkan的图形管线长什么样
- 如何创建图形管线对象
- 如何使用Vulkan绘制图元

把Vulkan当作图形API可能就是最常见的用法。图形功能就是Vulkan里最基本的部分，而且也是任何视觉相关应用的驱动核心。Vulkan中的图形处理可被视为一种管线，管线里需求的图形指令会在其中获取，并最终在显示设备上生产出一张图片。本章对Vulkan中图形管线的基础做了概括，并且介绍了我们的第一个图形例子。

> Vulkan一般处于视觉相关应用的核心位置，图形管线类似生产流水线，每一道工序就是一个阶段(stage),穿梭于这些阶段之间的就是各种图形指令，流水线的最终输出就是图片

## 逻辑图形管线

Vulkan中的图形管线可视作一个生产流水线，指令从管线的开头输入，并在各个阶段中处理。每个阶段执行一种类型的转换，执行前会取到指令和其相关的数据，然后把这些指令和数据转换成其他的指令和数据。直到管线的末端，这些指令就已被转换成各种颜色的像素，这些像素组成了你需要输出的图片。

图形管线中的许多组件都是可选的，可不启用，甚至不被Vulkan的实现设备所支持。一个应用中只有唯一一个组件是必须启用的，就是顶点着色器(vertex shader)。完整的Vulkan图形管线见图7.1。然而，不要被吓到，我们会在本章中慢慢介绍每个阶段，并会在后续章节深入其中细节

![图7.1](img/fg7_1.png)

下列是每个管线中每个阶段的简明描述：

- `draw(绘制)`: 你的指令进入到vulkan图形管线的地方，一般来说，vulkan的设备里会有一个小型处理器或者某个硬件会把command buffer中的指令进行解析，并且与硬件直接交互，来归纳出具体工作
- `input assembly（输入集成）`: 这个阶段会把你发送绘制时，包含了顶点信息索引和顶点缓冲进行读取
- `vertex shader(顶点着色器)`: 顶点着色器在此执行，此阶段会把顶点的属性输入，然后为下一阶段准备转换和处理后的顶点数据
- `tessellation control shader(细分控制着色器)`: 这个可编程的着色阶段是负责生成细分因子和其他被用于固定功能细分引擎的每个补丁过的数据
- `tessellation primitive generation(细分图元生成)`: 没在图7.1中，此阶段有着固定的功能，会利用上一阶段的细分因子来把图元拆分成更小更简单的图元，以便在后续的细分计算着色器中进行着色
- `tessellation evaluation shader(细分计算着色器)`: 这个着色阶段会把在细分图元生成器生成的每个新的顶点上运行。其操作与顶点着色器类似，不同的是其只对传入的生成的顶点着色，而不是从内存里读取的那些顶点进行着色。
- `geometry shader(几何着色器)`： 这个着色阶段会在完整的图元上操作。图元可能是点，线或者三角形，又或是前三者的特殊变种。这个阶段有能力在管线中途改变图元的类型
- `primitive assembly图元组装`： 这个阶段会把顶点，细分，或几何阶段生成的多个顶点分组，并形成可用于栅格化的图元。这个阶段也会裁剪和剔除图元，然后把它们转换成恰当的viewport(视口)中
- `裁剪和剔除`: 这个固定功能的阶段决定这些图元的那一部分会对输出图像有贡献，并且对于没贡献的部分图元进行丢弃，对可能可见的图元转发给栅格化
- `rasterizer(栅格化)`：栅格化是vulkan中所有图形的基础核心。栅格化会取已被组合好，用一串顶点表示的图元，然后把其转换成单个fragment(片段),这些片段可能就是组成你图片的各个像素
- `prefragment operations(片段前操作)`: 一旦片段的位置已知，某些操作就可在片段上执行，这些预片段操作包括深度和模板测试(若已启用)
- `fragment assembly(片段组装)`: 没在图里展示，片段组装阶段取栅格化的输出结果，称为一个组，传入到片段着色阶段
- `fragment shader(片段着色器)`: 这个阶段运行在管线中的最终着色器，此着色器负责计算出会被送往最终固定功能的处理阶段的数据
- `postfragment operation(片段后操作)`: 在某些场景下，片段着色器修改了会在片段前操作的数据。这种情况下，那些片段前的操作会移动到片段后的阶段来执行
- `color blending(混色)`: 颜色操作会取片段着色器和片段后操作的最终结果，并且把这些数据用于更新framebuffer(帧缓冲)。 颜色操作包括有混色，和逻辑操作

如上所述，在图形管线中，许多阶段之间是相互关联的，这点与在第六章介绍的计算管线不同。图形管线不单包含了固定功能中各种选项的配置，还有着多达5个的着色器阶段。此外，一些逻辑上有着固定功能的阶段，根据具体设备，实际上至少有一部分是从驱动生成并用着色器代码实现的。

把图形管线表示为一种对象的目的，就是为Vulkan的实现提供尽可能多的信息，以便把那些固定功能的硬件和可编程的着色器核心之间的部分进行移动

> Vulkan通过将图形管线作为一个对象，让驱动程序提前知道整个渲染流程的详细信息，使其能够在不同的GPU架构上灵活分配计算任务，最大化利用硬件资源，提高性能和效率

若这些信息在同一对象的同一时间中并不都可用，可能意味着vulkan的一些实现可能需要基于那些可配置的状态进行重编译。为了避免这种情况，图形管线中的这组状态已被精心挑选出来了, 这可让状态的切换尽可能快。

vulkan中绘制的基础单位是顶点。顶点被分组成图元，然后被vulkan管线处理。最简单的绘制指令是vkCmdDraw()，它的原型是这样的:

```C
void vkCmdDraw(
    VkCommandBuffer commandBuffer, // 提交到哪个指令缓冲
    uint32_t vertexCount, // 绘制的顶点数量
    uint32_t instanceCount, // 实例的数量，一般为1，需要重复绘制几乎相同顶点时可设为其他值
    uint32_t firstVertex, // 从顶点索引为几开始绘制，一般为0，想不从0开始可设为其他值
    uint32_t firstInstance // 从第几个实例的索引开始绘制，一般为0，想不从0开始可设为其他值
);
```

与其他Vulkan指令类似，vkCmdDraw()会传到一个指令缓冲中，并在之后被设备执行。传入到哪个指令缓冲，就由commandBuffer字段来设定。输入到管线中的顶点数量，就在vertexCount中设定。若你想重复绘制只有些许不同的顶点集，你可以在instanceCount中设定实例(instance)的数量。这被称为实例化(instancing)，我们会在后面的章节介绍。现在，我们可以把instanceCount设为1。不从顶点或实例里索引为0的顶点开始绘制也是可以的，只需设置firstVertex和firstInstance即可，此处也会在后文介绍，在这里，也都设为0。

在绘制之前，你必须把一个图形管线绑定到指令缓冲上，绑定前，必须先创建图形管线。若你没绑定管线就尝试绘制，就会导致未定义行为发生(一般是异常行为)

当你调用vkCmdDraw()时，vertexCount个顶点会被生成，并且输入到当前图形管线中。每个顶点都会被输入集成(input assembly)执行，然后是你的顶点着色器。

声明额外的输入是可选的，但顶点着色器必须有。因此，最简单的图形管线仅由一个顶点着色器组成

> 在Vulkan中，程序可以选择是否声明额外的输入数据（如自定义的顶点属性或推送常量），但顶点着色器是必须的。这意味着，即使是最简单的图形管线，也至少需要一个顶点着色器来处理输入数据并生成后续渲染阶段所需的信息

## 渲染通道(Renderpass)

图形管线和计算管线的区别之一，一般来说，就是图形管线会被用来把像素渲染出来，并把其形成图像，可能还有额外的处理或者会展现给用户。在复杂的图形应用中，图片可能是用多个通道建立的，每个通道负责场景中不同的部分，把各种效果在整个帧上执行，比如后处理，或者组成，渲染UI元素等。

这些通道在Vulkan中可用renderpass对象来表示。单个渲染通道对象会把在单独一组输出图像上的多个通道或渲染阶段封装起来。渲染通道中的每个通道被称为子通道(subpass)。Renderpass对象可以包含多个子通道，但即便是只有单个输出图像的单的通道的简单的应用，renderpass对象也包含了这个输出图像的信息

所有的绘制必须包含在某个渲染通道中。此外，图形管线需要知道它们要渲染到哪里去。因此。在创建图形管线前，有必要创建一个renderpass对象，以便让管线知道其要输出什么图像。在第13章"多通道渲染"中，会更加深入地介绍渲染通道。本章中，我们会创建一个最简单的renderpass对象，让其可渲染出一个图像

为了创建一个renderpass对象，可调用vkCreateRenderPass(),其原型如下:

```c
Vk Result vkCreateRenderPass {
    VkDevice device, // 创建此renderpass的设备
    const VkRenderPassCreateInfo* pCreateInfo, // renderpass创建时的参数结构体的指针
    const VkAllocationCallbacks* pAllocator,
    VkRenderPass* pRenderPass
};
```

vkCreateRenderPass()里的device参数就是要创建renderpass对象的设备，pCreateInfo是指到定义renderpass的结构体。其结构体定义如下:

```c
typedef struct VkRenderPassCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkRenderPassCreateFlags flags;
    uint32_t attachmentCount;
    const VkAttachmentDescription* pAttachments;
    uint32_t subpassCount;
    const VkSubpassDescription* pSubpasses;
    uint32_t dependencyCount;
    const VkSubpassDependency* pDependencies;
} VkRenderPassCreateInfo;
```

sType字段需要设为VK_STRUCTURE_TYPE_RENDERPASS_CREATE_INFO, pNext应被设为nullptr，flags是预留给未来使用的，需被设为0

pAttachments,是指向attachmentCount数组的指针，VkAttachmentDescription结构体定义了与此renderpass关联的附件(attachment)。每个结构体定义了可被单个或多个子通道作为输入/输出的单个图像。若确实renderpass没有关联的附件，可把attachmentCount设置为0,pAttachments设置为nullptr。然而，除了某些进阶使用场景外，几乎所有图形渲染都会至少使用一个附件，VkAttachmentDescription的定义如下:

```c
typedef struct VkAttachmentDescription {
    VkAttachmentDescriptionFlags flags; // 一般设为0，除非想多附件共用一块内存来节省内存
    VkFormat format; // 设定附件格式，取VkFormat的其中一个枚举值
    VkSampleCountFlagBits samples; // 要多重采样时使用，代表图像中的样本数
    VkAttachmentLoadOp loadOp; // 设定renderpass开始时，如何加载附件
    VkAttachmentStoreOp storeOp; // 设定renderpass结束时，如何存储附件
    VkAttachmentLoadOp stencilLoadOp; // 若附件是depth-stencil结合的，可定义renderpass开始时的加载操作
    VkAttachmentStoreOp stencilStoreOp;// 若附件是depth-stencil结合的，可定义renderpass结束时的存储操作
    VkImageLayout initialLayout; // renderpass开始时image的期望布局(renderpass不负责转换布局)
    VkImageLayout finalLayout; // renderpass结束时image的布局(renderpass会转换成此布局)
} VkAttachmentDescription;
```

flags字段用于为Vulkan给出关于此附件的额外信息。其只有一个值: VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT, 若已设置，代表这个附件可能被同一个renderpass的另一个附件所引用，并使用同一块内存。此选项告知VUlkan不要做任何会导致此附件不一致的操作。此选项在某些进阶场景中会被使用，这些场景中，内存是珍贵的，并且你想优化这些内存的使用。大部分情况下，flags可被设为0。

format字段设定了附件的格式，其值来自于VkFormat枚举的其中之一，并且要与被当作附件的图像的格式相匹配。同样的，samples代表了图像中的样本数，多重采样时会被使用，当不使用多重采样，可把samples设置为VK_SAMPLE_COUNT_1_BIT

之后的四个字段对renderpass的开头和结束时如何处理附件做出了设定。加载操作会告知Vulkan当renderpass开始时，要对附件做什么，其可被设置为以下值:

- VK_ATTACHMENT_LOAD_OP_LOAD: 代表附件已包含数据，并且你想渲染到其上。这会导致Vulkan在renderpass开始时，把附件的内容看作可用的
- VK_ATTACHMENT_LOAD_OP_CLEAR: 代表你想要Vulkan在renderpass开始时清除附件。当renderpass开始后，想要清除的附件颜色会被设定
- VK_ATTACHMENT_LOAD_OP_DONT_CARE: 代表你不关心在renderpass开始时附件的内容如何处理，Vulkan可随心所欲处置，此值可用来显式清除附件，或者你想让自己知道你会在renderpass里替换附件的内容

同样的，存储操作会告知Vulkan你想在renderpass结束时如何操作附件的内容，其可被设为以下值:

- VK_ATTACHMENT_STORE_OP_STORE: 代表你想要Vulkan保持附件的内容，以便之后使用，一般来说意味着这些内容会被取出并写入到内存里。一般场景是你想把图像展示给用户，或者之后需要读取，或者要在另一个renderpass中用作附件时，会设置此值(另一个renderpass会设置为VK_ATTACHMENT_LOAD_OP_LOAD)
- VK_ATTACHMENT_STORE_OP_DONT_CARE: 代表renderpass结束后，你不再需要附件的内容，一般来说用作深度或模板缓冲的中间产物时使用

若附件是一个深度-模板组合起来的，那么stencilLoadOp和stencilStoreOp字段会告知Vulkan如何对附件中模板的部分如何操作(loadOp和storeOp定义了附件的深度部分的操作)，这与深度部分的操作是不同的。

initialLayout和finalLayout字段告诉Vulkan，当renderpass开始和结束时，图像的期望布局。注意renderpass对象不会自动的把图像移动到初始布局中。这只是图像在renderpass被使用时，所期望的布局。然后，最终布局确实是由renderpass结束时进行移动的。

你可以使用栅栏(barrier)来显式的把图像从一种布局移动到另一种布局，然而可能的话，最好在renderpass里面做图像布局的移动。这给了Vulkan最大的机会来为每个renderpass部分选择正确的布局，并且甚至用其他渲染来并行的把图像在布局之间移动。这些字段和renderpass的进阶使用方法，总体会在第13章 多通道渲染里介绍。

定义了所有会在renderpasss中使用的附件后，你需要定义所有的子通道，每个子通道引用了一定数量的附件(即你传入到pAttachments的数组)作为输入和输出，这些描述是用VkSubPassDescription结构体的数组来设定，每个结构体对应renderpass中的一个子通道。VkSubpassDescription的定义如下:

```c
typedef struct VkSubpassDescription {
    VkSubpassDescriptionFlags flags;
    VkPipelineBindPoint pipelineBindPoint;
    uint32_t inputAttachmentCount;
    const VkAttachmentReference* pInputAttachments;
    uint32_t colorAttachmentCount;
    const VkAttachmentReference* pColorAttachments;
    const VkAttachmentReference* pResolveAttachments;
    const VkAttachmentReference* pDepthStencilAttachment;
    uint32_t preserveAttachmentCount;
    const uint32_t* pPreserveAttachments;
} VkSubpassDescription;
```

### 待补1

## 帧缓冲(FrameBuffer)

framebuffer代表了图形管线会渲染出的一组图像，这些framebuffer影响到管线的最后几个阶段，比如深度，模板测试，混色，逻辑操作，多重采样等。帧缓冲对象可用renderpass的引用来创建的，并且可用于任何有着相同附件组织的renderpass中。

调用vkCreateFrameBuffer()来创建帧缓冲对象

```c
VkResult vkCreateFramebuffer (
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFramebuffer* pFramebuffer
);
```

### 待补2

## 创建一个简单的图形管线

创建一个图形管线与第六章提到的创建计算管线类似，然而，你可看到，图形管线包含了许多着色阶段和固定功能的处理区块，所以图形管线的描述对应的就更加复杂。图形管线通过调用vkCreateGraphicsPipelines()创建，函数的原型如下:

```c
VkResult vkCreateGraphicsPipelines (
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines
);
```

如你所见，此函数的原型与创建计算管线的很类似，都取了device, 管线缓存pipelineCache,和多个createInfo结构体的数组，及其数量(对应pCreateInfos, createInfoCount)。VkGraphicsPipelineCreateInfo是一个又大又复杂的结构体，其包含了指向了多个其他结构体及其句柄的指针，深呼吸一口，其定义如下:

```c
typedef struct VkGraphicsPipelineCreateInfo {
    VkStructureType sType; // 固定值VK_GRAPHICS_PIPELINE_CREATE_INFO
    const void* pNext; // 没有扩展就是nullptr
    VkPipelineCreateFlags flags; // 此管线会被如何使用，有哪些值取决于vulkan版本
    uint32_t stageCount;
    const VkPipelineShaderStageCreateInfo* pStages;
    const VkPipelineVertexInputStateCreateInfo* pVertexInputState;
    const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
    const VkPipelineTessellationStateCreateInfo* pTessellationState;
    const VkPipelineViewportStateCreateInfo* pViewportState;
    const VkPipelineRasterizationStateCreateInfo* pRasterizationState;
    const VkPipelineMultisampleStateCreateInfo* pMultisampleState;
    const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState;
    const VkPipelineColorBlendStateCreateInfo* pColorBlendState;
    const VkPipelineDynamicStateCreateInfo* pDynamicState;
    VkPipelineLayout layout;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
} VkGraphicsPipelineCreateInfo;
```

之前提醒过你了，这个结构体又大又复杂，其内部具有多个子结构体引用的指针，然而，拆分成多快就会变得简单，并且许多额外的创建信息是可选的，可被设为nullptr,就如同其他Vulkan的createInfo结构体一样，此结构体也用sType字段打头，也有pNext字段，sType是VK_GRAPHICS_PIPELINE_CREATE_INFO, pNext是nullptr(除非使用了扩展)。

flags字段包含了这个管线会被如何使用的信息，这些标志，由当前版本的Vulkan定义，如下所示:

- VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT: 告知Vulkan此管线不会被使用在性能优先的应用中，并且比起Vulkan花大把时间优化此管线，你更偏向于快速返回一个预备好的管线对象。若你用来显示启动画面或快速显示UI元素时可以使用
- VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT和VK_PIPELINE_CREATE_DERIVATIVES_BIT: 与派生的管线一起是用。这种特性让你能把相似的管线分组，并且你会在这些管线之间快速切换,第一个标志告知Vulkan你想创建一个新的派生管线，第二个标志告知Vulkan此管线是一个(派生)管线

## 图形着色器的阶段

后续两个字段：stageCount和pStages，是你把着色器传入到管线的位置，pStage指向stageCount数量的VKPipelineShaderStageCreateInfo结构体的指针，每个结构体描述了其中一个着色阶段。此结构体与计算管线中的VkComputePipelineCreateInfo相同，只是在这里它是一个数组。VkPipelineShaderStageCreateInfo的定义如下:

```c
typedef struct VkPipelineShaderStageCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineShaderStageCreateFlags flags;
    VkShaderStageFlagBits stage;
    VkShaderModule module;
    const char* pName;
    const VkSpecializationInfo* pSpecializationInfo;
} VkPipelineShaderStageCreateInfo;
```

所有图形管线至少包含一个顶点着色器，并且顶点着色器总是管线中首个着色阶段。因此pStage应该指向一个描述了顶点着色的VkPipelineShaderStageCreateInfo。此结构体内部的参数也需要名副其实。module字段应该是一个着色器模块，其至少包含了一个顶点着色器，pName应该是一个顶点着色器的入口的名字

由于在我们的简单管线中，我们不会使用绝大部分的图形管线阶段，我们可以把大部分其他字段设置为默认值或nullptr，layout字段与VkComputePipelineCreateInfo结构体一样，用来设定管线中使用的资源的布局

renderpass可设置成我们之前创建的renderpass对象的句柄，此renderpass只有一个子通道，故我们可以设置subpass为0，代码7.2展示了创建图形管线的最小示例，其只含有一个顶点着色器。这个代码看着挺长，其中的结构体会在后文解释

```c
VkPipelineShaderStageCreateInfo shaderStageCreateInfo =
{
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
    nullptr, // pNext
    0, // flags
    VK_SHADER_STAGE_VERTEX_BIT, // stage
    module, // module
    "main", // pName
    nullptr // pSpecializationInfo
};
```

```c
static const
VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
{
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sTypenullptr, // pNext
    0, // flags
    0, //
    vertexBindingDescriptionCount
    nullptr, // pVertexBindingDescriptions
    0, //
    vertexAttributeDescriptionCount
    nullptr // pVertexAttributeDescriptions
};
```

```c
static const
VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
{
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,// sType
    nullptr, // pNext
    0, // flags
    VK_PRIMITIVE_TOPOLOGY_POINT_LIST, // topology
    VK_FALSE // primitiveRestartEnable
};
```

```c
static const
VkViewport dummyViewport =
{
    0.0f, 0.0f, // x, y
    1.0f, 1.0f, // width, height
    0.1f, 1000.0f // minDepth, maxDepth
}
```

```c
static const
VkRect2D dummyScissor =
{
    { 0, 0 }, // offset
    { 1, 1 } // extent
};
```

```c
static const
VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
{
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // sType
    nullptr, // pNext
    0, // flags
    1, // viewportCount
    &dummyViewport, // pViewports
    1, // scissorCount
    &dummyScissor // pScissors
};
```

```c
static const
VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
{
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
    nullptr, // pNext
    0, // flagsVK_FALSE, // depthClampEnable
    VK_TRUE, // rasterizerDiscardEnable
    VK_POLYGON_MODE_FILL, // polygonMode
    VK_CULL_MODE_NONE, // cullMode
    VK_FRONT_FACE_COUNTER_CLOCKWISE, // frontFace
    VK_FALSE, // depthBiasEnable
    0.0f, // depthBiasConstantFactor
    0.0f, // depthBiasClamp
    0.0f, // depthBiasSlopeFactor
    0.0f // lineWidth
};
```

```c
static const
VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
{
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, // sType
    nullptr, // pNext
    0, // flags
    1, // stageCount
    &shaderStageCreateInfo, // pStages
    &vertexInputStateCreateInfo, // pVertexInputState
    &inputAssemblyStateCreateInfo, // pInputAssemblyState
    nullptr, // pTessellationState
    &viewportStateCreateInfo, // pViewportState
    &rasterizationStateCreateInfo, // pRasterizationState
    nullptr, // pMultisampleState
    nullptr, // pDepthStencilState
    nullptr, // pColorBlendState
    nullptr, // pDynamicState
    VK_NULL_HANDLE, // layout
    renderpass, // renderPass
    0, // subpass
    VK_NULL_HANDLE, // basePipelineHandle
    0, // basePipelineIndex
};
```

```c
result = vkCreateGraphicsPipelines(device,
    VK_NULL_HANDLE,
    1,
    &graphicsPipelineCreateInfo,
    nullptr,
&pipeline);
```

当然，大部分时候，你使用的图形管线不会只有一个顶点着色器。如同本章前文所介绍的那样，你可以在图形管线中使用多达5个着色器阶段。这些阶段如下:

- `顶点着色器`: 使用VK_SHADER_STAGE_VERTEX_BIT设定，同一时刻处理一个顶点，并且把其传入管线的下一个逻辑阶段
- `曲面细分控制着色器(TCS)`: 使用VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT设定，同一时刻处理一个控制点，但是可访问所有组成此patch的数据。也可视作一个patch着色器，其产出细分因子，并且与patch相关的per-patch数据

    > `patch`: 由多个控制点组成，比如贝塞尔曲线可以由4个控制点定义
    >
    > `细分因子`： 决定了patch被细分的密度,若值较大，细分越多，曲面更平滑，反之越粗糙，但可减少计算量
    >
    > `per-patch数据`: 这些数据是针对整个patch的，而不是单独控制点的，比如整体法线方向，纹理坐标映射方式等

- `曲面细分计算着色器(TES)`: 用VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT设定，同一时刻处理一个曲面细分后的顶点，在许多应用中，正如其名，其计算每个点的patch函数，并且可访问被TCS产出的整个patch数据
- `几何着色器`: 使用VK_SHADER_STAGE_GEOMETRY_BIT设定，每个图元执行一次，并且可产出或丢弃新的图元，也可改变图元的类型
- `片段着色器`: 使用VK_SHADER_STAGE_FRAGMENT_BIT设定，在栅格化后，每个片段执行一次，主要负责计算每个像素的最终颜色

大多数简单的渲染中至少会包含顶点和片段着色器，每个着色阶段能从前一个阶段消耗或传递数据到下个阶段。在某些情况下，某个着色器的输入是由固定功能块提供的，某些时候着色器输出的数据会被固定功能块消耗。不论数据的来龙去脉，定义着色器的输入和输出的方式都是相同的。

> 着色器（Shader）之间的数据传递遵循统一的声明方式

在SPIR-V中的着色器声明一个输入，变量必须用Input修饰，同理，输出变量使用Output修饰。与GLSL不同的是，SPIR-V中，有特殊目的的输入输出参数没有预定义的名称，而是用它们的目的来修饰。然后你用GLSL写了着色器，并用GLSL编译器编译成SPIR-V。编译器会识别出访问内置变量，并且把其翻译为恰当的声明，并且把这些输入输出变量在SPIR-V着色器的结果中进行修饰

## 顶点输入状态

为了渲染真实的几何体，你需要把数据从管线开头传入进去。你可使用由SPIR-V提供的顶点和实例的索引来编程式的生成几何体，或者显式的从某个缓冲中获取几何体数据。又或者，你可以对美村中的几何数据布局做出描述，然后让Vulkan返回给你，并把其直接提供给你的着色器。

为了实现此功能，我们使用VkGraphicsPipelineCreateInfo里的pVertexInputState成员来实现。此成员是一个指向VkPipelineVertexInputStateCreateInfo结构体的指针，其定义如下:

```c
typedef struct VkPipelineVertexInputStateCreateInfo {
    VkStructureType sType; // VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    const void* pNext; // nullptr
    VkPipelineVertexInputStateCreateFlags flags; // 预留字段,设置为0
    uint32_t vertexBindingDescriptionCount; // 顶点绑定的数量
    const VkVertexInputBindingDescription* pVertexBindingDescriptions; //顶点绑定描述的数组指针
    uint32_t vertexAttributeDescriptionCount;
    const
    VkVertexInputAttributeDescription* pVertexAttributeDescriptions;
} VkPipelineVertexInputStateCreateInfo;
```

此结构体也是用sType, pNext字段打头。应被设置为VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO和nullptr, flags字段是预留字段，应设为0

顶点的输入状态被分为两个部分，顶点绑定和顶点属性：

- `顶点的绑定`： 用于绑定包含顶点数据的缓冲
- `顶点的属性`： 描述顶点数据在缓冲里的布局方式

绑定到顶点缓冲绑定点的缓冲某些时候被视为顶点缓冲。但是一般不会用顶点缓冲这个词称呼它，某种意义上，任何缓冲都能存储顶点数据，并且单个缓冲可以存储顶点数据，也可以存储其他类型的数据。用于存储顶点数据的缓冲的唯一要求是，其必须是使用VK_BUFFER_USAGE_VERTEX_BUFFER_BIT标志创建的。

vertexBindingDescriptionCount是被管线使用的顶点绑定的数量，pVertexBindingDescriptions是指向多个VkVertexInputBindingDescription结构体数组的指针，灭个结构体描述了其中一个绑定，其定义如下:

```c
typedef struct VkVertexInputBindingDescription {
    uint32_t binding; // 绑定的索引值，多个绑定不需要索引值连续
    uint32_t stride; // 
    VkVertexInputRate inputRate;
} VkVertexInputBindingDescription;
```

binding字段是此结构体描述的绑定的索引，每个管线可以定位多个顶点缓冲绑定，并且它们的索引不需要是相邻的。没必要在某个管线里描述每一个绑定，只需要每个每个绑定都有其描述即可

被VkVertexInputBinddingDescription声明的最后一个绑定索引必须小于设备所支持的绑定最大数量。这个限制至少大于16，但是对于某些设备，可能比这个值高。若你不需要大于16个绑定，那就没必要检查这个限制值。然而，你可以使用在VkPhysicalDeviceLimits结构体里的maxVertexInputBindings，决定最高绑定索引，此结构体可通过调用vkGetPhysicalDeviceProperties()获取

### 待补3