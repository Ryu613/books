# 第八章 绘制

1. vulkan有好几个绘制命令，其中vkCmdDraw()把顶点数据推送入图形管线并绘制
2. 绘制之前要先create并begin RenderPass
3. 若图形管线要求顶点数据，在绘制指令执行前，需要绑定缓冲，叫做vertex buffer,对应指令为vkCmdBindVertexBuffers(),vertex buffer就是某个command buffer