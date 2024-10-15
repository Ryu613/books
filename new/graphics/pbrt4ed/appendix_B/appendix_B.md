# 附录B 工具

## B.4 容器和内存管理

### B.4.4 标签化的指针

TaggedPointer类是pbrt处理多态性的核心。对于已知的类型，用一个指针指向它，同时利用指针中多余的比特位来编码对象准确的类型(即为对象打标签)。当需要动态分派或其他与类型相关的操作时，对象的类型就可以从指针中解出来。这个类的实现在util/taggedptr.h中

TaggedPointer是一个模板类，需要在编译期间就提供出要代表的所有对象的类型。注意，这种方法排除了在运行时加载新类型的额外类定义的可能性，而使用基于虚函数的常规多态性方法则是可以做到这一点。

> 也就是说当前方法由于设计限制，无法像基于虚函数的多态机制那样灵活地在运行时加载和使用新的类型或类定义

<<TaggedPointer的定义>>

```c++
template <typename... Ts>
class TaggedPointer {
  public:
    // <<TaggedPointer的public类型>> 
    // <<TaggedPointer Public Methods>> 
  private:
    // <<TaggedPointer的Private成员>> 
};
```

所有在TaggedPointer中可能的类型都通过一个public的类定义给出

<<TaggedPointer的public类型>>

```c++
using Types = Typepack<Ts...>
```

现代处理器几乎都是使用64位指针，这个指针允许用来定位$2^64$字节的内存地址。现在一般内存大小从几十G到几百G，远低于64位指针能够定位的几十亿级的地址数。因此，处理器只用很少数量的比特数来定义所使用的内存空间。到现在，CPU才从48位地址空间增加到57位，然而，对于单个系统来说，$2^57$字节的内存数量还是太大了，这种大型地址空间对于多机器下的集群计算很有用，可以用来表示全局统一的地址空间，或者把离线存储的数据做指针映射。

TaggedPointer因此就用了其中高位的比特数来编码对象的类型。对于57位的内存空间，也还有7比特剩余，也会允许$2^7$种类型定义，已经远高于pbrt所需。

<<TaggedPointer的Private成员>>

```c++
static constexpr int tagShift = 57;
static constexpr int tagBits = 64 - tagShift;
```

tagMask是一个位掩码，用于把类型的标签的比特解出来，ptrMask用于解出原始的指针

<<TaggedPointer的private成员>>

```c++
static constexpr uint64_t tagMask = ((1ull << tagBits) - 1) << tagShift;
static constexpr uint64_t ptrMask = ~tagMask;
```

现在我们能实现TaggedPointer的主构造器。给定一个已知类型T的指针，它使用TypeIndex()方法来为这个类型获取一个整形索引，然后，bits成员会被设置为带有类型的整形索引的原始指针，上移至指针指未使用的比特上。

<<TaggedPointer的public方法>>

```c++
template <typename T>
TaggedPointer(T *ptr) {
    uintptr_t iptr = reinterpret_cast<uintptr_t>(ptr);
    constexpr unsigned int type = TypeIndex<T>();
    bits = iptr | ((uintptr_t)type << tagShift);
}
```

<<TaggedPointer的private成员>>

```c++
uintptr_t bits = 0;
```

TypeIndex()方法的大部分工作已经用IndexOf完成了，这个方法在上一章讲了。然而，还需要一个索引用于表示空指针，所以0的索引就被用于定义这个空指针，其余的就依次加1求得。

<<TaggedPointer的public方法>>

```c++
template <typename T>
static constexpr unsigned int TypeIndex() {
    using Tp = typename std::remove_cv_t<T>;
    if constexpr (std::is_same_v<Tp, std::nullptr_t>) return 0;
    else return 1 + pbrt::IndexOf<Tp, Types>::count;
}
```

Tag()方法会从TaggedPointer中相关的比特中解出标签并返回， Is()方法用于在运行期检查某个TaggedPointer是否代表了某种特定类型

<<TaggedPointer的public方法>>

```c++
unsigned int Tag() const { return ((bits & tagMask) >> tagShift); }
template <typename T>
bool Is() const { return Tag() == TypeIndex<T>(); }
```

