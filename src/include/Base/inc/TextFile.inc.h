#include "Base/Os.h"

namespace Sunaba{

//InputTextFile
inline InputTextFile::InputTextFile(const wchar_t* filename){
	InputFile in(filename);
	if (in.isError()){
		return;
	}
	const char* data = reinterpret_cast<const char*>(in.data()->pointer());
	int size = in.data()->size();
	convertToUnicode(&mText, data, size);
}

inline bool InputTextFile::isError() const{
	return (mText.pointer() == 0);
}

inline const Array<wchar_t>* InputTextFile::text() const{
	return &mText;
}

//OutputTextFile
inline OutputTextFile::OutputTextFile(const wchar_t* filename) : mFile(0){
	mFile = new OutputFile(filename);
	if (mFile->isError()){
		DELETE(mFile);
	}
}

inline OutputTextFile::~OutputTextFile(){
	DELETE(mFile);
}

inline bool OutputTextFile::isError() const{
	return (mFile == 0);
}

inline void OutputTextFile::write(const wchar_t* text, int size){
	if (!mFile){
		return;
	}
	//変換を行う。
	//テンポラリバッファのサイズを計算
	//\nを\r\nに変換し、\0が出たらそこで打ち切る。
	//2文字づつ見る関係で、最初の文字だけチェック
	int src = 0;
	if (text[src] == L'\0'){
		return; //何も書き込まない
	}
	int dstSize = 0;
	if (text[src] == L'\n'){
		++dstSize;
	}
	++src;
	++dstSize;
	while (src < size){
		if ((text[src - 1] != L'\r') && (text[src] == L'\n')){
			++dstSize;
		}else if (text[src] == L'\0'){
			break;
		}
		++src;
		++dstSize;
	}
	Array<wchar_t> modified(dstSize);
	//改めて変換しながらコピー
	int dst = 0;
	src = 0;
	if (text[src] == L'\n'){
		modified[dst] = L'\r';
		++dst;
	}
	modified[dst] = text[src];
	++src;
	++dst;
	while (src < size){
		if ((text[src - 1] != L'\r') && (text[src] == L'\n')){
			modified[dst] = L'\r';
			++dst;
		}else if (text[src] == L'\0'){
			break;
		}
		modified[dst] = text[src];
		++src;
		++dst;
	}
	//UCS2-LEに変換
	Array<unsigned char> bin((dstSize * 2) + 2);
	bin[0] = 0xff;
	bin[1] = 0xfe;
	dst = 2;
	for(int i = 0; i < dstSize; ++i){
		bin[dst + 0] = static_cast<unsigned char>(modified[i] & 0xff);
		bin[dst + 1] = static_cast<unsigned char>((modified[i] >> 8) & 0xff);
		dst += 2;
	}
	mFile->write(bin.pointer(), bin.size());
	if (mFile->isError()){
		DELETE(mFile);
	}
}

inline void OutputTextFile::flush(){
	if (!mFile){
		return;
	}
	mFile->flush();
}

} //namespace Sunaba
