# C++ Misc things

杂项精选

简明扼要

## 1. noexcept的使用

用于声明某个函数是否能容忍异常抛出，若声明，则还有异常抛出时程序直接退出

### 使用场景

- 底层函数，需要极致性能
- 对异常0容忍，包括间接的函数

ref: [chen3feng的回答](https://www.zhihu.com/question/30950837)

## 2. 函数后面的const

### 2.1 使用

- 声明此函数允许const对象调用(不会修改成员)
- 非const对象调用时，强调此函数不会修改成员

### 2.2 场景

- 直接返回私有成员引用的get函数

### 2.3 注意

1. mutable 成员变量可在 const 函数中被修改（如缓存、计数器）
2. const 函数内部只能调用其他 const 成员函数（避免间接修改状态)
3. 代码编写麻烦，比如函数有时要写const和非const两个版本或者需要把const版本嵌套在非const版本里并用const_cast转换
4. 设计要求高，若基类声明了const函数，且有众多派生类，派生类重载后发现需要修改成员，就需要修改基类，修改范围扩大

## 3. assert

### 3.1 设计意图

代码调试时，用户需要确认某个参数不应该是非预期的情况，但是我无法完全保证不会是非预期，若确实发生此情况，应该直接报错并退出程序。

调试完毕时(不需要调试时)，这些调试代码是不需要的

### 3.2 使用

在C++中, 引入: #include <cassert>而不是<assert.h>

代码里写"assert(可以转为布尔值的表达式);"

- 若表达式为false，则会通过stderr打印错误信息，然后调用abort终止程序
- 若表达式为true，则程序继续往后运行

若引入头文件前加#define NDEBUG,则assert不会生效，一般非调试阶段要加

### 3.3 场景

一般只在调试环境使用，非调试环境一般是全局禁用assert的(也就是程序编译时视assert的代码什么都不做(性能开销为0))，引入此头文件前加#define NDEBUG即可

### 3.4 注意点

1. 注意把assert和if，exception做区分
2. 只在必要时使用assert，自己能确信不会有预期情况发生就不用，反之则用，不要滥用
3. assert本身无性能开销，但是一般只在调试阶段用
4. assert太过简单，这是有意为之，不要用其做复杂判断
5. 有时候看不出是因为什么原因报错，可以在assert上面加个注释
6. cassert和assert.h区别在于cassert多引入了个yvals_core.h,里面是C++定义的一些宏，若是C++，引入cassert，而不是assert.h

## 4. 位运算符

优点：一般来说比算术运算符的运算性能好
缺点：位运算符只能代替部分场景下的代数运算，且有时位运算符不直观，有可能看不懂是在干啥

### 4.1 使用场景

与正数里，2的幂，2的整数倍，奇偶判断，整除相关时，或一堆具有二进制状态的数据要一一做判断时，可以考虑使用位运算

1. 判断是否是2的次方
   - 如：assert(!(num & num - 1)); // num需要是2的次方(包括0，1)，否则退出
2. 清零
   - 如： config1 = num&0
3. 判断奇偶
   - 如: if(num & 1 == 0)

#### 4.2 注意点

1. 负数是用补码运算，不满足正数判断规则

## 5. 内存对齐

### 为什么要内存对齐

1. 要提高CPU从内存存取数据的性能
2. 要兼容不同系统和CPU
3. 要支持特定指令集，如SIMD
4. 要对内存使用进行跟踪和统计

对于非对齐的内存数据，不同的CPU行为不同，对对齐的要求也不同

非对齐的数据，有些CPU可以访问, 有些CPU不能访问，能访问的情况下，有些CPU会降低性能,有些不会,此处不详述

所以统一做内存对齐，有助于提高性能和兼容性，代价是浪费一些内存空间, 因为要做内存的对齐填充

### 内存存取原理

CPU为了效率，不是每个bit来存取内存，而是每次按2或4或8或16,或32字节来存取内存数据

> 例：某数据占3比特，地址是1，2，3, CPU从地址0取，每次取2字节，那么要取2次，还得剔除不要的数据并聚合后放到寄存器，就比较费时间了，尽量减少存取次数就能提升性能

### 5.1 如何内存对齐

1. 基础类型的对齐值就是其大小，即sizeof等于alignof
2. struct/class/union类型里，每个非静态成员都要对齐，对齐值是这些成员里，alignof后最大的那个值
3. 若手动设置了对齐值，就要与2里的这个值比较，若手动设置的值更小，就按这个值对齐，反之按第2里的值对齐
4. 成员对齐后，整个struct/class/union也要对齐，按3里确定后的值的整数倍对齐，多余出来的空间做填充
5. 若对象是嵌套的场景，同上

#### 手动设置对齐值的方法

1. 用#pragma pack: 需要是2的次方倍(如1，2，4，8)，默认为8
2. 用编译器的/Zp参数：等效于#pragma pack(如/Zp8等效于#pragma pack(8))

### 5.2 例子

