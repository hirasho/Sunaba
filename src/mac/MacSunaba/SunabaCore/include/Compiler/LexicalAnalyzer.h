#ifndef INCLUDED_SUNABA_COMPILER_LEXICALANALYZER_H
#define INCLUDED_SUNABA_COMPILER_LEXICALANALYZER_H

#include <sstream>

namespace Sunaba{
struct Token;
struct Localization;

//コンパイラの一部
class LexicalAnalyzer{
public:
	//最後に改行があると仮定して動作する。前段で改行文字を後ろにつけること。
	static bool process(
		Array<Token>* out, 
		std::wostringstream* messageStream,
		const wchar_t* in,
		int inSize,
		const wchar_t* filename,
		int lineStart,
		const Localization&);
private:
	enum Mode{
		MODE_NONE, //出力直後
		MODE_LINE_BEGIN, //行頭
		MODE_MINUS, //-の上 (次が>かもしれない)
		MODE_EXCLAMATION, //!の上(次が=かもしれない)
		MODE_LT, //<の上(次が=かも)
		MODE_GT, //>の上(次が=かも)
		MODE_NAME, //名前[a-zA-Z0-9_全角]の上
		MODE_STRING_LITERAL, //"文字列"の上
	};
	LexicalAnalyzer(std::wostringstream* messageStream, const wchar_t* filename, int lineStart, const Localization&);
	~LexicalAnalyzer();
	bool process(Array<Token>* out, const wchar_t* in, int inSize);
	void beginError();

	RefString mFilename; //処理中のファイル名
	int mLine;
	int mLineStart;
	std::wostringstream* mMessageStream; //借り物
	const Localization* mLocalization; //借り物
};

} //namespace Sunaba

#include "Compiler/inc/LexicalAnalyzer.inc.h"

#endif
