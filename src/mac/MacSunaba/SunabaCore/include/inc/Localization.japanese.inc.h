namespace Sunaba{

inline void Localization::initJapanese(){
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
	ifAtHead = false; //「なら」は後ろ
	whileAtHead = false; //「な限り」は後ろ
	defAtHead = false; //「とは」は後ろ
	ifWord = L"なら";
	whileWord0 = L"なかぎり";
	whileWord1 = L"な限り";
	defWord = L"とは";
	constWord = L"定数";
	includeWord = L"挿入";
	outWord = L"出力";
	memoryWord = L"メモリ";
	argDelimiter = L'、';
	errorMessages = table;
	errorMessageCount = sizeof(table) / sizeof(ErrorMessage);
}

} //namespace Sunaba
