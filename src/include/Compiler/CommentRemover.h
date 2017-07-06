#ifndef INCLUDED_SUNABA_COMPILER_COMMENTREMOVER_H
#define INCLUDED_SUNABA_COMPILER_COMMENTREMOVER_H

namespace Sunaba{

class CommentRemover{
public:
	//TODO: 末尾に改行を加えるが、本来ここでやるべきことじゃない。
	static bool process(Array<wchar_t>* out, const Array<wchar_t>& in);
};

} //namespace Sunaba

#include "Compiler/inc/CommentRemover.inc.h"

#endif
