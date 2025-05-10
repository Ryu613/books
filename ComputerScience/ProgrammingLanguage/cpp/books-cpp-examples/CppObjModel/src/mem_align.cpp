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
	B struct_b; // size: 22, align: 2, offset: 40√
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