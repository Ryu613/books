#include "Example.hpp"
#include "p99/Stack.hpp"

// 前置声明，方便进行指针定义和作为数据类型
// 若要访问这个类的成员，就需定义此类

namespace esscpp {
	class Stack;
	void p99() {
		std::cout << "run P99" << std::endl;
		// 以下两种情况皆不需要类的定义，只需前置声明即可
		// 使用前置声明来定义定义指针
		Stack* pt = 0;
		// 使用前置声明来作为函数的数据类型
		void Process(const Stack&);
	}
}