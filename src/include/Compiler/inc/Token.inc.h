#include "Base/RefString.h"
#include "Machine/Instruction.h"
#include "Localization.h"

namespace Sunaba{

inline Token::Token() : 
mNumber(0), 
mType(TOKEN_UNKNOWN), 
mOperator(OPERATOR_UNKNOWN),
mLine(0),
mFilename(0){
}

inline TokenType Token::set(const wchar_t* s, int l, TokenType type, int line, const Localization& loc){
	mString.set(s, l);
	mLine = line;
	if ( //自動挿入トークン(Structurizerからしか呼ばれない)
	(type == TOKEN_BLOCK_BEGIN) ||  //自動挿入トークン(Structurizerからしか呼ばれない)
	(type == TOKEN_BLOCK_END) ||  //自動挿入トークン(Structurizerからしか呼ばれない)
	(type == TOKEN_STATEMENT_END) || //自動挿入トークン(Structurizerからしか呼ばれない)
	(type == TOKEN_END) || //ファイル終端ダミー
	(type == TOKEN_SUBSTITUTION)){ //->から来る代入
		mType = type;
	}else if (type == TOKEN_UNKNOWN){
		TokenType tokenType = TOKEN_UNKNOWN;
		if (l == 1){ //1文字識別トークンなら試す
			OperatorType operatorType;
			getOneCharacterDefinedType(&tokenType, &operatorType, s[0]);
			if (tokenType != TOKEN_UNKNOWN){
				mType = tokenType;
				mOperator = operatorType;
			}
		}
		if (tokenType == TOKEN_UNKNOWN){ //その他文字列
			if (convertNumber(&mNumber, s, l)){ //数字か？
				if (abs(mNumber) <= getMaxImmS(IMM_BIT_COUNT_I)){
					mType = TOKEN_NUMBER;
				}else{
					mType = TOKEN_LARGE_NUMBER;
				}
			}else{ //数字じゃない場合、
				mType = getKeywordType(s, l, loc);
				if (mType == TOKEN_UNKNOWN){
					mType = TOKEN_NAME; //わからないので名前
				}
			}
		}
	}else if (type == TOKEN_LINE_BEGIN){
		mType = TOKEN_LINE_BEGIN;
		mSpaceCount = l; //スペースの数
	}else if (type == TOKEN_STRING_LITERAL){
		mType = TOKEN_STRING_LITERAL;
	}else{
		ASSERT(false); //ここはバグ。
	}
	return mType;
}

inline void Token::setOperator(OperatorType operatorType, int line){  //演算子専用
	mType = TOKEN_OPERATOR;
	mOperator = operatorType;
	mLine = line;
}

inline bool Token::isOneCharacterDefinedType(wchar_t c){ //1文字でタイプが確定する場合ならそのタイプを返す。
	TokenType tokenType;
	OperatorType operatorType;
	getOneCharacterDefinedType(&tokenType, &operatorType, c);
	return (tokenType != TOKEN_UNKNOWN);
}

//1文字で判定出来るタイプ
inline void Token::getOneCharacterDefinedType(TokenType* tokenType, OperatorType* operatorType, wchar_t c){ //1文字でタイプが確定する場合ならそのタイプを返す。
	*tokenType = TOKEN_UNKNOWN;
	*operatorType = OPERATOR_UNKNOWN;
	switch (c){
		case L'(': *tokenType = TOKEN_LEFT_BRACKET; break;
		case L')': *tokenType = TOKEN_RIGHT_BRACKET; break;
		case L'[': *tokenType = TOKEN_INDEX_BEGIN; break;
		case L']': *tokenType = TOKEN_INDEX_END; break;
		case L',': *tokenType = TOKEN_COMMA; break;

		case L'+': *tokenType = TOKEN_OPERATOR; *operatorType = OPERATOR_PLUS; break;
		case L'*': *tokenType = TOKEN_OPERATOR; *operatorType = OPERATOR_MUL; break;
		case L'/': *tokenType = TOKEN_OPERATOR; *operatorType = OPERATOR_DIV; break;
		case L'=': *tokenType = TOKEN_OPERATOR; *operatorType = OPERATOR_EQ; break;
	}
}

inline void Token::toString(std::wostringstream* s, const Token* t, int count){
	int level = 0;
	for (int i = 0; i < count; ++i){
		if (t[i].mType == TOKEN_BLOCK_END){
			--level;
		}
		for (int j = 0; j < level; ++j){
			*s << '\t';
		}
		t[i].toString(s);
		*s << std::endl;
		if (t[i].mType == TOKEN_BLOCK_BEGIN){
			++level;
		}
	}
}

inline std::wstring Token::toString() const{
	std::wostringstream s;
	toString(&s);
	return s.str();
}

//デバグ用なので日本語のみ対応
inline void Token::toString(std::wostringstream* s) const{
	if (mType == TOKEN_OPERATOR){
		switch (mOperator){
			case OPERATOR_PLUS: *s << L"+"; break;
			case OPERATOR_MINUS: *s << L"-"; break;
			case OPERATOR_MUL: *s << L"×"; break;
			case OPERATOR_DIV: *s << L"÷"; break;
			case OPERATOR_EQ: *s << L"="; break;
			case OPERATOR_NE: *s << L"≠"; break;
			case OPERATOR_LT: *s << L"<"; break;
			case OPERATOR_GT: *s << L">"; break;
			case OPERATOR_LE: *s << L"≦"; break;
			case OPERATOR_GE: *s << L"≧"; break;
			default: ASSERT(false); break;
		}
	}else if (mType == TOKEN_LINE_BEGIN){
		*s << L"行開始(" << mSpaceCount << L')'; 
	}else if ((mType == TOKEN_NAME) || (mType == TOKEN_STRING_LITERAL) || (mType == TOKEN_NUMBER)){
		s->write(mString.pointer(), mString.size());
	}else{
		switch (mType){
			case TOKEN_WHILE_PRE: *s << "while"; break;
			case TOKEN_IF_PRE: *s << L"if"; break;
			case TOKEN_WHILE_POST: *s << L"なかぎり"; break;
			case TOKEN_IF_POST: *s << L"なら"; break;
			case TOKEN_DEF_PRE: *s << L"def"; break;
			case TOKEN_DEF_POST: *s << L"とは"; break;
			case TOKEN_CONST: *s << L"定数"; break;
			case TOKEN_INCLUDE: *s << L"挿入"; break;
			case TOKEN_STATEMENT_END: *s << L"行末"; break;
			case TOKEN_LEFT_BRACKET: *s << L"("; break;
			case TOKEN_RIGHT_BRACKET: *s << L")"; break;
			case TOKEN_BLOCK_BEGIN: *s << L"範囲開始"; break;
			case TOKEN_BLOCK_END: *s << L"範囲終了"; break;
			case TOKEN_INDEX_BEGIN: *s << L"["; break;
			case TOKEN_INDEX_END: *s << L"]"; break;
			case TOKEN_COMMA: *s << L","; break;
			case TOKEN_END: *s << L"ファイル終了"; break;
			case TOKEN_SUBSTITUTION: *s << L"→"; break;
			case TOKEN_OUT: *s << L"出力"; break;
			default: ASSERT(false); break;
		}
	}
}

inline TokenType Token::getKeywordType(const wchar_t* s, int l, const Localization& loc){
	RefString t(s, l);
	TokenType r = TOKEN_UNKNOWN;
	if (t == L"while"){
		r = TOKEN_WHILE_PRE;
	}else if ((t == loc.whileWord0) || (loc.whileWord1 && (t == loc.whileWord1))){ //whileWord1はないことがある
		if (loc.whileAtHead){
			r = TOKEN_WHILE_PRE;
		}else{
			r = TOKEN_WHILE_POST;
		}
	}else if (t == L"if"){
		r = TOKEN_IF_PRE;
	}else if (t == loc.ifWord){
		if (loc.ifAtHead){
			r = TOKEN_IF_PRE;
		}else{
			r = TOKEN_IF_POST;
		}
	}else if (t == L"def"){
		r = TOKEN_DEF_PRE;
	}else if (t == loc.defWord){
		if (loc.defAtHead){
			r = TOKEN_DEF_PRE;
		}else{
			r = TOKEN_DEF_POST;
		}
	}else if ((t == L"const") || (t == loc.constWord)){
		r = TOKEN_CONST;
	}else if ((t == L"include") || (t == loc.includeWord)){
		r = TOKEN_INCLUDE;
	}else if ((t == L"out") || (t == loc.outWord)){
		r = TOKEN_OUT;
	}
	return r;
}

} //namespace Sunaba
