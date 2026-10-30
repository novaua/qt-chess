#pragma once
#include <CppUnitTest.h>
#include <QString>
#include <string>

// CppUnitTest can show a QString in a failed assertion's message only with this specialization; the
// Qt-side test files share it here instead of each defining its own copy.
namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<>
	inline std::wstring ToString(const QString& q) { return q.toStdWString(); }
}
