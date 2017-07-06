#ifndef INCLUDED_SUNABA_COMPILER_COMPILER_H
#define INCLUDED_SUNABA_COMPILER_COMPILER_H

#include <sstream>

namespace Sunaba{
template<class T> class Array;
struct Localization;

class Compiler{
public:
	static bool process(
		Array<wchar_t>* result,
		std::wostringstream* messageOut,
		const wchar_t* filename,
		const Localization&);
};

} //namespace Sunaba

#include "Compiler/inc/Compiler.inc.h"

#endif
