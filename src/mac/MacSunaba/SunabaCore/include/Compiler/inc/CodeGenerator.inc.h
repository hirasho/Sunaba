#include "Base/Array.h"
#include "Compiler/Node.h"
#include "Base/Os.h"
#include "Base/TextFile.h"
#include "Compiler/FunctionInfo.h"
#include "Compiler/FunctionGenerator.h"

namespace Sunaba{

inline bool CodeGenerator::process(
Array<wchar_t>* result, 
std::wostringstream* messageStream,
const Node* root,
bool english){
	CodeGenerator generator(messageStream, english);
	return generator.process(result, root);
}

inline CodeGenerator::CodeGenerator(
std::wostringstream* messageStream,
bool english) : 
mMessageStream(messageStream),
mEnglish(english){
}

inline CodeGenerator::~CodeGenerator(){
	mMessageStream = 0;
}

inline bool CodeGenerator::process(Array<wchar_t>* result, const Node* root){
	Tank<wchar_t> tmp;
	if (!generateProgram(&tmp, root)){ //エラー
		return false;
	}
	tmp.copyTo(result);
	//デバグ出力
	tmp.add(L'\n');
	tmp.add(L'\0');
	Array<wchar_t> debugOut;
	tmp.copyTo(&debugOut);
	writeToConsole(debugOut.pointer());    
#ifndef NDEBUG
#ifdef __APPLE__
    Array<wchar_t> outputPath;
    {
        char pathBuf[2048] = { 0 };
        Sunaba::getWorkingDirectory( pathBuf );
        sprintf( pathBuf, "%s/compiled.txt", pathBuf );
        pathBuf[ strlen(pathBuf) ] = '\0';
        convertToUnicode( &outputPath, pathBuf, (int) strlen(pathBuf)+1, false );
    }

	OutputTextFile outFile( outputPath.pointer() );
#else
	OutputTextFile outFile(L"compiled.txt");
#endif
	outFile.write(debugOut.pointer(), debugOut.size());
#endif
	return true;
}

inline bool CodeGenerator::generateProgram(Tank<wchar_t>* out, const Node* node){
	ASSERT(node->mType == NODE_PROGRAM);
	out->addString(L"pop -1 #$mainの戻り値領域\n");
	out->addString(L"call !main\n"); //main()呼び出し
	out->addString(L"j !end #プログラム終了点へジャンプ\n"); //プログラム終了点へジャンプ
	//$mainの情報を足しておく
	FunctionInfo& mainFuncInfo = mFunctionMap.insert(std::make_pair(RefString(L"!main"), FunctionInfo())).first->second;

	//関数情報収集。関数コールを探しだして、見つけ次第、引数、出力、名前についての情報を収集してmapに格納
	Node* child = node->mChild;
	while (child){
		if (child->mType == NODE_FUNCTION_DEFINITION){
			if (!collectFunctionDefinitionInformation(child)){ //main以外
				return false;
			}
		}
		child = child->mBrother;
	}
	//関数コールを探しだして、見つけ次第コード生成
	child = node->mChild;
	while (child){
		if (child->mType == NODE_FUNCTION_DEFINITION){
			if (!generateFunctionDefinition(out, child)){ //main以外
				return false;
			}
		}else if (child->isOutputValueSubstitution()){ //なければ出力があるか調べる
			mainFuncInfo.setHasOutputValue(); //戻り値があるのでフラグを立てる。
		}
		child = child->mBrother;
	}
	//あとはmain
	if (!generateFunctionDefinition(out, node)){
		return false;
	}
	//最後にプログラム終了ラベル
	out->addString(L"\n!end:\n");
	out->addString(L"pop 1 #!mainの戻り値を破棄。最終命令。なくてもいいが。\n");
	return true;
}

inline bool CodeGenerator::collectFunctionDefinitionInformation(const Node* node){
	int argCount = 0; //引数の数
	//まず、関数マップに項目を足す
	RefString funcName;
	const Node* child = node->mChild;
	FunctionInfo* funcInfo = 0;
	ASSERT(node->mToken);
	funcName = node->mToken->mString;
	//関数重複チェック
	std::pair<FunctionMap::iterator, bool> fp = mFunctionMap.insert(std::make_pair(funcName, FunctionInfo()));
	if (fp.second == false){ //もうこの関数ある
		beginError(node);
		*mMessageStream << L"部分プログラム\"";
		mMessageStream->write(funcName.pointer(), funcName.size());
		*mMessageStream << L"\"はもう作られている。" << std::endl;
		return false;
	}
	funcInfo = &(fp.first->second);
	//引数の処理
	//まず数を数える
	{ //argが後ろに残ってるとバグ源になるので閉じ込める
		const Node* arg = child; //childは後で必要なので、コピーを操作
		while (arg){
			if (arg->mType != NODE_VARIABLE){
				break;
			}
			++argCount;
			arg = arg->mBrother;
		}
	}
	funcInfo->setArgCount(argCount);
	//出力値があるか調べる
	const Node* lastStatement = 0;
	while (child){
		if (child->isOutputValueSubstitution()){
			funcInfo->setHasOutputValue(); //戻り値があるのでフラグを立てる。
		}
		lastStatement = child;
		child = child->mBrother;
	}
	return true;
}

inline bool CodeGenerator::generateFunctionDefinition(
Tank<wchar_t>* out,
const Node* node){
	//まず、関数マップに項目を足す
	RefString funcName;
	if (node->mToken){
		funcName = node->mToken->mString;
	}else{
		funcName = L"!main";
	}
	FunctionMap::iterator it = mFunctionMap.find(funcName);
	ASSERT(it != mFunctionMap.end()); //絶対ある
	//関数重複チェック
	if (!FunctionGenerator::process(out, mMessageStream, node, funcName, it->second, mFunctionMap, mEnglish)){
		return false;
	}
	return true;
}

inline void CodeGenerator::beginError(const Node* node){
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


