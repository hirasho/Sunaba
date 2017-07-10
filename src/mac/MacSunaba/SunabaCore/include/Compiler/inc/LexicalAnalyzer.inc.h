#include "Compiler/Token.h"
#include "Base/Tank.h"

namespace Sunaba{

//コンパイラの一部
inline bool LexicalAnalyzer::process(
Array<Token>* out, 
std::wostringstream* messageStream,
const wchar_t* in,
int inSize,
const wchar_t* filename,
int lineStart){
	LexicalAnalyzer analyzer(messageStream, filename, lineStart);
	return analyzer.process(out, in, inSize);
}

inline LexicalAnalyzer::LexicalAnalyzer( 
std::wostringstream* messageStream,
const wchar_t* filename,
int lineStart) :
mFilename(filename), 
mLine(1),
mLineStart(lineStart),
mMessageStream(messageStream){
}

inline LexicalAnalyzer::~LexicalAnalyzer(){
	mMessageStream = 0;
}

//最後に改行があると仮定して動作する。前段で改行文字を後ろにつけること。
/*
0:出力直後
1:行頭
3:!
4:<
5:>
6:名前
7:""の中
8:-

0,' ',0
0,'\n',1
0,=,2
0,!,3
0,<,4
0,>,5
0,c,1字トークンなら出力して0、その他なら6
0,",7

1,'\n,1 トークン出力
1,' ',1 空白カウント+
1,*,0 1字戻す

3,'=',0 !=出力
3,*,0 1字戻す

4,'=',0 <=出力
4,*,0 1字戻す

5,'=',0 >=出力
5,*,0 1字戻す

6,c,6 継続
6,*,0 出力して1字戻す

7,",1
7,*,7

8,'>', ->出力
8,*,0 1字戻す

*/
inline bool LexicalAnalyzer::process(
Array<Token>* out,
const wchar_t* in,
int inSize){
	STRONG_ASSERT(in[inSize - 1] == L'\n');
	Tank<Token> tmp;

	int s = inSize;
	int mode = MODE_LINE_BEGIN;
	int begin = 0; //tokenの開始点
	int literalBeginLine = 0;
	int i = 0;
	while (i < s){
		bool advance = true;
		wchar_t c = in[i];
		int l = i - begin; //現トークンの文字列長
		switch (mode){
			case MODE_NONE:
				if (Token::isOneCharacterDefinedType(c)){
					tmp.add()->set(in + i, 1, TOKEN_UNKNOWN, mLine);
				}else if (c == L'-'){ //-が来た
					mode = MODE_MINUS; //判断保留
				}else if (c == L'!'){ //!が来た
					mode = MODE_EXCLAMATION; //保留
				}else if (c == L'<'){ //<が来た
					mode = MODE_LT; //保留
				}else if (c == L'>'){ //>が来た
					mode = MODE_GT; //保留
				}else if (isInName(c)){ //識別子か数字が始まりそう
					mode = MODE_NAME;
					begin = i; //ここから
				}else if (c == L'"'){ //"が来た
					mode = MODE_STRING_LITERAL;
					begin = i + 1; //次の文字から
					literalBeginLine = mLine;
				}else if (c == L'\n'){
					mode = MODE_LINE_BEGIN;
					begin = i + 1;
				}else if (c == L' '){
					; //何もしない
				}else{
					beginError();
					*mMessageStream << L"Sunabaで使うはずのない文字\"" << c << L"\"が現れた。"; 
					if (c == L';'){
						*mMessageStream << L"C言語と違って文末の;は不要。";
					}else if ((c == L'{') || (c == L'}')){
						*mMessageStream << L"C言語と違って{や}は使わない。";
					}
					*mMessageStream << std::endl;
					return false;
				}
				break;
			case MODE_LINE_BEGIN:
				if (c == L' '){ //スペースが続けば継続
					;
				}else if (c == L'\n'){ //改行されれば仕切り直し。空白しかないまま改行ということは、前の行は無視していい。出力しない。
					begin = i + 1;
				}else{
					//トークンを出力して、MODE_NONEでもう一度回す。第二引数のlはスペースの数で重要。
					tmp.add()->set(0, l, TOKEN_LINE_BEGIN, mLine);
					mode = MODE_NONE;
					advance = false;
				}
				break;
			case MODE_EXCLAMATION:
				if (c == L'='){
					tmp.add()->setOperator(OPERATOR_NE, mLine);
					mode = MODE_NONE;
				}else{
					beginError();
					*mMessageStream << L"\'!\'の後は\'=\'しか来ないはずだが、\'" << c << L"\'がある。\"!=\"と間違えてないか？" << std::endl;
					return false;
				}
				break;
			case MODE_LT:
				if (c == L'='){
					tmp.add()->setOperator(OPERATOR_LE, mLine);
					mode = MODE_NONE;
				}else{ //その他の場合<を出力して、MODE_NONEでもう一度回す
					tmp.add()->setOperator(OPERATOR_LT, mLine);
					mode = MODE_NONE;
					advance = false;
				}
				break;
			case MODE_GT:
				if (c == L'='){
					tmp.add()->setOperator(OPERATOR_GE, mLine);
					mode = MODE_NONE;
				}else{ //その他の場合>を出力して、MODE_NONEでもう一度回す
					tmp.add()->setOperator(OPERATOR_GT, mLine);
					mode = MODE_NONE;
					advance = false;
				}
				break;
			case MODE_NAME:
				if (isInName(c)){ //識別子の中身。
					; //継続
				}else{ //他の場合、全て出力
					TokenType type = tmp.add()->set(in + begin, l, TOKEN_UNKNOWN, mLine);
					if (type == TOKEN_UNKNOWN){
						beginError();
						*mMessageStream << L"解釈できない文字列( ";
						mMessageStream->write(in + begin, l) << L" )が現れた。" << std::endl;
						return false;
					}else if (type == TOKEN_LARGE_NUMBER){ //大きすぎる数
						beginError();
						*mMessageStream << L"数値はプラスマイナス" << getMaxImmS(IMM_BIT_COUNT_I) << L"までしか書けない。" << std::endl;
						return false;
					}
					//もう一度回す
					mode = MODE_NONE;
					advance = false;
				}
				break;
			case MODE_STRING_LITERAL:
				if (c == L'"'){ //終わった
					tmp.add()->set(in + begin, l, TOKEN_STRING_LITERAL, mLine);
					mode = MODE_NONE;
				}else{
					; //継続
				}
				break;
			case MODE_MINUS:
				if (c == L'>'){
					tmp.add()->set(0, 0, TOKEN_SUBSTITUTION, mLine);
					mode = MODE_NONE;
				}else{ //その他の場合-を出力して、MODE_NONEでもう一度回す
					tmp.add()->setOperator(OPERATOR_MINUS, mLine);
					mode = MODE_NONE;
					advance = false;
				}
				break;
			default: ASSERT(false); break; //これはバグ
		}
		if (advance){
			if (c == L'\n'){
				++mLine;
			}
			++i;
		}
	}
	//文字列が終わっていなければ
	if (mode == MODE_STRING_LITERAL){
		beginError();
		*mMessageStream << literalBeginLine << L"行目の文字列(\"\"ではさまれたもの)が終わらないままファイルが終わった。" << std::endl;
		return false;
	}
	//配列に移す
	tmp.copyTo(out);
	return true;
}

inline void LexicalAnalyzer::beginError(){
	int p = getFilenameBegin(mFilename.pointer(), mFilename.size());
	mMessageStream->write(mFilename.pointer() + p, mFilename.size() - p);
	if (mLine != 0){
		*mMessageStream << L'(' << mLine << L") ";
	}else{
		*mMessageStream << L' ';
	}
}

} //namespace Sunaba
