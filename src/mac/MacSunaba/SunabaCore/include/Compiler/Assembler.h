#ifndef INCLUDED_SUNABA_COMPILER_ASSEMBLER_H
#define INCLUDED_SUNABA_COMPILER_ASSEMBLER_H

#include "Base/RefString.h"
#include "Machine/Instruction.h"
#include <map>
#include <sstream>

namespace Sunaba{
template<class T> class Array;
template<class T> class Tank;

class Assembler{
public:
	static bool process(
		Array<unsigned>* result,
		std::wostringstream* messageStream,
		const Array<wchar_t >& compiled);
private:
	enum TokenType{
		TOKEN_INSTRUCTION,
		TOKEN_IDENTIFIER,
		TOKEN_NUMBER,
		TOKEN_NEWLINE,
		TOKEN_LABEL_END,
		TOKEN_UNKNOWN,
	};
	enum Mode{
		MODE_SPACE, //スペース上
		MODE_STRING, //何らかの文字列。それが何であるかはToken生成時に自動判別する。
	};
	struct Label{
		Label();
		int mId; //ラベルID
		int mAddress; //ラベルのアドレス
	};
	struct Token{
		Token();
		void set(const wchar_t* s, int l, int line);

		RefString mString;
		TokenType mType;
		Instruction mInstruction;
		int mLine;
		int mNumber; //数値ならここで変換してしまう。
	};
	typedef std::map<RefString, Label> LabelNameMap;
	typedef std::map<int, const Label*> LabelIdMap;

	Assembler(std::wostringstream* messageStream);
	~Assembler();
	void tokenize(Array<Token>* out, const Array<wchar_t>& in);
	void collectLabel(const Array<Token>& in);
	bool parse(Tank<unsigned>* out, const Array<Token>& in);
	bool parseLabel(const Tank<unsigned>* out, const Array<Token>& in, int* pos);
	bool parseInstruction(Tank<unsigned>* out, const Array<Token>& in, int* pos);
	bool resolveLabelAddress(Array<unsigned>* instructions);

	LabelNameMap mLabelNameMap; //ラベル名->ラベル(実体)
	LabelIdMap mLabelIdMap; //ラベルID->ラベル(ポインタ)
	std::wostringstream* mMessageStream; //借り物
};

} //namespace Sunaba

#include "Compiler/inc/Assembler.inc.h"

#endif
