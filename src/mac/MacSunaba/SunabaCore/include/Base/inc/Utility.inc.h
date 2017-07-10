#include "Base/Array.h"
#include "Base/Base.h"

namespace Sunaba{

inline int getStringSize(const wchar_t* str){
	int l = 0;
	while (str[l] != 0){
		++l;
	}
	return l;
}

inline bool isEqualString(const wchar_t* s0, int l0, const wchar_t* s1, int l1){
	if (l0 != l1){
		return false;
	}
	for (int i = 0; i < l0; ++i){
		if (s0[i] != s1[i]){
			return false;
		}
	}
	return true;
}

//s0はサイズ指定、s1はサイズ不明NULL終端文字列
inline bool isEqualString(const wchar_t* s0, int l0, const wchar_t* s1){
	int i = 0;
	while ((i < l0) && (s1[i])){
		if (s0[i] != s1[i]){
			return false;
		}
		++i;
	}
	return ((i == l0) && (s1[i] == L'\0')); //長さが同じならtrue
}

inline bool isLessString(const wchar_t* s0, int l0, const wchar_t* s1, int l1){
	for (int i = 0; (i < l0) && (i < l1); ++i){
		if (s0[i] < s1[i]){
			return true;
		}else if ( s0[i] > s1[i]){
			return false;
		}
	}
	return (l0 < l1);
}

inline wchar_t toLower(wchar_t c){
	if ((c >= 'A') && (c <= 'Z')){
		c -= 'A';
		c += 'a';
	}
	return c;
}

inline bool checkExtension(const wchar_t* path, const wchar_t* ext){
	int sl = getStringSize(path);
	int el = getStringSize(ext);
	
	int periodPos = sl - 1;
	while (periodPos >= 0){
		if (path[periodPos] == L'.'){
			break;
		}
		--periodPos;
	}
	int l;
	if (periodPos < 0){
		l = 0;
	}else{
		l = sl - periodPos - 1;
	}
	if (l != el){
		return false;
	}
	for (int i = 0; i < l; ++i){
		wchar_t c0 = toLower(path[i + periodPos + 1]);
		wchar_t c1 = toLower(ext[i]);
		if (c0 != c1){
			return false;
		}
	}
	return true;
}

inline bool isAlphabet(wchar_t c){
	bool r = false;
	if ( 
	((c >= L'a') && (c <= L'z')) ||
	((c >= L'A') && (c <= L'Z'))){
		r = true;
	}
	return r;
}

//[?_@$%&a-zA-Z全角0_9]のチェック
inline bool isInName(wchar_t c){ 
	bool r = false;
	if (
	(c == L'@') ||
	(c == L'$') ||
	(c == L'&') ||
	(c == L'?') ||
	(c == L'_') ||
	isAlphabet(c) ||
	((c >= L'0') && (c <= L'9')) ||
	(c >= 0x100)){ //2バイト文字ならなんでもOK
		r = true;
	}
	return r;
}

//識別子ですか？
inline bool isName(const wchar_t* s, int l){
	bool r = true;
	if (l == 0){
		r = false;
	}else{
		for (int i = 0; i < l; ++i){
			if (!isInName(s[i])){
				r = false;
				break;
			}
		}
	}
	return r;
}

inline bool isAsmName(const wchar_t* s, int l){
	bool r = true;
	if (l == 0){
		r = false;
	}else{
		for (int i = 0; i < l; ++i){
			if (!isInName(s[i]) && (s[i] != L'!')){
				r = false;
				break;
			}
		}
	}
	return r;
}

inline bool isHexNumber(wchar_t c){
	bool r = false;
	if (
	((c >= L'a') && (c <= L'f')) ||
	((c >= L'A') && (c <= L'F')) ||
	((c >= L'0') && (c <= L'9'))){
		r = true;
	}
	return r;
}

inline bool isDecNumber(wchar_t c){
	return ((c >= L'0') && (c <= L'9'));
 }

inline bool isBinNumber(wchar_t c){
	return ((c == L'0') || (c == L'1'));
}

inline bool isAllHexNumber(const wchar_t* s, int l){
	bool r = true;
	for (int i = 0; i < l; ++i){
		if (!isHexNumber(s[i])){
			r = false;
			break;
		}
	}
	return r;
}

inline bool isAllDecNumber(const wchar_t* s, int l){
	bool r = true;
	for (int i = 0; i < l; ++i){
		if (!isDecNumber(s[i])){
			r = false;
			break;
		}
	}
	return r;
}

inline bool isAllBinNumber(const wchar_t* s, int l){
	bool r = true;
	for (int i = 0; i < l; ++i){
		if (!isBinNumber(s[i])){
			r = false;
			break;
		}
	}
	return r;
}

inline bool convertHexNumber(int* out, const wchar_t* s, int l){
	int o = 0;
	for (int i = 0; i < l; ++i){
		o *= 16;
		wchar_t c = s[i];
		if ((c >= L'0') && (c <= L'9')){
			o += (c - L'0');
		}else if ((c >= L'a') && (c <= L'f')){
			o += (c - L'a') + 0xA;
		}else if ((c >= L'A') && (c <= L'A')){
			o += (c - L'A') + 0xA;
		}else{
			return false;
		}
	}
	*out = o;
	return true;
}

inline bool convertDecNumber(int* out, const wchar_t* s, int l){
	int o = 0;
	for (int i = 0; i < l; ++i){
		o *= 10;
		wchar_t c = s[i];
		if ((c >= L'0') && (c <= L'9')){
			o += (c - L'0');
		}else{
			return false;
		}
	}
	*out = o;
	return true;
}

inline bool convertBinNumber(int* out, const wchar_t* s, int l){
	int o = 0;
	for (int i = 0; i < l; ++i){
		o *= 2;
		wchar_t c = s[i];
		if ((c >= L'0') && (c <= L'1')){
			o += (c - L'0');
		}else{
			return false;
		}
	}
	*out = o;
	return true;
}

//文字列->数字(マイナスも認識)
inline bool convertNumber(int* out, const wchar_t* s, int l){
	bool minus = false;
	bool ret = false;
	if (s[0] == L'-'){ //マイナス付き
		minus = true;
		++s; //sを1文字進める
		--l; //lを1文字減らす
	}
	if (isDecNumber(s[0])){ //最初が数値なら数値か、不正かのどちらか
		if (l >= 3){ //3文字以上で、
			if (s[0] == L'0'){ //0から始まり、
				if ((s[1] == L'x') || (s[1] == L'X')){ //xなら16進
					ret = convertHexNumber(out, s + 2, l - 2);
				}else if ((s[1] == L'b') || (s[1] == L'B')){ //bなら2進
					ret = convertBinNumber(out, s + 2, l - 2);
				}
			}
		}
		if (!ret){ //2進や16進で引っかからなかったならば、10進を試す
			ret = convertDecNumber(out, s, l);
		}
		if (ret && minus){
			*out = -(*out);
		}
	}
	return ret;
}

//10進で数値化
inline void makeIntString(wchar_t* p, int v){
	wchar_t* origP = p;
	if (v < 0){
		*p = L'-';
		v = -v;
		++p;
	}
	int div = 1000 * 1000 * 1000; //10億
	bool nonZeroFound = false;
	for (int i = 0; i < 9; ++i){ //10桁
		int t = v / div;
		if (nonZeroFound || (t > 0)){
			*p = static_cast<wchar_t>(t + L'0');
			++p;
			nonZeroFound = true;
		}
		v -= t * div;
		div /= 10;
	}
	//最後の桁は必ず出力
	*p = static_cast<wchar_t>(v + L'0');
	++p;
	*p = L'\0';
	ASSERT((p - origP) < 16);

	static_cast<void>(origP); //Releaseでは使いません。
}

inline int convertColor100to256(int a){
	unsigned u = static_cast<unsigned>(a);
	unsigned r = u / 10000;
	u -= r * 10000;

	unsigned g = u / 100;
	u -= g * 100;
	unsigned b = u;
	
	r = ((r * 255) + 49) / 99;
	g = ((g * 255) + 49) / 99;
	b = ((b * 255) + 49) / 99;

	return (b << 16) | (g << 8) | r;
}

inline int roundUpToPow2(int a){
	STRONG_ASSERT(a > 0);
	int r = 1;
	while (r < a){
		r += r;
	}
	return r;
}

inline int readS4(const unsigned char* p){
	return p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);
}

