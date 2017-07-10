#include "Base/Tank.h"
#include "Base/Array.h"

namespace Sunaba{

//文字置換を行う。
//\n以外の制御文字と全角スペース->半角スペース
inline void CharacterReplacer::process(Array<wchar_t>* out, const Array<wchar_t>& in){
	out->setSize(in.size() * 2); //全部2文字に化けるケースに備える
	int size = in.size();
	int dst = 0;
	wchar_t c2; //2文字になる場合
	for (int i = 0; i < size; ++i){
		wchar_t c = in[i];
		c2 = L'\0';
		if ( //捨てる文字を列挙
		((c < L' ') && (c != L'\n')) || //改行以外の制御文字(31以下)
		(c == 0x7f)){ //DEL(0x7f)
			; //破棄
		}else{ //活かす文字は一部置換
			if ((c >= L'０') && (c <= L'９')){ //全角数字
				c = (c - L'０') + L'0';
			}else if ((c >= L'Ａ') && (c <= L'Ｚ')){ //全角大文字
				c = (c - L'Ａ') + L'A';
			}else if ((c >= L'ａ') && (c <= L'ｚ')){ //全角小文字
				c = (c - L'ａ') + L'a';
			}else{ //その他の全角半角変換
				switch (c){
					//0x20-0x2f
//					case L'　': c = L' '; break; //全角スペースはTabProcessorにて半角に置換済み
					case L'！': c = L'!'; break;
					case L'”': c = L'"'; break;
					case L'＃': c = L'#'; break; 
					case L'＄': c = L'$'; break; 
					case L'％': c = L'%'; break; 
					case L'＆': c = L'&'; break; 
					case L'’': c = L'\''; break;
					case L'（': c = L'('; break;
					case L'）': c = L')'; break;
					case L'＊': c = L'*'; break;
					case L'＋': c = L'+'; break;
					case L'，': c = L','; break;
					case L'、': c = L','; break; //日本語の「、」もカンマと見做す
					case L'−': c = L'-'; break;
					case L'．': c = L'.'; break;
					case L'／': c = L'/'; break;
					//0x3a-0x3f
					case L'：': c = L':'; break; 
					case L'；': c = L';'; break; 
					case L'＜': c = L'<'; break;
					case L'＝': c = L'='; break;
					case L'＞': c = L'>'; break;
					case L'？': c = L'?'; break;
					case L'＠': c = L'@'; break;
					case L'［': c = L'['; break;
					case L'¥': c = L'\\'; break; 
					case L'］': c = L']'; break;
					case L'＾': c = L'^'; break;
					case L'＿': c = L'_'; break; 
					//0x4b-0x4f
					case L'｛': c = L'{'; break;
					case L'｜': c = L'|'; break;
					case L'｝': c = L'}'; break;
					case L'〜': c = L'~'; break;
					//その他
					case L'×': c = L'*'; break; //×もサポート
					case L'÷': c = L'/'; break; //÷もサポート
					case L'≦': c = L'<'; c2 = L'='; break;
					case L'≠': c = L'!'; c2 = L'='; break;
					case L'≧': c = L'>'; c2 = L'='; break;
					case L'→': c = L'-'; c2 = L'>'; break;
					case L'⇒': c = L'-'; c2 = L'>'; break;
					default: break; //変換なし
				}
			}
			(*out)[dst] = c;
			++dst;
			if (c2 != L'\0'){ //2文字目がある
				(*out)[dst] = c2;
				++dst;
			}
		}
	}
	out->setSize(dst);
}

} //namespace Sunaba

