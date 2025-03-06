# C++ Misc things

杂项精选

言简意赅，尽量一看就懂

## 1. 是否使用noexcep声明?

### 这些情况可以考虑使用，其他不考虑

- 底层轮子，抠性能
- 调用频繁，若有报错需要终止并暴露
- 函数代码变更少，值得做优化

ref: [chen3feng的回答](https://www.zhihu.com/question/30950837)

## 2. 函数后面const的场景

`含义`: 该成员函数不会修改对象成员变量，且可被const对象调用
`原理`: 编译器将 const 成员函数中的 this 指针视为 const T* 类型，指向的对象不可修改（除非成员用 mutable 修饰）
`使用场景`:

- const对象，只能调用const成员函数

```C++
const MyClass obj;
obj.getValue(); // 必须声明为 void getValue() const;
```

- 表明函数仅读取数据，增强代码可读性和安全性,如各种get方法

```C++
int getSize() const { return size_; } // 明确不会修改成员
```

`注意`:

1. mutable 成员变量可在 const 函数中被修改（如缓存、计数器）
2. const 函数内部只能调用其他 const 成员函数（避免间接修改状态)
3. 代码编写麻烦，比如函数有时要写const和非const两个版本或者需要把const版本嵌套在非const版本里并用const_cast转换
4. 设计要求高，若基类声明了const函数，且有众多派生类，派生类重载后发现需要修改成员，就需要修改基类，修改范围扩大