inline int readS2(const unsigned char* p){
	return p[0] + (p[1] << 8);
}

inline int packColor(const unsigned char* p){
	return p[0] + (p[1] << 8) + (p[2] << 16); //BGR
}

inline void getDirectoryNameFromAbsoluteFilename(Array<wchar_t>* out, const wchar_t* filename){
#ifdef _WIN32
	int l = getStringSize(filename);
	//後ろから\か/を探す
	int pos = l - 1;
	while (pos >= 0){
		if ((filename[pos] == L'\\') || (filename[pos] == L'/')){
			break;
		}
		--pos;
	}
	ASSERT((pos >= 0) && "bug or unexpected input"); //これはありえないはずだが...
	out->clear();
	out->setSize(pos + 1); //バックスラッシュ含むので+1
	for (int i = 0; i <= pos; ++i){ //バックスラッシュ含む
		wchar_t c = filename[i];
		c = (c == L'/') ? L'\\' : c; //スラッシュはバックスラッシュに置き換え
		(*out)[i] = c;
	}
#else
    int l = getStringSize( filename );
    // 後ろから \ から / を探す
    int pos = l - 1;
    while( pos >= 0 ) {
        if( (filename[pos] == L'\\') || (filename[pos] == L'/') ) {
            break;
        }
        --pos;
    }
    ASSERT( (pos >=0) && "bug or unexpected input"); //これはありえないはずだが...
    out->clear();
    out->setSize( l+1 );
    for( int i = 0; i <= pos; ++i ) {
        wchar_t c = filename[i];
        c = ( c == L'\\' ) ? L'/' : c; // '\' をスラッシュに置き換える.
        (*out)[i] = c;
    }
    (*out)[ l ] = '\0';
#endif
}

