#ifndef INCLUDED_SUNABA_COMPILER_TOKEN_H
#define INCLUDED_SUNABA_COMPILER_TOKEN_H

#include "Base/RefString.h"
#include <sstream>

namespace Sunaba{
class RefString;

enum TokenType{
	//特定の文字列と対応する種類
	TOKEN_WHILE,
	TOKEN_WHILE_J, //なかぎり
	TOKEN_IF,
	TOKEN_IF_J, //なら
	TOKEN_DEF,
	TOKEN_DEF_J, //とは
	TOKEN_CONST,
	TOKEN_INCLUDE,
	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,
	TOKEN_COMMA,
	TOKEN_INDEX_BEGIN,
	TOKEN_INDEX_END,
	TOKEN_SUBSTITUTION, //->か→か⇒
	TOKEN_OUT,
	//実際の内容が様々であるような種類
	TOKEN_NAME,
	TOKEN_STRING_LITERAL, //文字列リテラル
	TOKEN_NUMBER,
	TOKEN_OPERATOR,
	//Structurizerで自動挿入される種類
	TOKEN_STATEMENT_END,
	TOKEN_BLOCK_BEGIN,
	TOKEN_BLOCK_END,
	TOKEN_LINE_BEGIN,
	//終わり
	TOKEN_END,

	TOKEN_UNKNOWN,
	TOKEN_LARGE_NUMBER, //エラーメッセージ用
};

enum OperatorType{
	OPERATOR_PLUS,
	OPERATOR_MINUS,
	OPERATOR_MUL,
	OPERATOR_DIV,
	OPERATOR_EQ, //==
	OPERATOR_NE, //!=
	OPERATOR_LT, //<
	OPERATOR_GT, //>
	OPERATOR_LE, //<=
	OPERATOR_GE, //>=

	OPERATOR_UNKNOWN,
};

struct Token{
	Token();
	TokenType set(const wchar_t* s, int l, TokenType type, int line);
	void setOperator(OperatorType, int line);

	static bool isOneCharacterDefinedType(wchar_t c);
	static TokenType getKeywordType(const wchar_t* s, int l);

	//デバグ機能
	static void toString(std::wostringstream*, const Token*, int count);
	std::wstring toString() const;
	//以下変数
	RefString mString;
	union{
		int mNumber; //数値ならここで変換して持ってしまう
		int mSpaceCount; //LINE_BEGINの場合、ここにスペースの数を入れる
	};
	TokenType mType;
	OperatorType mOperator;
	int mLine; //デバグ用行番号
	RefString mFilename; //デバグ用ファイル名(他のTokenのmStringのコピー)
private:
	void toString(std::wostringstream*) const;
	static void getOneCharacterDefinedType(TokenType*, OperatorType*, wchar_t c);

	Token(const Token&); //封印
};

} //namespace Sunaba

#include "Compiler/inc/Token.inc.h"

#endif
