#pragma once

#include <string>
#include <vector>

using namespace std;

namespace ex_p99 {
	class Stack {
	public:
		bool push(const string&);
		bool pop(string &elem);
		bool peek(string &elem);

		bool empty();
		bool full();

		int size() {
			return _stack_size();
		}

	private:
		vector<string> _stack;

		const int _stack_size() {
			return _stack.size();
		}
	};
}