- 使用alignof(A)获取某个类型A的对齐值
- 使用sizeof(A)获取某个类型A的占用空间大小
- 使用offsetof(A,a)获取某个类型A的成员a的偏移量
- 使用#pragma pack()设置数据打包时对齐字节数，一般用来压缩数据占用的空间，要小于对象里成员中最大的alignof值才有效

```c++
#include <cstdlib>
#include <iostream>

// 默认对齐
struct A {
 char a;// size:1, align:1, offset:0
 char* b;//size:8, align:8, offset:8
 int c; // size:4, align:4, offset:16
 double d; // size:8, align:8, offset:24
}; // alignof: 8, sizeof: 32

// 按2byte对齐
#pragma pack(2)
struct B {
 char a;// size:1, align:1, offset:0
 char* b;//size:8, align:8, offset:2
 int c; // size:4, align:4, offset:10
 double d; // size:8, align:8, offset:14
}; // alignof: 2, sizeof: 22

// 恢复默认对齐,不写的话还是2
#pragma pack()
// 测试pack大于最大align成员的场景
// 测试后发现，pack必须小于成员里最大alignof的值才有效，大于此值无效
// #pragma pack(16)
struct C {
 void* ptr; // size:8, align:8, offset:0
 A struct_a; // size: 32, align: 8, offset: 8√
 B struct_b; // size: 22, align: 2, offset: 40√ 注意还是按照2来对齐的
}; // alignof: 8√, sizeof: 62√

// windows10, 64位，visual studio 2022
int main() {
 // A
 std::cout << "A.a: alignof: "
  << alignof(char) << " sizeof: "
  << sizeof(char) << " offsetof: "
  << offsetof(A, a)
  << std::endl;
 std::cout << "A.b: alignof: "
  << alignof(char*) << " sizeof: "
  << sizeof(char*) << " offsetof: "
  << offsetof(A, b)
  << std::endl;
 std::cout << "A.c: alignof: "
  << alignof(int) << " sizeof: "
  << sizeof(int) << " offsetof: "
  << offsetof(A, c)
  << std::endl;
 std::cout << "A.d: alignof: "
  << alignof(double) << " sizeof: "
  << sizeof(double) << " offsetof: "
  << offsetof(A, d)
  << std::endl;
 std::cout << "A: alignof: "
  << alignof(A) << " sizeof: "
  << sizeof(A)
  << std::endl;
 // ==================================
 std::cout << std::endl;
 // B
 std::cout << "B.a: alignof: "
  << alignof(char) << " sizeof: "
  << sizeof(char) << " offsetof: "
  << offsetof(B, a)
  << std::endl;
 std::cout << "B.b: alignof: "
  << alignof(char*) << " sizeof: "
  << sizeof(char*) << " offsetof: "
  << offsetof(B, b)
  << std::endl;
 std::cout << "B.c: alignof: "
  << alignof(int) << " sizeof: "
  << sizeof(int) << " offsetof: "
  << offsetof(B, c)
  << std::endl;
 std::cout << "B.d: alignof: "
  << alignof(double) << " sizeof: "
  << sizeof(double) << " offsetof: "
  << offsetof(B, d)
  << std::endl;
 std::cout << "B: alignof: "
  << alignof(B) << " sizeof: "
  << sizeof(B)
  << std::endl;
 // ==================================
 std::cout << std::endl;
 // C
 std::cout << "C.ptr: alignof: "
  << alignof(void*) << " sizeof: "
  << sizeof(void*) << " offsetof: "
  << offsetof(C, ptr)
  << std::endl;
 std::cout << "C.struct_a: alignof: "
  << alignof(A) << " sizeof: "
  << sizeof(A) << " offsetof: "
  << offsetof(C, struct_a)
  << std::endl;
 std::cout << "C.struct_b: alignof: "
  << alignof(B) << " sizeof: "
  << sizeof(B) << " offsetof: "
  << offsetof(C, struct_b)
  << std::endl;
 std::cout << "C: alignof: "
  << alignof(C) << " sizeof: "
  << sizeof(C)
  << std::endl;
}
```

结果是:

```c++
A.a: alignof: 1 sizeof: 1 offsetof: 0
A.b: alignof: 8 sizeof: 8 offsetof: 8
A.c: alignof: 4 sizeof: 4 offsetof: 16
A.d: alignof: 8 sizeof: 8 offsetof: 24
A: alignof: 8 sizeof: 32

B.a: alignof: 1 sizeof: 1 offsetof: 0
B.b: alignof: 8 sizeof: 8 offsetof: 2
B.c: alignof: 4 sizeof: 4 offsetof: 10
B.d: alignof: 8 sizeof: 8 offsetof: 14
B: alignof: 2 sizeof: 22

C.ptr: alignof: 8 sizeof: 8 offsetof: 0
C.struct_a: alignof: 8 sizeof: 32 offsetof: 8
C.struct_b: alignof: 2 sizeof: 22 offsetof: 40
C: alignof: 8 sizeof: 64
```

## 6. inline

inline函数的定义写到头文件里

## 7. 模板+策略包

## 8. type_traits

## 9. std::pmr

## 10. SFINAE