标签的最大值就等于代表的类型的数量

<<TaggedPointer的public方法>>

```c++
static constexpr unsigned int MaxTag() { return sizeof...(Ts); }
```

特定类型的指针用CastOrNullPtr()返回。若TaggedPointer没有持有T类型的对象，则返回nullptr。此外，TaggedPointer还提供了一个常量版本的此方法，返回常量形式的T*,同时不安全的Cast()方法总是返回给定类型的指针。这些方法只应该在TaggedPointer持有的类型确定的时候使用。

<<TaggedPointer的public方法>>

```c++
template <typename T>
T *CastOrNullptr() {
    if (Is<T>()) return reinterpret_cast<T *>(ptr());
    else return nullptr;
}
```

对于需要原始指针，但是void指针就足够了的场景，可以用ptr()方法，这个方法也有个const版。

<<TaggedPointer的public方法>>

```c++
void *ptr() { return reinterpret_cast<void *>(bits & ptrMask); }
```

TaggedPointer中最有意思的方法是Dispatch(),它是pbrt中为多态类型实现动态分派机制的核心方法。它的任务是决定TaggedPointer指向哪个类型的对象，然后调用队以ing的函数，把对象的指针传过去，转换为正确的类型(可看Spectrum::operator()方法，这个方法调用了TaggedPointer::Dispatch()，提供给Dispatch()的具体操作的函数在这里会被讨论)

大部分工作都会被独立的Dispatch()函数完成，这个函数是在detail命名空间中定义的，这样做表示虽然这个函数在头文件中定义，但是不能被头文件外部的代码使用。在detail中的那些函数需要入参里提供的函数的返回值类型，是用ReturnType这个助手模板类中决定的。在此我们不会介绍ReturnType的实现，这个类使用了C++模板包扩展，在调用此函数时，可以找到func的返回类型，若它们不是相同的，会在编译器报错，并且通过它的类型定义来提供返回的类型

<<taggedPointer的public方法>>

```c++
template <typename F>
PBRT_CPU_GPU decltype(auto) Dispatch(F &&func) {
    using R = typename detail::ReturnType<F, Ts...>::type;
    return detail::Dispatch<F, R, Ts...>(func, ptr(), Tag() - 1);
}
```

detail::Dispatch() 函数可以根据 TaggedPointer 管理的类型数量，接受任意数量的类型作为参数进行处理。为了应对不同数量的类型，这个函数通过提供多种模板特化来处理不同数量的类型参数。

pbrt早期版本中，我们实现了一种分派机制，是用二叉查找实现的，让一系列递归函数调用是基于类型的索引，直到找到对应的类型。这种做法与这里的实现方法有等效的性能，但是代码量更少。然而，我们发现这种做法把调用栈搞乱了，导致调试不方便。是用当前的方式，动态的分派只会导致一次函数调用。

作为Dispatch()的一个字里，这里是持有三种类型的的一个实现，这个函数被回调函数F的类型参数化了，并且返回了类型R。函数里只有一个switch，来基于传入TaggedPointer::Dispatch()中的索引，选择调用合适的指针类型

<<TaggedPointer的助手模板>>

```c++
template <typename F, typename R, typename T0, typename T1, typename T2>
R Dispatch(F &&func, void *ptr, int index) {
    switch (index) {
      case 0:  return func((T0 *)ptr);
      case 1:  return func((T1 *)ptr);
      default: return func((T2 *)ptr);
    }
}
```

还有其他的detail::Dispatch()实现，最多支持8种类型，如果提供更多的类型，则后备的实现会处理前8个类型，然后递归调用detail::Dispatch()来处理后面剩余的类型。对于pbrt来说，最多约有10个左右的类型，这种方式的实现运行起来还不错。

TaggedPointer也包含了一个常数类型的分派方法，也包括DispatchCPU(),这个函数对于只在CPU上运行的方法来说是必要的。(默认的Dispatch()方法要求既可在CPU，也可在GPU上运行，在pbrt中是普遍的)， 在detail命名空间中，这两种对应的分派函数都存在。
