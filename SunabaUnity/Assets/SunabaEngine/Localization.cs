using System.Collections.Generic;

namespace Sunaba
{
	public enum ErrorName
	{
		ConcatenatorCantOpenFile,
		ConcatenatorCantReadText,
		ConcatenatorIncompleteInclude,
		ConcatenatorInvalidTokenAfterInclude,
		ConcatenatorGarbageAfterInclude,
		
		Unknown,
	};

	public class Localization
	{
		public class ErrorMessage
		{
			public ErrorMessage(ErrorName name, string preToken, string postToken)
			{
				this.name = name;
				this.preToken = preToken;
				this.postToken = postToken;
			}
			public ErrorName name;
			public string preToken;
			public string postToken;
		}

		public bool ifAtHead;
		public bool whileAtHead;
		public bool defAtHead;
		public string ifWord;
		public string whileWord0;
		public string whileWord1;
		public string defWord;
		public string constWord;
		public string includeWord;
		public string outWord;
		public string memoryWord;
		public char argDelimiter;
		public List<ErrorMessage> errorMessages;

		public Localization(string langName)
		{
			switch (langName)
			{
				case "chinese":
					InitChinese();
					break;
				case "korean":
					InitKorean();
					break;
				default:
					InitJapanese();
					break;
			}
		}

		public void InitJapanese()
		{
			// TODO: Jsonでも読むようにしろ C++時代手頃なシリアライザがなかったからこんなことになってる
			errorMessages = new List<ErrorMessage>();
			errorMessages.Add(new ErrorMessage(
				ErrorName.ConcatenatorCantOpenFile,
				null,
				"を開けない。あるのか確認せよ。"));
			errorMessages.Add(new ErrorMessage(
				ErrorName.ConcatenatorCantReadText,
				null,
				"をテキストファイルとして解釈できない。文字コードは大丈夫か？そもそも本当にテキストファイルか？"));
			errorMessages.Add(new ErrorMessage(
				ErrorName.ConcatenatorIncompleteInclude,
				"挿入(include)行の途中でファイルが終わった。",
				null));
			errorMessages.Add(new ErrorMessage(
				ErrorName.ConcatenatorInvalidTokenAfterInclude,
				"挿入(include)と来たら、次は\"\"で囲まれたファイル名が必要。",
				null));
			errorMessages.Add(new ErrorMessage(
				ErrorName.ConcatenatorGarbageAfterInclude,
				"挿入(include)行に続けて何かが書いてある。改行しよう。",
				null));
			//言語別文法設定
			ifAtHead = false; //「なら」は後ろ
			whileAtHead = false; //「な限り」は後ろ
			defAtHead = false; //「とは」は後ろ
			ifWord = "なら";
			whileWord0 = "なかぎり";
			whileWord1 = "な限り";
			defWord = "とは";
			constWord = "定数";
			includeWord = "挿入";
			outWord = "出力";
			memoryWord = "メモリ";
			argDelimiter = '、';
		}

		public void InitChinese()
		{
			// TODO:
		}

		void InitKorean()
		{
			// TODO:
		}
	}
}