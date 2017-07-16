#include "Base/Array.h"
#include "Compiler/Node.h"
#include "Compiler/FunctionInfo.h"
#include "Base/Tank.h"

namespace Sunaba{

//Variable
inline FunctionGenerator::Variable::Variable() : mDefined(false), mInitialized(false), mOffset(0){
}

inline void FunctionGenerator::Variable::set(int offset){
	ASSERT(!mDefined);
	mOffset = offset;
}

inline void FunctionGenerator::Variable::define(){
	mDefined = true;
}

inline bool FunctionGenerator::Variable::isDefined() const{
	return mDefined;
}

inline void FunctionGenerator::Variable::initialize(){
	mInitialized = true;
}

inline bool FunctionGenerator::Variable::isInitialized() const{
	return mInitialized;
}

inline int FunctionGenerator::Variable::offset() const{
	return mOffset;
}

//BlockTreeNode
inline FunctionGenerator::Block::Block(int baseOffset) :
mParent(0),
mBaseOffset(baseOffset),
mFrameSize(0){
}

inline FunctionGenerator::Block::~Block(){
	mParent = 0;
}

inline bool FunctionGenerator::Block::addVariable(const RefString& name, bool isArgument){
	std::pair<VariableMap::iterator, bool> p = mVariables.insert(VariableMap::value_type(name, Variable()));
	if (!p.second){
		return false;
	}else{
		Variable* retVar = &(p.first->second);
		retVar->set(mBaseOffset + mFrameSize);
		++mFrameSize;
		if (isArgument){ //引数なら定義済みかつ初期化済み
			retVar->define();
			retVar->initialize();
		}
		return true;
	}
}

inline void FunctionGenerator::Block::collectVariables(const Node* firstStatement){
	const Node* statement = firstStatement;
	while (statement){
		//代入文なら、変数名を取得して登録。
		if (statement->mType == NODE_SUBSTITUTION_STATEMENT){
			const Node* left = statement->mChild;
			ASSERT(left);
			if (left->mType == NODE_VARIABLE){ //変数への代入文のみ扱う。配列要素や定数は無視。
				ASSERT(left->mToken);
				RefString vName(left->mToken->mString);
				Variable* v = findVariable(vName); //ここより上にこの変数があるか調べる。
				if (!v){ //ない。新しい変数を生成
					if (!addVariable(vName)){
						ASSERT(false); //ありえん
					}
				}
			}
		}
		statement = statement->mBrother;
	}
}

inline FunctionGenerator::Variable* FunctionGenerator::Block::findVariable(const RefString& name){
	//まず自分にあるか？
	VariableMap::iterator it = mVariables.find(name);
	if (it != mVariables.end()){ //あった。
		return &(it->second);
	}else if (mParent){ //親がいる
		return mParent->findVariable(name);
	}else{ //こりゃダメだ。
		return 0;
	}
}

inline void FunctionGenerator::Block::beginError(std::wostringstream* stream, const Node* node){
	const Token* token = node->mToken;
	ASSERT(token);
	stream->write(token->mFilename.pointer(), token->mFilename.size());
	if (token->mLine != 0){
		*stream << L'(' << token->mLine << L") ";
	}else{
		*stream << L' ';
	}
}

//FunctionGenerator
inline bool FunctionGenerator::process(
Tank<wchar_t>* result,
std::wostringstream* messageStream,
const Node* root,
const RefString& name,
const FunctionInfo& info,
const std::map<RefString, FunctionInfo>& functionMap,
bool english){
	FunctionGenerator generator(messageStream, name, info, functionMap, english);
	return generator.process(result, root);
}

inline FunctionGenerator::FunctionGenerator(
std::wostringstream* messageStream,
const RefString& name,
const FunctionInfo& info,
const std::map<RefString, FunctionInfo>& functionMap,
bool english) :
mMessageStream(messageStream),
mRootBlock(0),
mCurrentBlock(0),
mLabelId(0),
mName(name),
mInfo(info),
mFunctionMap(functionMap),
mEnglish(english),
mOutputExist(false){
}

inline FunctionGenerator::~FunctionGenerator(){
	mMessageStream = 0;
	DELETE(mRootBlock);
	mCurrentBlock = 0;
}

inline bool FunctionGenerator::process(
Tank<wchar_t>* out, 
const Node* node){
	const Node* headNode = node; //後でエラー出力に使うのでとっておく。
	//FP相対オフセットを計算
	int argCount = mInfo.argCount();
	//ルートブロック生成(TODO:このnew本来不要。コンストラクタでスタックに持つようにできるはず)
	mCurrentBlock = mRootBlock = new Block(-argCount - 3);  //戻り値、引数*N、FP、CPと詰めたところが今のFP。戻り値の位置は-argcount-3

	//戻り値変数を変数マップに登録
	mCurrentBlock->addVariable(RefString(L"!ret"));

	//引数処理
	//みつかった順にアドレスを割り振りながらマップに登録。
	//呼ぶ時は前からプッシュし、このあとFP,PCをプッシュしたところがSPになる。
	const Node* child = node->mChild;
	while (child){ //このループを抜けた段階で最初のchildは最初のstatementになっている
		if (child->mType != NODE_VARIABLE){
			break;
		}
		ASSERT(child->mToken);
		RefString variableName(child->mToken->mString);
		if (!mCurrentBlock->addVariable(variableName, true)){
			beginError(node);
			*mMessageStream << L"部分プログラム\"";
			mMessageStream->write(mName.pointer(), mName.size());
			*mMessageStream << L"\"の入力\"";
			mMessageStream->write(variableName.pointer(), variableName.size());
			*mMessageStream << L"\"はもうすでに現れた。二個同じ名前があるのはダメ。" << std::endl;
			return false;
		}
		child = child->mBrother;
	}
	//FP、CPを変数マップに登録(これで処理が簡単になる)
	mCurrentBlock->addVariable(RefString(L"!fp"));
	mCurrentBlock->addVariable(RefString(L"!cp"));

	//ルートブロックのローカル変数サイズを調べる
	mRootBlock->collectVariables(child);

	//関数開始コメント
	out->addString(L"\n#部分プログラム\"");
	out->add(mName.pointer(), mName.size());
	out->addString(L"\"の開始\n");
	//関数開始ラベル
	out->addString(L"func_"); //160413: add等のアセンブラ命令と同名の関数があった時にラベルを命令と間違えて誤作動する問題の緊急回避
	out->add(mName.pointer(), mName.size());
	out->addString(L":\n");

	//ローカル変数を確保
	wchar_t numberBuffer[16];
	int netFrameSize = mCurrentBlock->mFrameSize - 3 - argCount; //戻り値、FP、CP、引数はここで問題にするローカル変数ではない。呼出側でプッシュしているからだ。
	if (netFrameSize > 0){
		out->addString(L"pop ");
		makeIntString(numberBuffer, -netFrameSize); //-1は戻り値を入れてしまった分
		out->addString(numberBuffer);
		out->addString(L" #ローカル変数確保\n");
	}
	//中身を処理
	const Node* lastStatement = 0;
	while (child){
		if (!generateStatement(out, child)){
			return false;
		}
		lastStatement = child;
		child = child->mBrother;
	}
	//関数終了点ラベル。上のループの中でreturnがあればここに飛んでくる。
//	out->add(mName.pointer(), mName.size());
//	out->addString(L"_end:\n");

	//ret生成(ローカル変数)
	out->addString(L"ret ");
	makeIntString(numberBuffer, netFrameSize);
	out->addString(numberBuffer);
	out->addString(L" #部分プログラム\"");
	out->add(mName.pointer(), mName.size());
	out->addString(L"\"の終了\n");
	//出力の整合性チェック。
	//ifの中などで出力してるのに、ブロック外に出力がないケースを検出
	if (mInfo.hasOutputValue() != mOutputExist){
		ASSERT(mOutputExist); //outputExistがfalseで、hasOutputValue()がtrueはありえない
		if (headNode->mToken){ //普通の関数ノード
			beginError(headNode);
			*mMessageStream << L"部分プログラム\"";
			mMessageStream->write(mName.pointer(), mName.size());
			*mMessageStream << L"\"は出力したりしなかったりする。条件実行や繰り返しの中だけで出力していないか？" << std::endl;
		}else{ //プログラムノード
			ASSERT(headNode->mChild);
			beginError(headNode->mChild);
			*mMessageStream << L"このプログラムは出力したりしなかったりする。条件実行や繰り返しの中だけで出力していないか？" << std::endl;
		}
		return false;
	}
	return true;
}

inline bool FunctionGenerator::generateStatement(Tank<wchar_t>* out, const Node* node){
	//ブロック生成命令は別扱い
	if (
	(node->mType == NODE_WHILE_STATEMENT) ||
	(node->mType == NODE_IF_STATEMENT)){
		//新ブロック生成
		Block newBlock(mCurrentBlock->mBaseOffset + mCurrentBlock->mFrameSize);
		newBlock.mParent = mCurrentBlock; //親差し込み
		mCurrentBlock = &newBlock;
		mCurrentBlock->collectVariables(node->mChild); //フレーム生成
		//ローカル変数を確保
		wchar_t numberBuffer[16];
		if (mCurrentBlock->mFrameSize > 0){
			out->addString(L"pop ");
			makeIntString(numberBuffer, -(mCurrentBlock->mFrameSize));
			out->addString(numberBuffer);
			out->addString(L" #ブロックローカル変数確保\n");
		}
		if (node->mType == NODE_WHILE_STATEMENT){
			if (!generateWhile(out, node)){
				return false;
			}
		}else if (node->mType == NODE_IF_STATEMENT){
			if (!generateIf(out, node)){
				return false;
			}
		}
		//ローカル変数ポップ
		if (mCurrentBlock->mFrameSize > 0){
			out->addString(L"pop ");
			makeIntString(numberBuffer, mCurrentBlock->mFrameSize);
			out->addString(numberBuffer);
			out->addString(L" #ブロックローカル変数破棄\n");
		}
		mCurrentBlock = mCurrentBlock->mParent; //スタック戻し
	}else if (node->mType == NODE_SUBSTITUTION_STATEMENT){
		if (!generateSubstitution(out, node)){
			return false;
		}
	}else if (node->mType == NODE_FUNCTION){ //関数だけ呼んで結果を代入しない文
		if (!generateFunctionStatement(out, node)){
			return false;
		}
	}else if (node->mType == NODE_FUNCTION_DEFINITION){ //関数定義はもう処理済みなので無視。
		; //スルー
	}else{
		ASSERT(false); //BUG
	}
	return true;
}

/*
ブロック開始処理に伴うローカル変数確保を行い、

1の間ループ->0ならループ後にジャンプと置き換える。

whileBegin:
  Expression;
  push 0
  eq
  b whileEnd
  Statement ...
  push 1
  b whileBegin //最初へ
whileEnd:
*/
inline bool FunctionGenerator::generateWhile(Tank<wchar_t>* out, const Node* node){
	ASSERT(node->mType == NODE_WHILE_STATEMENT);

	wchar_t numberBuffer[16];
	//開始ラベル
	out->add(mName.pointer(), mName.size());
	out->addString(L"_whileBegin");
	makeIntString(numberBuffer, mLabelId);
	++mLabelId;
	out->addString(numberBuffer);
	out->addString(L":\n");
	//Expression処理
	const Node* child = node->mChild;
	ASSERT(child);
	if (!generateExpression(out, child)){ //最初の子はExpression
		return false;
	}
	//いくつかコード生成
	out->addString(L"bz "); //-1
	out->add(mName.pointer(), mName.size());
	out->addString(L"_whileEnd");
	out->addString(numberBuffer);
	out->addString(L"\n");

	//内部の文を処理
	child = child->mBrother;
	while (child){
		if (!generateStatement(out, child)){
			return false;
		}
		child = child->mBrother;
	}
	//ループの最初へ飛ぶジャンプを生成
	out->addString(L"j ");
	out->add(mName.pointer(), mName.size());
	out->addString(L"_whileBegin");
	out->addString(numberBuffer);
	out->addString(L" #ループ先頭へ無条件ジャンプ\n");
	//ループ終了ラベルを生成
	out->add(mName.pointer(), mName.size());
	out->addString(L"_whileEnd");
	out->addString(numberBuffer);
	out->addString(L":\n");
	return true;
}

/*
1なら直下を実行->0ならif文末尾にジャンプと置き換える
  Expression
  push 0
  eq
  b ifEnd
  Statement...
ifEnd:
*/
inline bool FunctionGenerator::generateIf(Tank<wchar_t>* out, const Node* node){
	ASSERT(node->mType == NODE_IF_STATEMENT);
	wchar_t numberBuffer[16];
	//Expression処理
	const Node* child = node->mChild;
	ASSERT(child);
	if (!generateExpression(out, child)){ //最初の子はExpression
		return false;
	}
	//コード生成
	out->addString(L"bz ");
	out->add(mName.pointer(), mName.size());
	out->addString(L"_ifEnd");
	makeIntString(numberBuffer, mLabelId);
	++mLabelId;
	out->addString(numberBuffer);
	out->addString(L"\n");

	//内部の文を処理
	child = child->mBrother;
	while (child){
		if (!generateStatement(out, child)){
			return false;
		}
		child = child->mBrother;
	}
	//ラベル生成
	out->add(mName.pointer(), mName.size());
	out->addString(L"_ifEnd");
	out->addString(numberBuffer);
	out->addString(L":\n");
	return true;
}

inline bool FunctionGenerator::generateFunctionStatement(Tank<wchar_t>* out, const Node* node){
	//まず関数呼び出し
	if (!generateFunction(out, node, true)){
		return false;
	}
	//関数の戻り値がプッシュされているので捨てます。
//	out->addString(L"pop 1 #戻り値を使わないので、破棄\n");
	return true;
}

inline bool FunctionGenerator::generateFunction(Tank<wchar_t>* out, const Node* node, bool isStatement){
	ASSERT(node->mType == NODE_FUNCTION);
	//まず、その関数が存在していて、定義済みかを調べる。
	ASSERT(node->mToken);
	RefString funcName(node->mToken->mString);
	std::map<RefString, FunctionInfo>::const_iterator it = mFunctionMap.find(funcName);
	if (it == mFunctionMap.end()){
		beginError(node);
		*mMessageStream << L"部分プログラム\"";
		mMessageStream->write(funcName.pointer(), funcName.size());
		*mMessageStream << L"\"なんて知らない。" << std::endl;
		return false;
	}
	const FunctionInfo& func = it->second;
	int popCount = 0; //後で引数/戻り値分ポップ
	if (func.hasOutputValue()){ //戻り値あるならプッシュ
		out->addString(L"pop -1 #");
		out->add(funcName.pointer(), funcName.size());
		out->addString(L"の戻り値領域\n");
		if (isStatement){ //戻り値を使わないのでポップ数+1
			++popCount;
		}
	}else if (!isStatement){ //戻り値がないなら式の中にあっちゃだめ
		beginError(node);
		*mMessageStream << L"部分プログラム\"";
		mMessageStream->write(funcName.pointer(), funcName.size());
		*mMessageStream << L"\"は、\"出力\"か\"out\"という名前付きメモリがないので、出力は使えない。ifやwhileの中にあってもダメ。" << std::endl;
		return false;
	}
	//引数の数をチェック
	Node* arg = node->mChild;
	int argCount = 0;
	while (arg){
		++argCount;
		arg = arg->mBrother;
	}
	popCount += argCount; //引数分追加
	if (argCount != func.argCount()){
		beginError(node);
		*mMessageStream << L"部分プログラム\"";
		mMessageStream->write(funcName.pointer(), funcName.size());
		*mMessageStream << L"\"は、入力を" << func.argCount();
		*mMessageStream << L"個受け取るのに、ここには" << argCount << L"個ある。間違い。" << std::endl;
		return false;
	}
	//引数を評価してプッシュ
	arg = node->mChild;
	while (arg){
		if (!generateExpression(out, arg)){
			return false;
		}
		arg = arg->mBrother;
	}
	//call命令生成
	out->addString(L"call func_"); //160413: add等のアセンブラ命令と同名の関数があった時にラベルを命令と間違えて誤作動する問題の緊急回避

	out->add(funcName.pointer(), funcName.size());
	out->addString(L"\n");

	//返ってきたら、引数/戻り値をポップ
	if (popCount > 0){
		out->addString(L"pop ");
		wchar_t numberBuffer[16];
		makeIntString(numberBuffer, popCount);
		out->addString(numberBuffer);
		out->addString(L" #引数/戻り値ポップ\n");
	}
	
	return true;
}

/*
LeftValue
Expression
st
*/
inline bool FunctionGenerator::generateSubstitution(Tank<wchar_t>* out, const Node* node){
	ASSERT(node->mType == NODE_SUBSTITUTION_STATEMENT);
	//左辺値のアドレスを求める。最初の子が左辺値
	Node* child = node->mChild;
	ASSERT(child);
	//変数の定義状態を参照
	Variable* var = 0;
	if ((child->mType == NODE_OUT) || child->mToken){ //変数があるなら
		RefString name(child->mToken->mString);
		if (child->mType == NODE_OUT){
			name = L"!ret";
		}
		var = mCurrentBlock->findVariable(name);
		if (!var){ //配列アクセス時でタイプミスすると変数が存在しないケースがある
			beginError(child);
			*mMessageStream << L"名前付きメモリか定数\"";
			mMessageStream->write(name.pointer(), name.size());
			*mMessageStream << L"\"は存在しないか、まだ作られていない。" << std::endl;
			return false; 
		}else if (!(var->isDefined())){ //未定義ならここで定義
			var->define();
		}
	}
	int staticOffset;
	bool fpRelative;
	if (!pushDynamicOffset(out, &staticOffset, &fpRelative, child)){
		return false;
	}
	//右辺処理
	child = child->mBrother;
	ASSERT(child);
	if (!generateExpression(out, child)){
		return false;
	}
	wchar_t numberBuffer[16];
	makeIntString(numberBuffer, staticOffset);
	if (fpRelative){
		out->addString(L"fst ");
	}else{
		out->addString(L"st ");
	}
	out->addString(numberBuffer);
	out->addString(L" #\"");
	out->add(node->mToken->mString.pointer(), node->mToken->mString.size());
	out->addString(L"\"へストア\n");
	//左辺値は初期化されたのでフラグをセット。すでにセットされていても気にしない。
	if (var){
		var->initialize();
	}
	return true;
}

//第一項、第二項、第二項オペレータ、第三項、第三項オペレータ...という具合に実行
inline bool FunctionGenerator::generateExpression(Tank<wchar_t>* out, const Node* node){
	//解決されて単項になっていれば、そのままgenerateTermに丸投げ。ただし単項マイナスはここで処理。
	bool ret = false;
	if (node->mType != NODE_EXPRESSION){
		ret = generateTerm(out, node);
	}else{
		if (node->mNegation){
			out->addString(L"i 0 #()に対する単項マイナス用\n"); //0をプッシュ
		}
		//項は必ず二つある。
		ASSERT(node->mChild);
		ASSERT(node->mChild->mBrother);
		if (!generateTerm(out, node->mChild)){
			return false;
		}
		if (!generateTerm(out, node->mChild->mBrother)){
			return false;
		}
		//演算子を適用
		const wchar_t* opStr = 0;
		switch (node->mOperator){
			case OPERATOR_PLUS: opStr = L"add"; break;
			case OPERATOR_MINUS: opStr = L"sub"; break;
			case OPERATOR_MUL: opStr = L"mul"; break;
			case OPERATOR_DIV: opStr = L"div"; break;
			case OPERATOR_LT: opStr = L"lt"; break;
			case OPERATOR_LE: opStr = L"le"; break;
			case OPERATOR_EQ: opStr = L"eq"; break;
			case OPERATOR_NE: opStr = L"ne"; break;
			default: ASSERT(false); break; //これはParserのバグ。とりわけ、LE、GEは前の段階でGT,LTに変換されていることに注意
		}
		out->addString(opStr);
		out->addString(L"\n");
		//単項マイナスがある場合、ここで減算
		if (node->mNegation){
			out->addString(L"sub #()に対する単項マイナス用\n");
		}
		ret = true;
	}
	return ret;
}

//右辺値。
inline bool FunctionGenerator::generateTerm(Tank<wchar_t>* out, const Node* node){
	//単項マイナス処理0から引く
	if (node->mNegation){
		out->addString(L"i 0 #単項マイナス用\n"); //0をプッシュ
	}
	wchar_t numberBuffer[16];
	//タイプで分岐
	if (node->mType == NODE_EXPRESSION){
		if (!generateExpression(out, node)){
			return false;
		}
	}else if (node->mType == NODE_NUMBER){ //数値は即値プッシュ
		makeIntString(numberBuffer, node->mNumber );
		out->addString(L"i ");
		out->addString(numberBuffer);
		out->addString(L" #即値プッシュ\n");
	}else if (node->mType == NODE_FUNCTION){
		if (!generateFunction(out, node, false)){
			return false;
		}
	}else{ //ARRAY_ELEMENT,VARIABLEのアドレスプッシュ処理
		//変数の定義状態を参照
		Variable* var = 0;
		if (node->mToken){ //変数があるなら
			RefString name(node->mToken->mString);
			if (node->mType == NODE_OUT){
				name = L"!ret";
			}
			var = mCurrentBlock->findVariable(name);
			//知らない変数。みつからないか、あるんだがまだその行まで行ってないか。				
 			if (!var){
				beginError(node);
				*mMessageStream << L"名前付きメモリか定数\"";
				mMessageStream->write(name.pointer(), name.size());
				*mMessageStream << L"\"は存在しない。" << std::endl;
				return false; 
			}
			if (!(var->isDefined())){
				beginError(node);
				*mMessageStream << L"名前付きメモリ\"";
				mMessageStream->write(name.pointer(), name.size());
				*mMessageStream << L"\"はまだ作られていない。" << std::endl;
				return false; //まだ宣言してない
			}
			if (!(var->isInitialized())){
				beginError(node);
				*mMessageStream << L"名前付きメモリ\"";
				mMessageStream->write(name.pointer(), name.size());
				if (mEnglish){
					*mMessageStream << L"\"は数をセットされる前に使われている。「a->a」みたいなのはダメ。" << std::endl;
				}else{
					*mMessageStream << L"\"は数をセットされる前に使われている。「a→a」みたいなのはダメ。" << std::endl;
				}
				return false; //まだ宣言してない
			}
		}
		int staticOffset;
		bool fpRelative;
		if (!pushDynamicOffset(out, &staticOffset, &fpRelative, node)){
			return false;
		}
		makeIntString(numberBuffer, staticOffset);
		if (fpRelative){
			out->addString(L"fld ");
		}else{
			out->addString(L"ld ");
		}
		out->addString(numberBuffer);
		if (node->mToken){
			out->addString(L" #変数\"");
			out->add(node->mToken->mString.pointer(), node->mToken->mString.size());
			out->addString(L"\"からロード\n");
		}else{
			out->addString(L"\n");
		}
	}
	//単項マイナスがある場合、ここで減算
	if (node->mNegation){
		out->addString(L"sub #単項マイナス用\n");
	}
	return true;
}

//添字アクセスがあれば、添字を計算してpush、addを追加する。また、変数そのもののオフセットと、絶対アドレスか否か(memory[]か否か)を返す
inline bool FunctionGenerator::pushDynamicOffset(
Tank<wchar_t>* out,
int* staticOffset,
bool* fpRelative,
const Node* node){
	*fpRelative = false;
	*staticOffset = -0x7fffffff; //あからさまにおかしな値を入れておく。デバグのため。
	ASSERT((node->mType == NODE_OUT) || (node->mType == NODE_VARIABLE) || (node->mType == NODE_ARRAY_ELEMENT));
	//トークンは数字ですか、名前ですか
	if (node->mToken){
		if (node->mToken->mType == TOKEN_OUT){
			RefString name(L"!ret");
			Variable* var = mCurrentBlock->findVariable(name);
			ASSERT(var);
			*fpRelative = true; //変数直のみFP相対
			*staticOffset = var->offset();
			mOutputExist = true;
		}else if (node->mToken->mType == TOKEN_NAME){
			//変数の定義状態を参照
			RefString name(node->mToken->mString);
			Variable* var = mCurrentBlock->findVariable(name);
			//配列ならExpressionを評価してプッシュ
			if (node->mType == NODE_ARRAY_ELEMENT){
				wchar_t numberBuffer[16];
				makeIntString(numberBuffer, var->offset());
				out->addString(L"fld ");
				out->addString(numberBuffer);
				out->addString(L" #ポインタ\"");
				RefString name(node->mToken->mString);
				out->add(name.pointer(), name.size());
				out->addString(L"\"からロード\n");
				if (node->mChild){ //変数インデクス
					if (!generateExpression(out, node->mChild)){ //アドレスオフセットがプッシュされる
						return false;
					}
					out->addString(L"add\n");
					*staticOffset = 0;
				}else{ //定数インデクス
					*staticOffset = node->mNumber;
				}
			}else{
				*fpRelative = true; //変数直のみFP相対
				*staticOffset = var->offset();
			}
		}
	}else{ //定数アクセス。トークンがない。
		ASSERT(node->mType == NODE_ARRAY_ELEMENT); //インデクスがない定数アクセスはアドレスではありえない。
		if (node->mChild){ //変数インデクス
			if (!generateExpression(out, node->mChild)){ //アドレスをプッシュ
				return false;
			}
		}else{
			out->addString(L"i 0 #絶対アドレスなので0\n"); //絶対アドレスアクセス
		}
		*staticOffset = node->mNumber;
	}
	return true;
}

inline void FunctionGenerator::beginError(const Node* node){
	const Token* token = node->mToken;
	ASSERT(token);
	mMessageStream->write(token->mFilename.pointer(), token->mFilename.size());
	if (token->mLine != 0){
		*mMessageStream << L'(' << token->mLine << L") ";
	}else{
		*mMessageStream << L' ';
	}

}

} //namespace Sunaba
