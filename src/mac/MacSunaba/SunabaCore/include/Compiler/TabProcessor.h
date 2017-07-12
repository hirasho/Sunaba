#ifndef INCLUDED_SUNABA_COMPILER_TABPROCESSOR_H
#define INCLUDED_SUNABA_COMPILER_TABPROCESSOR_H

namespace Sunaba{
template<class T> class Array;

//タブ及び全角スペースを半角スペースに置換
class TabProcessor{
public:
	static void process(Array<wchar_t>* out, const Array<wchar_t>& in, int tabWidth = 8);
};

} //namespace Sunaba

#include "Compiler/inc/TabProcessor.inc.h"

#endif
