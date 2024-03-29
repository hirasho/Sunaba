using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Sunaba
{
	public class Concatenator
	{
		public static bool Process(
			List<Token> output,
			StringBuilder messageStream,
			string rootFilename,
			List<string> fullPathFilenamesOut,
			Localization localization,
			bool outputIntermediates)
		{
			var concatenator = new Concatenator(
				fullPathFilenamesOut, 
				messageStream, 
				rootFilename, 
				localization);
			return concatenator.Process(output, rootFilename, outputIntermediates);
		}

		// non public --------
		string rootFilename;
		int line; //吐き出したトークンの最大行
		List<string> fullPathFilenames; //今まで処理したファイル名をここに保存。フルパス。
		HashSet<string> filenameSet;
		List<Token> tokens;
		StringBuilder messageStream; //借り物
		Localization localization; //借り物

		Concatenator(
			List<string> fullPathFilenamesOut, 
			StringBuilder messageStream,
			string rootFilename,
			Localization localization)
		{
			this.fullPathFilenames = fullPathFilenamesOut;
			this.messageStream = messageStream;
			this.rootFilename = rootFilename;
			this.localization = localization;
			this.filenameSet = new HashSet<string>();
		}
	 
		bool Process(List<Token> tokensOut, string filename, bool outputIntermediates)
		{
			this.tokens = tokensOut;
			var filenameToken = new Token();
			filenameToken.str = filename;
			filenameToken.filename = filenameToken.str;
			if (!ProcessFile(filenameToken, rootFilename, outputIntermediates))
			{
				return false;
			}
			tokens.Add(new Token(TokenType.End, 0)); //ファイル末端
			var n = tokens.Count;
			if (n >= 2)
			{ //END以外にもあるなら、Endのファイル名を一個前からもらう
				tokens[n - 1].filename = tokens[n - 2].filename;
			}
			return true;
		}

		bool ProcessFile(Token filenameToken, string parentFullPath, bool outputIntermediates)
		{
			var filename = filenameToken.str;
			var parentPath = Path.GetDirectoryName(parentFullPath);
			var fullPath = Path.Combine(parentPath, filename);
			// ファイルがなかったら
			if (!File.Exists(fullPath))
			{
				var ext = System.IO.Path.GetExtension(fullPath);
				if (string.IsNullOrEmpty(ext))
				{
					fullPath = fullPath + ".txt"; // 拡張子がなければ自動でtxtを補う
				}

				if (!File.Exists(fullPath))
				{
					BeginError(filenameToken);
					messageStream.AppendFormat("{0} を開けない。合ってる?\n", fullPath);
					return false;
				}
			}

			//同じファイルを読んだことがないかフルパスでチェック。エラーにはせず、素通り。
			fullPathFilenames.Add(fullPath);
			if (filenameSet.Contains(fullPath))
			{
				return true;
			}
			filenameSet.Add(fullPath);

			string text = null;
			try
			{
				text = File.ReadAllText(fullPath);
			}
			catch (System.Exception e)
			{
				BeginError(filenameToken);
				messageStream.Append("を開けない。あるのか確認せよ。\n");
				return false;
			}

			if (string.IsNullOrEmpty(text))
			{
				BeginError(filenameToken);
				messageStream.Append("をテキストファイルとして解釈できない。文字コードは大丈夫か？そもそも本当にテキストファイル？\n");
				return false;
			}

			var filenameTrunk = Path.GetFileNameWithoutExtension(filename);

			//タブ処理
			var tabProcessed = new List<char>();
			TabProcessor.Process(tabProcessed, text.ToCharArray());
			if (outputIntermediates)
			{
				Utility.WriteDebugFile("concatenator_" + filenameTrunk + "_tabProcessed.txt", new string(tabProcessed.ToArray()));
			}

			//\rなど、文字数を変えない範囲で邪魔なものを取り除く。
			var characterReplaced = new List<char>(); 
			CharacterReplacer.Process(characterReplaced, tabProcessed, localization);
			if (outputIntermediates)
			{
				Utility.WriteDebugFile("concatenator_" + filenameTrunk + "_characterReplaced.txt", new string(characterReplaced.ToArray()));
			}

			//コメントを削除する。行数は変えない。
			var commentRemoved = new List<char>();
			if (!CommentRemover.Process(commentRemoved, characterReplaced))
			{
				return false;
			}

			if (outputIntermediates)
			{
				Utility.WriteDebugFile("concatenator_" + filenameTrunk + "_commentRemoved.txt", new string(commentRemoved.ToArray()));
			}

			//トークン分解
			var lexicalAnalyzed = new List<Token>();
			if (!LexicalAnalyzer.Process(lexicalAnalyzed, messageStream, commentRemoved.ToArray(), fullPath, localization))
			{
				return false;
			}

			if (outputIntermediates)
			{
				Utility.WriteDebugFile("concatenator_" + filenameTrunk + "_lexicalAnalyzed.txt", Token.ToString(lexicalAnalyzed));
			}

			//全トークンにファイル名を差し込む。そのためにファイル名だけにする。
			var rawFilename = Path.GetFileName(filenameToken.str);
			foreach (var token in lexicalAnalyzed)
			{
				token.filename = rawFilename;
			}

			var structurized = new List<Token>();
			if (!Structurizer.Process(structurized, messageStream, lexicalAnalyzed, localization))
			{
				return false;
			}

			if (outputIntermediates)
			{
				Utility.WriteDebugFile("concatenator_" + filenameTrunk + "_structurized.txt", Token.ToString(structurized));
			}

			//改めて全トークンにファイル名を差し込む
			foreach (var token in structurized)
			{
				token.filename = rawFilename;
			}

			//タンクにトークンを移動。
			//includeを探してそこだけ構文解析
			var tokenCount = structurized.Count;
			for (var i = 0; i < tokenCount; ++i)
			{
				if (structurized[i].type == TokenType.Include)
				{ //発見
					if ((i + 2) >= tokenCount)
					{
						BeginError(filenameToken);
						messageStream.Append("挿入(include)行の途中でファイルが終わった。\n");
					}

					if (structurized[i + 1].type != TokenType.StringLiteral)
					{
						BeginError(structurized[i]);
						messageStream.Append("挿入(include)と来たら、次は\"\"で囲まれたファイル名が必要。\n");
					}

					if (structurized[i + 2].type != TokenType.StatementEnd)
					{
						BeginError(structurized[i]);
						messageStream.Append("挿入(include)行に続けて何かが書いてある。改行しよう。\n");
					}

					if (!ProcessFile(structurized[i + 1], fullPath, outputIntermediates))
					{
						return false;
					}
					i += 2;
				}
				else
				{
					line = structurized[i].line;
					tokens.Add(structurized[i]);
				}
			}
			return true;
		}

		void BeginError(Token token)
		{
			messageStream.Append(token.filename);
			if (token.line != 0)
			{
				messageStream.AppendFormat("({0}):", token.line);
			}
			else
			{
				messageStream.Append(':');
			}
		}
	}
} //namespace Sunaba
