# 第三章 队列和指令

## 设备队列

1. 队列是设备实际执行工作的部分
2. 队列家族包含一个或多个队列，代表设备支持的某种相同的功能，包括性能，访问的内存等因素都一致
3. 若上述因素不满足，比如功能相同，但是性能不同等，还是得看作不同的队列
4. 当创建设备时，需要指定队列的数量和类型
5. 队列最主要的目的是执行工作。工作使用指令序列所表示，这些指令序列记录于指令缓冲(command buffers)
6. 指令缓冲不能直接创建，要通过指令池分配出来，指令缓冲记录了指令，最终提交到队列里执行

## 指令的记录

1. 指令通过Vulkan提供的指令函数来记录到command buffer里
2. 多线程时，需要自己保证同步性
3. 一个线程可以把指令记录到多个指令缓冲，多个线程在自己保证了同步性的前提下，也可以往同一个command buffer写入指令
4. 最好每个线程有自己的command buffer,防止多个线程共享一个command buffer造成的线程阻塞和同步问题，甚至可以每个线程一个指令池，这样具允许工作线程的指令池分配是在对应的池上，防止并发分配问题
5. 在把指令记录到指令缓冲前，你必须开启指令缓冲，用于重置状态，通过调用vkBeginCommandBuffer()完成

## 指令缓冲的回收

## 指令的提交