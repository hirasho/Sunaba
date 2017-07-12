#ifndef INCLUDED_SUNABA_COMPILER_CHARACTERREPLACER_H
#define INCLUDED_SUNABA_COMPILER_CHARACTERREPLACER_H

namespace Sunaba{
template<class T> class Array;
struct Localization;

//危険な文字を排除し、全角->半角の置き換えをおこなったりする。サイズは減るか維持。
class CharacterReplacer{
public:
	static void process(
		Array<wchar_t>* out, 
		const Array<wchar_t>& in,
		const Localization&);
};

} //namespace Sunaba

#include "Compiler/inc/CharacterReplacer.inc.h"

#endif