inline void makeAbsoluteFilename(Array<wchar_t>* out, const wchar_t* basePath, const wchar_t* filename){
#ifdef _WIN32
	out->clear();
	int l = getStringSize(filename);
	if (isAbsoluteFilename(filename)){ //絶対パスが来た。
		//outへは丸写し。
		out->setSize(l + 1); //NULL含むので+1
		for (int i = 0; i <= l; ++i){ //NULL含む
			(*out)[i] = filename[i];
		}
	}else{ //相対パスである場合、
		int basePathLength = getStringSize(basePath);
		//後ろから\か/を探す
		int pos = basePathLength - 1;
		while (pos >= 0){
			if ((basePath[pos] == L'\\') || (basePath[pos] == L'/')){
				break;
			}
			--pos;
		}
		//pos==-1なら相対パスそのままになる
		out->setSize(pos + 1 + l + 1); //最初の+1がバックスラッシュ。最後のはNULL
		for (int i = 0; i <= pos; ++i){ //バックスラッシュ含む
			(*out)[i] = basePath[i];
		}
		for (int i = 0; i <= l; ++i){ //NULL含む
			(*out)[pos + 1 + i] = filename[i];
		}
	}
	//最後にスラッシュをバックスラッシュに
	l = out->size();
	for (int i = 0; i < l; ++i){
		if ((*out)[i] == L'/'){
			(*out)[i] = L'\\';
		}
	}
#else
	out->clear();
	int l = getStringSize(filename);
	if (isAbsoluteFilename(filename)){ //絶対パスが来た。
		//outへは丸写し。
		out->setSize(l + 1); //NULL含むので+1
		for (int i = 0; i <= l; ++i){ //NULL含む
			(*out)[i] = filename[i];
		}
	}else{ //相対パスである場合、
		int basePathLength = getStringSize(basePath);
		//後ろから / を探す
		int pos = basePathLength - 1;
		while (pos >= 0){
			if ( basePath[pos] == L'/' ){
				break;
			}
			--pos;
		}
		//pos==-1なら相対パスそのままになる
		out->setSize(pos + 1 + l + 1); //最初の+1がバックスラッシュ。最後のはNULL
		for (int i = 0; i <= pos; ++i){ //バックスラッシュ含む
			(*out)[i] = basePath[i];
		}
		for (int i = 0; i <= l; ++i){ //NULL含む
			(*out)[pos + 1 + i] = filename[i];
		}
	}
	// 最後にバックスラッシュをスラッシュに.
	l = out->size();
	for( int i = 0; i < l; ++i ){
		if( (*out)[i] == L'\\' ) {
			(*out)[i] = L'/';
		}
	}
#endif
}

inline int getFilenameBegin(const wchar_t* filename, int filenameSize){
#ifdef _WIN32
	//後ろから\か/を探す
	int l = filenameSize;
	int p = l - 1;
	while (p >= 0){
		if ((filename[p] == L'\\') || (filename[p] == L'/')){
			break;
		}
		--p;
	}
	p += 1; //最初の文字はその次
	return p;
#elif defined(__APPLE__)
	//後ろから / を探す
	int l = filenameSize;
	int p = l - 1;
	while (p >= 0){
		if (filename[p] == L'/'){
			break;
		}
		--p;
	}
	p += 1; //最初の文字はその次
	return p;
#else
# error "Unknown"
#endif
}

inline bool isAbsoluteFilename(const wchar_t* filename){
	bool r = false;
#ifdef _WIN32
	if (isAlphabet(filename[0])){ //最初の文字がアルファベットで、
		if (filename[1] == L':'){ //セミコロンがあれば、絶対パス
			r = true;
		}
	}
	if (!r){ //\が2連続すれば絶対パス。共有。
	   if ((filename[0] == L'\\') && (filename[1] == L'\\')){
		   r = true;
	   }
	}
#elif defined(__APPLE__)
    if( filename[0] == L'/' ) {
        r = true;
    }
#else
#error "Unknown"
#endif
	return r;
}

} //namespace Sunaba
