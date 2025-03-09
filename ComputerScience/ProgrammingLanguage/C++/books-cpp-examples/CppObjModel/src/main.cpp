#include<Common.hpp>
#include "ClassA.hpp"
#include "MyObject.hpp"
#include "ThisPtr.hpp"

using namespace cppom;

/**
* 用sizeof观察对象占用的空间
*/
void ex1() {
	char aaa[1000] = "sadsfsads\0def";
	printf(aaa);
	// 类对象占用的空间
	ClassA a;
	int ilen = sizeof(a);
	a.char_ab = 'c';
	a.int_ab = 12;
	printf("\nilen = %d\n", ilen);
}

/**
* MyObject 的大小
*/
void ex2() {
	MyObject obj;
	int ilen = sizeof(obj);
	// 64位字节对齐为16，32位为8
	std::cout << ilen << std::endl;
}

/**
* this指针调整
*/
void ex3() {
	std::cout << sizeof(BaseA) << std::endl;
	std::cout << sizeof(BaseB) << std::endl;
	std::cout << sizeof(DeriveC) << std::endl;
	DeriveC myc;
	myc.funcA();
	myc.funcB();
	myc.funcC();
}

/**
* 拷贝构造函数的生成
*/
void ex4() {
	ClassA mya1;
	mya1.int_ab = 15;
	// 拷贝构造，但是编译器没有合成拷贝构造函数
	// 是由于编译器把int这种简单的数据类型，直接按值拷贝了，就不会生成拷贝构造函数
	// 若成员是类类型，会递归地赋值这个类的成员变量
	ClassA mya2 = mya1;
}

int main() {
	// ex1();
	// ex2();
	// ex3();
	ex4();
	return 0;
}