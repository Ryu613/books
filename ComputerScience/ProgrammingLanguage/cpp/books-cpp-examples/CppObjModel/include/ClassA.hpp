#pragma once

namespace cppom {
	class ClassA {
	public:
		void func() {};
		void func1() {};
		void func2() {};
		// 函数本身不占用类内存空间，但是由于虚函数会添加一个指针，虚函数指针会导致类内存+4byte(32位)或+8byte(64位)
		virtual void func3() {};

		// char类型占一个字节,sizeof此类还是1
		// 若两个变量同时存在，编译器可能做内存对齐操作，导致占用内存不是1+4=5，而是4+4=8
		char char_ab;
		// int占4
		int int_ab;
	}; // 此类占用空间:64位：4+4+8=16; 32位: 4+4+4=12
}