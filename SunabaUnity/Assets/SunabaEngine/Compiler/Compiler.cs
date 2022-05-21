using System.Collections.Generic;
using System.Diagnostics;

namespace Sunaba
{
	class Compiler
	{
		public static bool Process(
			System.Text.StringBuilder output,
			System.IO.StreamWriter messageStream,
			string filename,
			Localization localization,
			bool outputIntermediates)
		{
			//ファイル結合+トークン分解
			var fullPathFilenames = new List<string>(); //エラー表示用のファイル名を保持する。

			var tokens = new List<Token>();
			if (!Concatenator.Process(
				tokens, 
				messageStream, 
				filename, 
				fullPathFilenames, 
				localization,
				outputIntermediates))
			{
				return false;
			}

			//英語文法主体か非英語文法主体かをここで判別
			var japaneseCount = 0;
			var englishCount = 0;
			foreach (var token in tokens)
			{
				var t = token.type;
				if (
					(t == TokenType.WhilePost) ||
					(t == TokenType.IfPost) ||
					(t == TokenType.DefPost))
				{
					++japaneseCount;
				}
				else if (
					(t == TokenType.WhilePre) ||
					(t == TokenType.IfPre) ||
					(t == TokenType.DefPre))
				{
					++englishCount;
				}
				else if (t == TokenType.Out)
				{
					if (token.str == localization.outWord)
					{
						++japaneseCount;
					}
					else if (token.str == "out")
					{
						++englishCount;
					}
					else
					{
						Debug.Assert(false);
					}
				}
			}

			var english = (englishCount >= japaneseCount);

			//パースします。
			var rootNode = Parser.Process(
				tokens, 
				messageStream,
				english, 
				localization);
			if (rootNode == null)
			{
				return false;
			}

			//コード生成
			if (!CodeGenerator.Process(
				output, 
				messageStream, 
				rootNode, 
				english, 
				outputIntermediates))
			{
				return false;
			}

			return true;
		}
	}
} //namespace Sunaba
