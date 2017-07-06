#include "Compiler/TabProcessor.h"
#include "Compiler/CharacterReplacer.h"
#include "Compiler/CommentRemover.h"

namespace Sunaba{

inline bool Assembler::process(
Array<unsigned>* result,
std::wostringstream* messageStream,
const Array<wchar_t>& compiled,
const Localization& loc){
	//タブ処理
	Array<wchar_t> tabProcessed;
	TabProcessor::process(&tabProcessed, compiled);
	//読めるテキストが来たケースに備えて念のため文字置換
	Array<wchar_t> replaced;
	CharacterReplacer::process(&replaced, tabProcessed, loc);
	//コメントを削除する。行数は変えない。
	Array<wchar_t> commentRemoved;
	CommentRemover::process(&commentRemoved, replaced);
	//まずトークン分解
	Array<Token> tokens;
	Assembler assembler(messageStream);
	assembler.tokenize(&tokens, commentRemoved);
	//ラベルだけ処理します。
	assembler.collectLabel(tokens);
	//パースします。
	Tank<unsigned> tmpInst; //ここに命令を放り込む
	if (!assembler.parse(&tmpInst, tokens)){
		return false;
	}
	//出力へコピー
	tmpInst.copyTo(result);
	//分岐先アドレス解決
	if (!assembler.resolveLabelAddress(result)){
		return false;
	}
	return true;
}

inline Assembler::Assembler(std::wostringstream* messageStream) :
mMessageStream(messageStream){
}

inline Assembler::~Assembler(){
	mMessageStream = 0;
}

inline Assembler::Label::Label() : mId(-1), mAddress(-1){
}

inline Assembler::Token::Token() : 
mType(TOKEN_UNKNOWN), 
mInstruction(INSTRUCTION_UNKNOWN), 
mLine(0), 
mNumber(0){
}

inline void Assembler::Token::set(const wchar_t* s, int l, int line){
	mLine = line;
	if (l == 0){ //長さ0なら改行トークン
		mType = TOKEN_NEWLINE;
	}else if ((l == 1) && (s[0] == ':')){
		mType = TOKEN_LABEL_END;
	}else if (convertNumber(&mNumber, s, l)){
		mType = TOKEN_NUMBER;
	}else{
		mInstruction = nameToInstruction(s, l);
		if (mInstruction != INSTRUCTION_UNKNOWN){
			mType = TOKEN_INSTRUCTION;
		}else if (isAsmName(s, l)){
			mType = TOKEN_IDENTIFIER;
			mString.set(s, l);
		}
	}
}

inline void Assembler::tokenize(Array<Token>* out, const Array<wchar_t>& in){
	Tank<Token> tmp;
	int line = 0;
	int s = in.size();
	int mode = MODE_SPACE;
	int begin = 0; //tokenの開始点
	for (int i = 0; i < s; ++i){
		wchar_t c = in[i];
		int l = i - begin; //現トークンの文字列長
		switch (mode){
			case MODE_SPACE:
				if (c == L'\n'){ //改行が来たら
					//改行トークンを追加
					tmp.add()->set(0, 0, line); //長さ0なら改行
					++line;
				}else if (c == L' '){ //スペースが来たら、
					; //何もしない
				}else if (c == L':'){ //ラベル終わり
					tmp.add()->set(in.pointer() + i, 1, line);
				}else{ //スペースでも:でもない文字が来たら、何かの始まり
					mode = MODE_STRING;
					begin = i; //この文字から始まり
				}
				break;
			case MODE_STRING:
				if (c == L'\n'){ //改行が来たら
					//ここまでを出力
					ASSERT(l > 0);
					tmp.add()->set(in.pointer() + begin, l, line);
					//改行トークンを追加
					tmp.add()->set(0, 0, line); //長さ0なら改行
					mode = MODE_SPACE;
					++line;
				}else if (c == L' '){ //スペースが来たら
					//ここまでを出力
					ASSERT(l > 0);
					tmp.add()->set(in.pointer() + begin, l, line);
					mode = MODE_SPACE;
				}else if (c == L':'){ //:が来たら
					//ここまでを出力
					ASSERT(l > 0);
					tmp.add()->set(in.pointer() + begin, l, line);
					//加えて:を出力
					tmp.add()->set(in.pointer() + i, 1, line);
					mode = MODE_SPACE;
				} //else 文字列を継続
				break;
			default: ASSERT(false); break;
		}
	}
	//最後のトークンを書き込み
	int l = s - begin;
	if ((l > 0) && (mode == MODE_STRING)){
		tmp.add()->set(in.pointer() + begin, l, line);
	}
	tmp.add()->mLine = line; //ダミートークンを書き込み
	//配列に移す
	tmp.copyTo(out);
}

inline void Assembler::collectLabel(const Array<Token>& in){
	int tokenCount = in.size();
	int labelId = 0;
	for (int i = 1; i < tokenCount; ++i){
		//トークンを取り出して
		const Token& t0 = in[i - 1];
		const Token& t1 = in[i];
		if ((t0.mType == TOKEN_IDENTIFIER) && (t1.mType == TOKEN_LABEL_END)){ //ラベルだったら
			//ラベルmapにLabelを放り込む
			Label label;
			label.mId = labelId;
			++labelId;
			LabelNameMap::const_iterator it = mLabelNameMap.insert(LabelNameMap::value_type(t0.mString, label)).first;
			mLabelIdMap.insert(LabelIdMap::value_type(label.mId, &(it->second)));
			//次のトークンが存在しないか、改行である必要がある。ラベルに続けて書くのは遺法
			ASSERT((i == (tokenCount - 1)) || (in[i + 1].mType == TOKEN_NEWLINE));
		}
	}
}

inline bool Assembler::parse(Tank<unsigned>* out, const Array<Token>& in){
	int pos = 0;
	while (in[pos].mType != TOKEN_UNKNOWN){
		//トークンを取り出してみる
		TokenType t = in[pos].mType;
		//タイプに応じて構文解析
		if (t == TOKEN_IDENTIFIER){ //ラベル発見
			if (!parseLabel(out, in, &pos)){
				return false;
			}
		}else if (t == TOKEN_INSTRUCTION){ //命令発見
			if (!parseInstruction(out, in, &pos)){
				return false;
			}
		}else if (t == TOKEN_NEWLINE){ //無視
			++pos;
		}else{ //後はエラー
			*mMessageStream << in[pos].mLine << L" : 文頭にラベルでも命令でもないものがあるか、オペランドを取らない命令にオペランドがついていた。" << std::endl;
			return false;
		}
	}
	return true;
}

inline bool Assembler::parseLabel(const Tank<unsigned>* out, const Array<Token>& in, int* pos){
	//トークンを取り出して
	const Token& t = in[*pos];
	ASSERT(t.mType == TOKEN_IDENTIFIER);
	//ラベルmapから検索して、アドレスを放り込む
	LabelNameMap::iterator it = mLabelNameMap.find(t.mString);
	if (it == mLabelNameMap.end()){
		*mMessageStream << in[*pos].mLine <<
			L" : 文字列\"";
		mMessageStream->write(t.mString.pointer(), t.mString.size());
		*mMessageStream << L"\"は命令でもなく、ラベルでもないようだ。たぶん書き間違え。" << std::endl;
		return false;
	}
	ASSERT(it != mLabelNameMap.end());
	it->second.mAddress = out->size();
	++(*pos);
	//ラベル終了の:があるはず
	if (in[*pos].mType != TOKEN_LABEL_END){
		*mMessageStream << in[*pos].mLine << L" : ラベルは\':\'で終わらないといけない。" << std::endl;
		return false;
	}
	++(*pos);
	return true;
}

inline bool Assembler::parseInstruction(Tank<unsigned>* out, const Array<Token>& in, int* pos){
	//トークンを取り出して
	const Token& t = in[*pos];
	ASSERT(t.mType == TOKEN_INSTRUCTION);
	++(*pos);
	//後は命令種に応じて分岐
	Instruction inst = t.mInstruction;
	unsigned inst24 = static_cast<unsigned>(inst << 24);
	if (inst == INSTRUCTION_I){
		//オペランドが必要
		const Token& op = in[*pos];
		++(*pos);
		if (op.mType != TOKEN_NUMBER){
			*mMessageStream << in[*pos].mLine << L" : 命令iの次に数字が必要。";
			return false;
		}
		if (abs(op.mNumber) > getMaxImmS(IMM_BIT_COUNT_I)){
			*mMessageStream << in[*pos].mLine << L" : 命令iが取れる数字はプラスマイナス" << getMaxImmS(IMM_BIT_COUNT_I) << L"の範囲。";
			return false;
		}
		out->add(inst24 | (op.mNumber & getImmMask(IMM_BIT_COUNT_I)));
	}else if ((inst == INSTRUCTION_J) || (inst == INSTRUCTION_BZ) || (inst == INSTRUCTION_CALL)){
		//オペランドが必要
		const Token& op = in[*pos];
		++(*pos);
		ASSERT(op.mType == TOKEN_IDENTIFIER);
		LabelNameMap::const_iterator it = mLabelNameMap.find(op.mString);
		ASSERT(it != mLabelNameMap.end());
		int labelId = it->second.mId;
		if (labelId >= getMaxImmU(IMM_BIT_COUNT_FLOW)){
			*mMessageStream << in[*pos].mLine << L" : プログラムが大きすぎて処理できない(ラベルが" << getMaxImmU(IMM_BIT_COUNT_FLOW) << L"個以上ある)。" << std::endl;
			return false;
		}
		out->add(inst24 | labelId); //ラベルIDを仮アドレスとして入れておく。
	}else if (
	(inst == INSTRUCTION_LD) ||
	(inst == INSTRUCTION_ST) ||
	(inst == INSTRUCTION_FLD) ||
	(inst == INSTRUCTION_FST)){
		//オペランドが必要
		const Token& op = in[*pos];
		++(*pos);
		if (op.mType != TOKEN_NUMBER){
			*mMessageStream << in[*pos].mLine << L" : 命令ld,st,fld,fstの次に数字が必要。" << std::endl;
			return false;
		}
		if (abs(op.mNumber) > getMaxImmS(IMM_BIT_COUNT_LS)){
			*mMessageStream << in[*pos].mLine << L" : 命令popが取れる数字はプラスマイナス" << getMaxImmS(IMM_BIT_COUNT_LS) << L"の範囲。" << std::endl;
			return false;
		}
		out->add(inst24 | (op.mNumber & getImmMask(IMM_BIT_COUNT_LS)));
	}else if ((inst == INSTRUCTION_POP) || (inst == INSTRUCTION_RET)){
		//オペランドが必要
		const Token& op = in[*pos];
		++(*pos);
		if (op.mType != TOKEN_NUMBER){
			*mMessageStream << in[*pos].mLine << L" : 命令pop/retの次に数字が必要。" << std::endl;
			return false;
		}
		//即値は符号付き
		if (abs(op.mNumber) > getMaxImmS(IMM_BIT_COUNT_FLOW)){
			*mMessageStream << in[*pos].mLine << L" : 命令pop/retが取れる数字はプラスマイナス" << (1 << (IMM_BIT_COUNT_FLOW - 1)) << L"の範囲。" << std::endl;
			return false;
		}
		out->add(inst24 | (op.mNumber & getImmMask(IMM_BIT_COUNT_FLOW)));
	}else if ( //即値なし
	(inst == INSTRUCTION_ADD) ||
	(inst == INSTRUCTION_SUB) || 
 	(inst == INSTRUCTION_MUL) ||
	(inst == INSTRUCTION_DIV) || 
	(inst == INSTRUCTION_LT) ||
	(inst == INSTRUCTION_LE) ||
	(inst == INSTRUCTION_EQ) ||
	(inst == INSTRUCTION_NE)){
		out->add(inst24);
	}else{
		ASSERT(false); //バグ。
	}
	return true;
}

inline bool Assembler::resolveLabelAddress(Array<unsigned>* instructions){
	int instructionCount = instructions->size();
	int mask = (0xffffffff << IMM_BIT_COUNT_FLOW); //命令マスク
	for (int i = 0; i < instructionCount; ++i){
		int inst = (*instructions)[i];
		int inst24 = ((inst & mask) >> 24);
		if (
		(inst24 == INSTRUCTION_J) ||
		(inst24 == INSTRUCTION_BZ) ||
		(inst24 == INSTRUCTION_CALL)){ 
			int labelId = getImmU(inst, IMM_BIT_COUNT_FLOW);
			LabelIdMap::const_iterator it = mLabelIdMap.find(labelId);
			ASSERT(it != mLabelIdMap.end());
			int address = it->second->mAddress;
			if ((address > getMaxImmU(IMM_BIT_COUNT_FLOW)) || (address < 0)){
				*mMessageStream << L"- : j,bz,call命令のメモリ番号を解決できない。メモリ番号がマイナスか、" << getMaxImmU(IMM_BIT_COUNT_FLOW) << L"を超えている。" << std::endl;
				return false;
			}
			(*instructions)[i] = (inst & mask) | address;
		}
	}
	return true;
}

} //namespace Sunaba

