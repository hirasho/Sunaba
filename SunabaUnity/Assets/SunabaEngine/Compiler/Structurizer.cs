using System.Collections.Generic;
using System.Text;

namespace Sunaba
{
	public class Structurizer
	{
		/*
		行頭トークンにはスペースの数が入っている。
		これを見つけて、「ブロック開始」「ブロック終了」「文末」の3つのトークンを挿入する。

		行頭トークンを見つけたら、そのスペース数をスタックトップと比較する。
		増えていればブロック開始を挿入してプッシュ、減っていればスタック
		同じスペース数を発見するまでポップしつつその数だけブロック終了を挿入する。
		等しければ、文末トークンを挿入する。
		*/
		public static bool Process(
			List<Token> output,
			StringBuilder messageStream,
			List<Token> input, // 破壊される
			Localization localization)
		{
			var structurizer = new Structurizer(messageStream);
			return structurizer.Process(output, input, localization);
		}

		// non public -------
		StringBuilder messageStream;

		bool Process(
			List<Token> output,
			List<Token> input,
			Localization localization)
		{
			var spaceCountStack = new Stack<int>();
			spaceCountStack.Push(0); //ダミーで一個入れておく
			var n = input.Count;
			var bracketLevel = 0; //これが0の時しか処理しない。(で+1、)で-1。
			var indexLevel = 0; //これが0の時しか処理しない。[で+1、]で-1。
			var statementExist = false; //空行の行末トークンを生成させないためのフラグ。
			Token prevT = null;
			for (var i = 0; i < n; ++i)
			{
				var t = input[i];
				if (t.type == TokenType.LeftBracket)
				{
					++bracketLevel;
				}
				else if (t.type == TokenType.RightBracket)
				{
					--bracketLevel;
					if (bracketLevel < 0)
					{ //かっこがおかしくなった！
						BeginError(t);
						messageStream.Append(")が(より多い。\n");
						return false;
					}
				}
				else if (t.type == TokenType.IndexBegin)
				{
					++indexLevel;
				}
				else if (t.type == TokenType.IndexEnd)
				{
					--indexLevel;
					if (indexLevel < 0)
					{ //かっこがおかしくなった！
						BeginError(t);
						messageStream.Append("]が[より多い。\n");
						return false;
					}
				}
				
				if (t.type == TokenType.LineBegin)
				{ //行頭トークン発見
					//前のトークンが演算子か判定
					var prevIsOp = false;
					if ((prevT != null) && ((prevT.type == TokenType.Operator) || (prevT.type == TokenType.Substitution)))
					{
						prevIsOp = true;
					}

					if ((bracketLevel == 0) && (indexLevel == 0) && !prevIsOp)
					{ //()[]の中になく、前が演算子や代入でない場合は処理。それ以外は読み捨てる。
						var newCount = t.spaceCount;
						var oldCount = spaceCountStack.Peek();
						if (newCount > oldCount)
						{ //増えた
							spaceCountStack.Push(newCount);
							output.Add(new Token(TokenType.BlockBegin, t.line));
						}
						else if (newCount == oldCount)
						{ //前の文を終了
							if (statementExist)
							{ //中身が何かあれば
								output.Add(new Token(TokenType.StatementEnd, t.line));
								statementExist = false;
							}
						}
						else
						{ //newCount < oldCount){ ブロック終了トークン挿入
							if (statementExist)
							{ //中身が何かあれば文末
								output.Add(new Token(TokenType.StatementEnd, t.line));
								statementExist = false;
							}

							while (newCount < oldCount)
							{
								oldCount = spaceCountStack.Pop();
								output.Add(new Token(TokenType.BlockEnd, t.line));
							}
						
							if (newCount != oldCount)
							{ //ずれてる
								BeginError(t);
								messageStream.Append("字下げが不正。ずれてるよ。前の深さに合わせてね。\n");
								return false;
							}
						}
					}
				}
				else
				{ //その他であればそのトークンをそのまま挿入
					output.Add(t);
					statementExist = true;
				}
				prevT = t;
			}

			//もし最後に何かあるなら、最後の文末を追加
			var lastLine = input[n - 1].line;
			if (statementExist)
			{
				output.Add(new Token(TokenType.StatementEnd, lastLine));
			}

			//ブロック内にいるならブロック終了を補う
			while (spaceCountStack.Count > 1)
			{
				spaceCountStack.Pop();
				output.Add(new Token(TokenType.BlockEnd, lastLine));
			}

			return true;
		}

		Structurizer(StringBuilder messageStream)
		{
			this.messageStream = messageStream;
		}

		void BeginError(Token token)
		{
			messageStream.Append(token.filename);
			if (token.line != 0)
			{
				messageStream.AppendFormat("({0}) ", token.line);
			}
			else
			{
				messageStream.Append(' ');
			}
		}
	}
} //namespace Sunaba
