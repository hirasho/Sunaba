namespace Sunaba{

inline void Localization::initKorean(){
	static const ErrorMessage table[] = {
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
	ifAtHead = false; //「이면」は後ろ
	whileAtHead = false; //「동안」は後ろ
	defAtHead = false; //「정의」は後ろ
	ifWord = L"이면";
	whileWord0 = L"동안";
	whileWord1 = 0;
	defWord = L"정의";
	constWord = L"상수";
	includeWord = L"포함";
	outWord = L"출력";
	memoryWord = L"메모리";
	argDelimiter = L'\0'; //カンマだけでいいので不要
	errorMessages = table;
	errorMessageCount = sizeof(table) / sizeof(ErrorMessage);
}

} //namespace Sunaba
