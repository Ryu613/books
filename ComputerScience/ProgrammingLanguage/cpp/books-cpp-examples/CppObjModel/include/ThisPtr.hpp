#pragma once
#include <stdio.h>

namespace cppom {
	class BaseA {
	public:
		int a = 0;
		BaseA() {
			printf("A::A() 的 this指针是: %p!\n", this);
		}

		void funcA() {
			printf("A::funcA: this = %p\n", this);
		}
	};

	class BaseB {
	public:
		int b = 0;
		BaseB() {
			printf("B::B() 的this指针是: %p!\n", this);
		}

		void funcB() {
			printf("B::funcB: this = %p\n", this);
		}
	};

	class DeriveC : public BaseA, public BaseB {
	public:
		int c = 0;
		DeriveC() {
			printf("C::C() 的this指针是: %p!\n", this);
		}

		void funcC() {
			printf("C::funC: this = %p\n", this);
		}
	};
}