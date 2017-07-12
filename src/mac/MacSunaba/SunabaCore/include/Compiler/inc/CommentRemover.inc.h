
//#define ENABLE_CPP_COMMENT

namespace Sunaba{

inline bool CommentRemover::process(Array<wchar_t>* out, const Array<wchar_t>& in){
	Tank<wchar_t> tmp;
	int l = in.size();
	if (l < 2){
		return false;
	}
	int l1 = l - 1;
	int level = 0; //コメントのネストを有効に
	bool shortComment = false;

	wchar_t c0, c1, c2;

	//最初の文字
	c1 = in[0];
	c2 = in[1];
	if ((c1 == L'/') && (c2 == L'*')){ //コメント開始
		++level;
	}
	if (level == 0){
#if ENABLE_CPP_COMMENT
		if ((c1 == L'/') && (c2 == L'/')){
			shortComment = true;
		}
#endif
		if (c1 == L'#'){
			shortComment = true;
		}
	}
	if (c1 == L'\n'){
		shortComment = false;
		tmp.add(L'\n');
	}else if ((level == 0) && !shortComment){
		tmp.add(c1);
	}
	//ループ
	for (int i = 1; i < l1; ++i){
		c0 = in[i - 1];
		c1 = in[i];
		c2 = in[i + 1];
		if ((c1 == L'/') && (c2 == L'*')){ //コメント開始
			++level;
		}
		if (level == 0){
#if ENABLE_CPP_COMMENT
			if ((c1 == L'/') && (c2 == L'/')){
				shortComment = true;
			}
#endif
			if (c1 == L'#'){
				shortComment = true;
			}
		}
		//文字出力
		if (c1 == L'\n'){
			shortComment = false;
			tmp.add(L'\n');
		}else if ((level == 0) && !shortComment){
			tmp.add(c1);
		}
		if ((c0 == L'*') && (c1 == L'/')){ //コメント終了
			if (level > 0){
				--level;
			}
		}
	}
	//最後の文字
	c1 = in[l - 1];
	if (c1 == L'\n'){
		shortComment = false;
		tmp.add(L'\n');
	}else if ((level == 0) && !shortComment){
		tmp.add(c1);
	}
	//字句解析の便宜を図るために、最後に改行を追加
	tmp.add(L'\n');
	//出力
	tmp.copyTo(out);
	return true;
}

} //namespace Sunaba

#undef ENABLE_CPP_COMMENT