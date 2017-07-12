namespace Sunaba{

inline void Localization::initChinese(){
	static const ErrorMessage table[] = {
		//結合
		{ 
			ERROR_CONCATENATOR_CANT_OPEN_FILE, 
			0, 
			L"を開けない。あるのか確認せよ。" },
		{ 
			ERROR_CONCATENATOR_CANT_OPEN_FILE, 
			0, 
			L"をテキストファイルとして解釈できない。文字コードは大丈夫か？そもそも本当にテキストファイルか？" },
		{
			ERROR_CONCATENATOR_INCOMPLETE_INCLUDE,
			L"挿入(include)行の途中でファイルが終わった。",
			0},
		{
			ERROR_CONCATENATOR_INVALID_TOKEN_AFTER_INCLUDE,
			L"挿入(include)と来たら、次は\"\"で囲まれたファイル名が必要。",
			0},
		{
			ERROR_CONCATENATOR_GARBAGE_AFTER_INCLUDE,
			L"挿入(include)行に続けて何かが書いてある。改行しよう。",
			0},
	};
	//言語別文法設定
	ifAtHead = true;
	whileAtHead = true;
	defAtHead = false;
	ifWord = L"如果";
	whileWord0 = L"只要";
	whileWord1 = 0;
	defWord = L"是";
	constWord = L"常数";
	includeWord = L"包含";
	outWord = L"输出";
	memoryWord = L"存储区";
	argDelimiter = L'\0'; //カンマだけでいいので不要
	errorMessages = table;
	errorMessageCount = sizeof(table) / sizeof(ErrorMessage);
}

} //namespace Sunaba
