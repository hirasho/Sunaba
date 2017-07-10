#ifndef INCLUDED_SUNABA_COMPILER_STRUCTURIZER_H
#define INCLUDED_SUNABA_COMPILER_STRUCTURIZER_H

namespace Sunaba{
template<class T> class Array;
struct Token;

class Structurizer{
public:
	static bool process(
		Array<Token>* out,
		std::wostringstream* messageStream,
		Array<Token>* in); //inは破壊される。
private:
	bool process(
		Array<Token>* out,
		Array<Token>* in);
	Structurizer(std::wostringstream*);
	~Structurizer();
	void beginError(const Token&);

	std::wostringstream* mMessageStream;
};

} //namespace Sunaba

#include "Compiler/inc/Structurizer.inc.h"

#endif
