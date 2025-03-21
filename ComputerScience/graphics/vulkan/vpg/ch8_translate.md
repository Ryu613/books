# 第八章 绘制

> 待补跳转:
>
> - [待补1](#待补1)
> - [待补2](#待补2)
> - [待补3](#待补3)

## 你会学到

- Vulkan中不同的绘制指令的细节
- 如何通过实例化来绘制数据的多份拷贝
- 如何通过缓冲来给绘制传递参数

绘制是Vulkan的基本操作，此操作是基于图形管线工作的。Vulkan拥有多个绘制指令，每个指令生成图像的方式略有不同。本章会深入介绍Vulkan中的绘制指令。首先，我们重新介绍一下在第七章提到的基本绘制指令，然后我们会探索索引化(indexed)和实例化(instanced)的绘制指令。最后，我们会讨论从设备内存或其生成的绘制指令中如何取回参数。

回到第七章，你已知道第一个绘制指令vkCmdDraw(), 这个指令简单地把顶点传到Vulkan的图形管线中。我们简单介绍了此指令的参数。同时也暗示还有其他绘制指令。此处提供了vkCmdDraw()的原型供参考:

```c
void vkCmdDraw (
    VkCommandBuffer commandBuffer,
    uint32_t vertexCount, // 每次绘制时顶点的数量
    uint32_t instanceCount, // 
    uint32_t firstVertex,  // 顶点绘制时从哪个顶点索引开始
    uint32_t firstInstance);
```

正如设备上执行的所有指令一样，第一个参数是VkCommandBuffer的句柄.每次绘制的顶点数量通过vertexCount设定。顶点开始绘制的索引通过firstVertex设定。送到管线中的顶点会从firstVertex开始，并根据vertexCount依次处理，然后，着色器就可从那个连续的数组区看到数据返回了。若你正直接在着色器中使用顶点索引，你会看到从firstVertex开始一个一个数上去。

## 绘制的准备

就如在第七章提到的，所有绘制是被包含在一个renderpass中的。虽然renderpass对象可以封装多个subpass，甚至简单到只绘制单张图像的渲染过程都必须是某个renderpass的一部分。根据第七章所述，renderpass通过vkCreateRenderPass()创建。为了准备渲染，我们需要调用vkCmdBeginRenderPass(),此指令会设置当前的renderpass对象，更重要的是，对要被绘制并输出的一组图像做好配置。vkCmdBeginRenderPass()的原型如下:

```c
void vkCmdBeginRenderPass (
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents contents);
```

会在renderpass里发布的指令会包含在指令缓冲中，通过commandBuffer参数设定。描述renderpass的各种参数通过VkRenderPassBeginInfo实例的指针传入，参数为pRenderPassBegin。其定义如下:

```c
typedef struct VkRenderPassBeginInfo {
    VkStructureType sType; // VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
    const void* pNext; // nullptr
    VkRenderPass renderPass; // 要开始的renderpass
    VkFramebuffer framebuffer; // 渲染到哪个帧缓冲
    VkRect2D renderArea;
    uint32_t clearValueCount;
    const VkClearValue* pClearValues;
} VkRenderPassBeginInfo;
```

sType需被设置为VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, pNext设为nullptr, 需要开始的renderpass通过renderPass参数设定，渲染到的帧缓冲通过framebuffer设定, 正如第七章所述，帧缓冲是一组图像，其会被图形指令用来渲染。

在renderpass中的任何特定用例中，我们能选择只在绑定的图像上的一小部分区域来渲染。为了实现此目标，可用renderArea来设定渲染的矩形范围，只需设定renderArea.offset.x和renderArea.offset.y为0，并且renderArea.extent.width和renderArea.extent.height为在帧缓冲里图像的宽高即可，这样Vulkan就知道你想要在帧缓冲里总体的渲染区域。

若renderpass中任何附件拥有VK_ATTACHMENT_LOAD_OP_CLEAR的加载操作，那么你想清除的颜色或值就通过VkClearValue联合体的数组来设定，此数组的指针可传入pClearValues来设定。pClearValues的元素数量通过clearValueCount来设定。其原型如下:

```c
typedef union VkClearValue {
    VkClearColorValue color;
    VkClearDepthStencilValue depthStencil;
} VkClearValue;
```

若附件是一个颜色附件，那么会使用color的值，若附件是一个深度，模板，或深度-模板附件，那么会使用depthStencil的值，对应的两个结构体定义如下:

```c
typedef union VkClearColorValue {
    float float32[4];
    int32_t int32[4];
    uint32_t uint32[4];
} VkClearColorValue;
```

```c
typedef struct VkClearDepthStencilValue {
    float depth;
    uint32_t stencil;
} VkClearDepthStencilValue;
```

### 待补1

一旦renderpass已经开始，你就可以在指令缓冲中列出绘制指令了。所有的渲染会被导向到在vkCmdBeginRenderPass()里传入的VkRenderPassBeginInfo里。对于在renderpass中结束渲染，你需要通过调用vkCmdEndRenderPass()，其原型如下:

```c
void vkCmdEndRenderPass (
    VkCommandBuffer commandBuffer);
```

vkCmdEndRenderPass()执行后，任何在renderpass中的渲染会被完成，并且帧缓冲的内容会被刷新。此刻，帧缓冲的内容就是未定义的。只有具有 VK_ATTACHMENT_STORE_OP_STORE存储操作的附件会反映出在renderpass中渲染的新生成的内容。若某个附件具有VK_ATTACHMENT_STORE_OP_DONT_CARE的存储操作，那么帧缓冲的内容就会是未定义的

## 顶点数据

如何你想使用的图形管线需要在执行绘制前拿到顶点数据，你需要绑定缓冲来指出数据的来源。当缓冲被用作顶点数据的来源时，其有时被称为顶点缓冲(vertex buffer)。被用作顶点数据的缓冲，其指令为vkCmdBindVertexBuffers()，原型如下:

```c
void vkCmdBindVertexBuffers (
    VkCommandBuffer commandBuffer, // 顶点缓冲是哪个
    uint32_t firstBinding, // 一组指令缓冲里第一个要更新的索引值
    uint32_t bindingCount, // 有几个绑定需要更新
    const VkBuffer* pBuffers, // 
    const VkDeviceSize* pOffsets);
```

绑定到哪个指令缓冲通过commandBuffer来设定。某个管线可能引用了多个顶点缓冲，vkCmdBindVertexBuffers()指令可以在绑定的一组特定指令缓冲上进行更新。首个要更新的绑定的索引通过firstBinding设定，后续要更新的绑定的数量通过bindingCount设定。若需要在不连续的顶点缓冲绑定上更新，你需要调用多次此指令

## 待补2

缓冲中数据的布局和格式是基于对应的图形管线定义的，因此，数据的格式在此处不需设定，而是通过VkGraphicsPipelineCreateInfo穿入的VkPipelineVertexInputStateCreateInfo结构体来设定，回顾第七章，我们在List 7.3中展示了一个设置了交叉间隔的顶点数据作为C++的结构体，List 8.1略有进阶，其使用了一个缓冲来存储位置数据，并且有第二个缓冲来存储每个顶点的法线和纹理坐标

```c
typedef struct vertex_t
{
vmath::vec3 normal;
vmath::vec2 texcoord;
} vertex;
static const
VkVertexInputBindingDescription vertexInputBindings[] =
{
{ 0, sizeof(vmath::vec4), VK_VERTEX_INPUT_RATE_VERTEX }, // Buffer 1{ 1, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX } // Buffer 2
};
static const
VkVertexInputAttributeDescription vertexAttributes[] =
{
{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 }, // Position
{ 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // Normal
{ 2, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(vmath::vec3) } // Tex Coord
};
static const
VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
{
VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
nullptr, // pNext
0, // flags
vkcore::utils::arraysize(vertexInputBindings), //
vertexBindingDescription-
// Count
vertexInputBindings, // pVertexBinding-
// Descriptions
vkcore::utils::arraysize(vertexAttributes), // vertexAttribute-
// DescriptionCount
vertexAttributes // pVertexAttribute-
// Descriptions
};
```

在List 8.1中，我们已定义了三个顶点属性，分布于两个缓冲。在第一个缓冲中，只有一个vec4变量，用来存储位置。此缓冲的步进也因此就是vec4的大小，即16字节。在第二个缓冲中，我们存储了顶点的交叉间隔的法线和纹理坐标。我们把其表示为一个vertex结构体，可让编译器为我们计算步进。

## 索引化的绘制

### 待补3
