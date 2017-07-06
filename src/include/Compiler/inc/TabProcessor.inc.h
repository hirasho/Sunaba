#include "Base/Tank.h"
#include "Base/Array.h"

namespace Sunaba{

inline void TabProcessor::process(Array<wchar_t>* out, const Array<wchar_t>& in, int tabWidth){
	Tank<wchar_t> tmp;
	int col = 0; //行内位置
	int p = 0; //位置
	int l = in.size();
	while (p < l){
		if (in[p] == L'\t'){ //タブ発見
			int spaceCount = tabWidth - (col % tabWidth); //丁度割り切れれば丸々、最小1。
			for (int i = 0; i < spaceCount; ++i){
				tmp.add(L' ');
			}
			col += spaceCount;
		}else if(in[p] == L'　'){ //全角スペースは半角2個に置換
			tmp.add(L' ');
			tmp.add(L' ');
			col += 2;
		}else{
			tmp.add(in[p]);
			if (in[p] == L'\n'){ //改行発見
				col = 0;
			}else{
				++col;
			}
		}
		++p;
	}
	//出力
	tmp.copyTo(out);
}

} //namespace Sunaba
