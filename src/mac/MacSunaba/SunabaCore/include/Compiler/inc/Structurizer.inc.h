#include "Base/Tank.h"
#include "Base/Array.h"
#include "Compiler/Token.h"

namespace Sunaba{

/*
行頭トークンにはスペースの数が入っている。
これを見つけて、「ブロック開始」「ブロック終了」「文末」の3つのトークンを挿入する。

行頭トークンを見つけたら、そのスペース数をスタックトップと比較する。
増えていればブロック開始を挿入してプッシュ、減っていればスタック
同じスペース数を発見するまでポップしつつその数だけブロック終了を挿入する。
等しければ、文末トークンを挿入する。
*/
inline bool Structurizer::process(
Array<Token>* out,
std::wostringstream* messageStream,
Array<Token>* in){
	Structurizer structurizer(messageStream);
	return structurizer.process(out, in);
}

inline Structurizer::Structurizer(std::wostringstream* messageStream) : mMessageStream(messageStream){
}

inline Structurizer::~Structurizer(){
	mMessageStream = 0;
}

//括弧で継続しているケースはここではじく。
//継続しうるのは(と[の中だけで、[や(の中であれば改行は無視して良い。
//""の中は1トークンになっているので、リテラルの中の改行は無視でき、コメントはすでに捨てられている。
inline bool Structurizer::process(
Array<Token>* out,
Array<Token>* in){
	Tank<Token> tmp;
	static const int SPACE_COUNT_STACK_DEPTH = 128;
	int spaceCountStack[SPACE_COUNT_STACK_DEPTH];
	int stackPos = 1;
	spaceCountStack[0] = 0; //ダミーで一個入れておく
	int n = in->size();
	int bracketLevel = 0; //これが0の時しか処理しない。(で+1、)で-1。
	int indexLevel = 0; //これが0の時しか処理しない。[で+1、]で-1。
	bool statementExist = false; //空行の行末トークンを生成させないためのフラグ。
	Token* prevT = 0;
	for (int i = 0; i < n; ++i){
		Token* t = &((*in)[i]);
		if (t->mType == TOKEN_LEFT_BRACKET){
			++bracketLevel;
		}else if (t->mType == TOKEN_RIGHT_BRACKET){
			--bracketLevel;
			if (bracketLevel < 0){ //かっこがおかしくなった！
				beginError(*t);
				*mMessageStream << L")が(より多い。" << std::endl;
				return false;
			}
		}else if (t->mType == TOKEN_INDEX_BEGIN){
			++indexLevel;
		}else if (t->mType == TOKEN_INDEX_END){
			--indexLevel;
			if (indexLevel < 0){ //かっこがおかしくなった！
				beginError(*t);
				*mMessageStream << L"]が[より多い。" << std::endl;
				return false;
			}
		}
		if (t->mType == TOKEN_LINE_BEGIN){ //行頭トークン発見
			//前のトークンが演算子か判定
			bool prevIsOp = false;
			if (prevT && ((prevT->mType == TOKEN_OPERATOR) || (prevT->mType == TOKEN_SUBSTITUTION))){
				prevIsOp = true;
			}
			if ((bracketLevel == 0) && (indexLevel == 0) && !prevIsOp){ //()[]の中になく、前が演算子や代入でない場合は処理。それ以外は読み捨てる。
				int newCount = t->mSpaceCount;
				int oldCount = spaceCountStack[stackPos - 1];
				if (newCount > oldCount){ //増えた
					if (stackPos >= SPACE_COUNT_STACK_DEPTH){
						beginError(*t);
						*mMessageStream << L"ブロックが深すぎる。一体どんなプログラムを書いたんだ？" << std::endl;
						return false;
					}
					spaceCountStack[stackPos] = newCount;
					++stackPos;
					tmp.add()->set(0, 0, TOKEN_BLOCK_BEGIN, t->mLine);
				}else if (newCount == oldCount){ //前の文を終了
					if (statementExist){ //中身が何かあれば
						tmp.add()->set(0, 0, TOKEN_STATEMENT_END, t->mLine);
						statementExist = false;
					}
				}else{ //newCount < oldCount){ ブロック終了トークン挿入
					if (statementExist){ //中身が何かあれば文末
						tmp.add()->set(0, 0, TOKEN_STATEMENT_END, t->mLine);
						statementExist = false;
					}
					while (newCount < oldCount){
						--stackPos;
						ASSERT(stackPos >= 1); //ありえない
						oldCount = spaceCountStack[stackPos - 1];
						tmp.add()->set(0, 0, TOKEN_BLOCK_END, t->mLine);
					}
					if (newCount != oldCount){ //ずれてる
						beginError(*t);
						*mMessageStream << L"字下げが不正。ずれてるよ。前の深さに合わせてね。" << std::endl;
						return false;
					}
				}
			}
		}else{ //その他であればそのトークンをそのまま挿入
			tmp.add(*t);
			statementExist = true;
		}
		prevT = t;
	}
	//もし最後に何かあるなら、最後の文末を追加
	int lastLine = (*in)[n - 1].mLine;
	if (statementExist){
		tmp.add()->set(0, 0, TOKEN_STATEMENT_END, lastLine);
	}
	//ブロック内にいるならブロック終了を補う
	while (stackPos > 1){
		--stackPos;
		tmp.add()->set(0, 0, TOKEN_BLOCK_END, lastLine);
	}
	tmp.copyTo(out);
	return true;
}

inline void Structurizer::beginError(const Token& t){
	mMessageStream->write(t.mFilename.pointer(), t.mFilename.size());
	if (t.mLine != 0){
		*mMessageStream << L'(' << t.mLine << L") ";
	}else{
		*mMessageStream << L' ';
	}
}

} //namespace Sunaba

