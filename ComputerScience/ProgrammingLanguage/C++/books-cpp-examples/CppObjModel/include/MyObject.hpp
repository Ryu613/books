#pragma once

namespace cppom {
	class MyObject {
	public:
		MyObject() {};
		virtual ~MyObject() {};
		float getValue() const {
			return m_value;
		}
		static int s_getCount() {
			return ms_scount;
		}

		virtual void vfrandfunc() {};
	protected:
		float m_value;
		static int ms_scount;
	};
	// 64位： 8+4=12
	// 32位:  4+4=8
}