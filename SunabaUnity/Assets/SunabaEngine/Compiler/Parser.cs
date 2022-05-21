using System.Collections.Generic;
using System.Diagnostics;

namespace Sunaba
{
	class Parser
	{
		public static Node Process(
			IList<Token> input,
			System.IO.StreamWriter messageStream,
			bool english,
			Localization localization)
		{			
			var parser = new Parser(
				messageStream, 
				english, 
				localization);
			return parser.ParseProgram(input);
		}

		// non public ---------
		Dictionary<string, int> constMap;
		System.IO.StreamWriter messageStream;
		bool english;
		Localization localization;

		Parser(
			System.IO.StreamWriter messageStream,
			bool english, 
			Localization localization)
		{
			this.constMap = new Dictionary<string, int>();		
			this.messageStream = messageStream;
			this.english = english;
			this.localization = localization;
		}

		StatementType GetStatementType(IList<Token> input, int pos)
		{
			//文頭トークンからまず判断
			//いきなり範囲開始がある場合、インデントが狂っている。
			var t = input[pos].type;
			if (t == TokenType.BlockBegin)
			{
				BeginError(input[pos]);
				messageStream.WriteLine("字下げを間違っているはず。上の行より多くなっていないか。");
				return StatementType.Unknown;
			}
			else if ((t == TokenType.WhilePre) || (t == TokenType.IfPre))
			{
				return StatementType.WhileOrIf;
			}
			else if (t == TokenType.DefPre)
			{
				return StatementType.Def;
			}
			else if (t == TokenType.Const)
			{
				return StatementType.Const;
			}
			else if (t == TokenType.Out)
			{
				return StatementType.Substitution;
			}
		
			//文末までスキャンする。
			var endPos = pos;
			while ((input[endPos].type != TokenType.StatementEnd) && (input[endPos].type != TokenType.BlockBegin))
			{
				Debug.Assert(input[endPos].type != TokenType.End); //ENDの前に文末があるはずなんだよ。本当か？TODO:
				++endPos;
			}

			if (endPos > pos)
			{
				t = input[endPos - 1].type;
				if ((t == TokenType.WhilePost) || (t == TokenType.IfPost))
				{
					return StatementType.WhileOrIf;
				}
				else if (t == TokenType.DefPost)
				{
					return StatementType.Def;
				}
			}

			//代入を探す
			for (var p = pos; p < endPos; ++p)
			{
				t = input[p].type;
				if (t == TokenType.Substitution)
				{
					return StatementType.Substitution;
				}
			}

			//となると関数コールくさい。
			if (input[pos].type == TokenType.Name)
			{ //関数呼び出しの可能性 名前 ( とつながれば関数呼び出し文
				if (input[pos + 1].type == TokenType.LeftBracket)
				{
					return StatementType.Function;
				}
			}

			//ここまで来るとダメ。何なのか全然わからない。しかしありがちなのが「なかぎり」と「なら」の左に空白がないケース
			BeginError(input[pos]);//TODO:なら、なかぎりを検索してあれば助言を出す
			if (english)
			{
				messageStream.WriteLine("この行を解釈できない。注釈は//ではなく#だが大丈夫か？");
			}
			else
			{
				messageStream.WriteLine("この行を解釈できない。「なかぎり」や「なら」の左側には空白が必要だが、大丈夫か？また、注釈は//ではなく#だが大丈夫か？");
			}

			for (var p = pos; p < endPos; ++p)
			{
				if (input[p].opType == Operator.Eq)
				{
					messageStream.WriteLine("=があるが、→や->と間違えてないか？");
					break;
				}
			}
			messageStream.WriteLine("");
			return StatementType.Unknown;
		}
		
		TermType GetTermType(IList<Token> input, int pos)
		{
			var r = TermType.Unknown;
			if (input[pos].type == TokenType.LeftBracket)
			{
				r = TermType.Expression;
			}
			else if (input[pos].type == TokenType.Number)
			{
				r = TermType.Number;
			}
			else if (input[pos].type == TokenType.Name)
			{
				++pos;
				if (input[pos].type == TokenType.LeftBracket)
				{
					r = TermType.Function;
				}
				else if (input[pos].type == TokenType.IndexBegin)
				{
					r = TermType.ArrayElement;
				}
				else
				{
					r = TermType.Variable;
				}
			}
			else if (input[pos].type == TokenType.Out)
			{
				r = TermType.Out;
			}
			return r;
		}

		//Program : ( Const | FuncDef | Statement )*
		Node ParseProgram(IList<Token> input)
		{
			//定数マップに「メモリ」と「memory」を登録
			constMap.Add(localization.memoryWord, 0);
			constMap.Add("memory", 0);

			//Programノードを確保
			var node = new Node();
			node.type = NodeType.Program;
			var pos = 0;

			//定数全部処理
			//このループを消して、後ろのループのparseConstのtrueを消せば、定数定義を前に限定できる
			while (input[pos].type != TokenType.End)
			{
				if (input[pos].type == TokenType.Const)
				{
					if (!ParseConst(input, ref pos, false))
					{ //ノードを返さない。
						return null;
					}
				}
				else
				{
					++pos;
				}
			}

			pos = 0;
			Node lastChild = null;
			while (input[pos].type != TokenType.End)
			{
				var type = GetStatementType(input, pos);
				Node child = null;
				if (type == StatementType.Unknown)
				{
					return null;
				}
				else if (type == StatementType.Const)
				{
					if (!ParseConst(input, ref pos, true))
					{ //ノードを返さない。
						return null;
					}
				}
				else
				{
					if (type == StatementType.Def)
					{
						child = ParseFunctionDefinition(input, ref pos);
					}
					else
					{
						child = ParseStatement(input, ref pos);
					}

					if (child == null)
					{
						return null;
					}
					else if (lastChild == null)
					{
						node.child = child;
					}
					else
					{
						lastChild.brother = child;
					}
					lastChild = child;
				}
			}
			return node;
		}

		//Const : const name '->' Expression STATEMENT_END
		bool ParseConst(IList<Token> input, ref int pos, bool skip) //2パス目はスキップ
		{
			if (input[pos].type != TokenType.Const)
			{
				BeginError(input[pos]);
				messageStream.WriteLine("定数行のはずだが解釈できない。上の行からつながってないか。");
				return false;
			}
			++pos;

			//名前
			if (input[pos].type != TokenType.Name)
			{
				BeginError(input[pos]);
				if (english)
				{
					messageStream.WriteLine(string.Format("constの次は定数名。\"{0}\"は定数名と解釈できない。", input[pos].ToString()));
				}
				else
				{
					messageStream.WriteLine(string.Format("\"定数\"の次は定数名。\"{0}\"は定数名と解釈できない。", input[pos].ToString()));
				}
				return false;
			}
			var constName = input[pos].str;
			++pos;

			//->
			if (input[pos].type != TokenType.Substitution)
			{
				BeginError(input[pos]);
				if (english)
				{
					messageStream.WriteLine(string.Format("const [名前]、と来たら次は\"->\"のはずだが\"{0}\"がある。", input[pos].ToString()));
				}
				else
				{
					messageStream.WriteLine(string.Format("定数 [名前]、と来たら次は\"→\"のはずだが\"{0}\"がある。", input[pos].ToString()));
				}
				return false;
			}
			++pos;

			var expression = ParseExpression(input, ref pos);
			if (expression == null)
			{
				return false;
			}

			if (expression.type != NodeType.Number)
			{ //数字に解決されていなければ駄目。
				BeginError(input[pos]);
				messageStream.WriteLine("定数の右辺の計算に失敗した。メモリや名前つきメモリ、部分プログラム参照などが入っていないか？");
				return false;
			}

			var constValue = expression.number;
			//;
			if (input[pos].type != TokenType.StatementEnd)
			{
				BeginError(input[pos]);
				messageStream.WriteLine(string.Format("定数作成の後に\"{0}\"がある。改行してくれ。", input[pos].ToString()));
				return false;
			}
			++pos;

			if (!skip)
			{
				//定数マップに登録
				if (constMap.ContainsKey(constName))
				{
					BeginError(input[pos]);
					messageStream.WriteLine("すでに同じ名前の定数がある。");
					return false;
				}
				else
				{
					constMap.Add(constName, constValue);
				}
			}
			return true;
		}

		//FunctionDefinition : id '(' id? [ ',' id ]* ')' 'とは' [BLOCK_BEGIN statement... BLOCK_END]
		//FunctionDefinition : 'def' id '(' id? [ ',' id ]* ')' [BLOCK_BEGIN statement... BLOCK_END]
		Node ParseFunctionDefinition(IList<Token> input, ref int pos)
		{
			//defスキップ
			var defFound = false;
			if (input[pos].type == TokenType.DefPre)
			{
				++pos;
				defFound = true;
			}

			Debug.Assert(input[pos].type == TokenType.Name);

			//自分ノード
			var node = new Node();
			node.type = NodeType.FunctionDefinition;
			node.token = input[pos]; //関数名トークン
			++pos;

			//(
			if (input[pos].type != TokenType.LeftBracket)
			{
				BeginError(input[pos]);
				messageStream.WriteLine(string.Format("ここで入力リスト開始の\"(\"があるはずだが、\"{0}\"がある。これ、本当に部分プログラム？", input[pos].ToString()));
				return null;
			}
			++pos;

			//次がIDなら引数が一つはある
			Node lastChild = null;
			if (input[pos].type == TokenType.Name)
			{
				Node arg = ParseVariable(input, ref pos);
				if (arg == null)
				{
					return null;
				}
				node.child = arg;
				lastChild = arg;

				//第二引数以降を処理
				while (input[pos].type == TokenType.Comma)
				{ //コンマがある！
					++pos;
					if (input[pos].type != TokenType.Name)
					{
						BeginError(input[pos]);
						if (english)
						{
							messageStream.WriteLine(string.Format("入力リスト中で\",\"があるということは、まだ入力があるはずだ。しかし、\"{0}\"は入力と解釈できない。", input[pos].ToString()));
						}
						else
						{
							messageStream.WriteLine(string.Format("入力リスト中で\"、\"があるということは、まだ入力があるはずだ。しかし、\"{0}\"は入力と解釈できない。", input[pos].ToString()));
						}
						return null;
					}
					arg = ParseVariable(input, ref pos);
					if (arg == null)
					{
						return null;
					}

					//引数名が定数だったら不許可
					if (arg.type == NodeType.Number)
					{
						BeginError(input[pos]);
						messageStream.WriteLine("入力名はすでに定数に使われている。");
						return null;
					}
					lastChild.brother = arg;
					lastChild = arg;
				}
			}

			//)
			if (input[pos].type != TokenType.RightBracket)
			{ 
				BeginError(input[pos]);
				messageStream.WriteLine(string.Format("入力リストの後には\")\"があるはずだが、\"{0}\"がある。\",\"を書き忘れてないか？", input[pos].ToString()));
				return null;
			}
			++pos;

			if (input[pos].type == TokenType.DefPost)
			{ //「とは」がある場合、スキップ
				if (defFound)
				{
					BeginError(input[pos]);
					messageStream.WriteLine("\"def\"と\"とは\"が両方ある。片方にしてほしい。");
				}
				++pos;
			}

			//次のトークンがBLOCK_BEGINの場合、中を処理。
			if (input[pos].type == TokenType.BlockBegin)
			{
				++pos;
				//文をパース。複数ある可能性があり、1つもなくてもかまわない
				while (true)
				{
					Node child = null;
					if (input[pos].type == TokenType.BlockEnd)
					{ //抜ける
						++pos;
						break;
					}
					else if (input[pos].type == TokenType.Const)
					{ //定数。これはエラーです。
						BeginError(input[pos]);
						messageStream.WriteLine("部分プログラム内で定数は作れない。");
						return null;
					}
					else
					{
						child = ParseStatement(input, ref pos);
						if (child == null)
						{
							return null;
						}
					}

					if (lastChild != null)
					{
						lastChild.brother = child;
					}
					else
					{
						node.child = child;
					}
					lastChild = child;
				}
			}
			else if (input[pos].type == TokenType.StatementEnd)
			{ //中身がない
				++pos;
			}
			else
			{
				BeginError(input[pos]);
				messageStream.WriteLine(string.Format("部分プログラムの最初の行の行末に\"{0}\"が続いている。ここで改行しよう。", input[pos].ToString()));
				return null;
			}
			return node;
		}

		//Statement : ( While | If | Return | FuncDef | Func | Substitution )
		Node ParseStatement(IList<Token> input, ref int pos)
		{
			Node node = null;
			var type = GetStatementType(input, pos);
			if (type == StatementType.WhileOrIf)
			{
				node = ParseWhileOrIfStatement(input, ref pos);
			}
			else if (type == StatementType.Def)
			{ //関数定義はありえない
				BeginError(input[pos]);
				messageStream.WriteLine("部分プログラムの中で部分プログラムは作れない。");
				return null;
			}
			else if (type == StatementType.Const)
			{
				Debug.Assert(false); //これは上でチェックしているはず
			}
			else if (type == StatementType.Function)
			{ //関数呼び出し文
				node = ParseFunction(input, ref pos);
				if (node == null)
				{
					return null;
				}

				if (input[pos].type != TokenType.StatementEnd)
				{
					BeginError(input[pos]);
					if (input[pos].type == TokenType.BlockBegin)
					{
						if (english){
							messageStream.WriteLine("部分プログラムを作る気なら「def」が必要。あるいは、次の行の字下げが多すぎる。");
						}
						else
						{
							messageStream.WriteLine("部分プログラムを作る気なら「とは」が必要。あるいは、次の行の字下げが多すぎる。");
						}
					}
					else
					{
						if (english)
						{
							messageStream.WriteLine(string.Format("部分プログラム参照の後ろに、変なもの\"{0}\"がある。部分プログラムを作るなら「def」を置くこと。", input[pos].ToString()));
						}
						else
						{
							messageStream.WriteLine(string.Format("部分プログラム参照の後ろに、変なもの\"{0}\"がある。部分プログラムを作るなら「とは」を置くこと。", input[pos].ToString()));
						}
					}
					return null;
				}
				++pos;
			}
			else if (type == StatementType.Substitution)
			{ //代入文
				node = ParseSubstitutionStatement(input, ref pos);
			}
			else
			{
				Debug.Assert(type == StatementType.Unknown); 
				return null;//エラーメッセージはもう出してある。
			}
			return node;
		}

		//Substitution : [Out | Memory | id | ArrayElement ] '=' Expression STATEMENT_END
		Node ParseSubstitutionStatement(IList<Token> input, ref int pos)
		{
			//以下は引っかかったらおかしい
			if ((input[pos].type != TokenType.Name) && (input[pos].type != TokenType.Out))
			{ 
				BeginError(input[pos]);
				if (english)
				{
					messageStream.WriteLine("->があるのでメモリセット行だと思うが、そうなら最初に「memory」や「out」、名前付きメモリがあるはずだ。 ");
				}
				else
				{
					messageStream.WriteLine("→があるのでメモリセット行だと思うが、そうなら最初に「メモリ」や「出力」、名前付きメモリがあるはずだ。");
				}
				return null;
			}
			//自分ノード
			var node = new Node();
			node.type = NodeType.SubstitutionStatement;
			node.token = input[pos];

			//出力か、メモリ要素か、変数か、配列要素
			Node left = null;
			if (input[pos].type == TokenType.Out)
			{
				left = new Node();
				left.type = NodeType.Out;
				left.token = input[pos];
				++pos;
			}
			else if (input[pos + 1].type == TokenType.IndexBegin)
			{
				left = ParseArrayElement(input, ref pos);
			}
			else
			{
				left = ParseVariable(input, ref pos);
				if (left.type == NodeType.Number)
				{ //定数じゃねえか！
					BeginError(input[pos]);
					messageStream.WriteLine(string.Format("定数 {0}は変えられない。", input[pos].ToString()));
					return null;
				}
			}

			if (left == null)
			{
				return null;
			}
			node.child = left;

			//→が必要
			if (input[pos].type != TokenType.Substitution)
			{
				BeginError(input[pos]);
				if (english)
				{
					messageStream.WriteLine(string.Format("メモリセット行だと思ったのだが、\"->\"があるべき場所に、\"{0}\"がある。もしかして「if」か「while」？", input[pos].ToString()));
				}
				else
				{
					messageStream.WriteLine(string.Format("メモリセット行だと思ったのだが、\"→\"があるべき場所に、\"{0}\"がある。もしかして「なら」か「なかぎり」？", input[pos].ToString()));
				}
				return null;
			}
			++pos;

			//右辺は式
			var expression = ParseExpression(input, ref pos);
			if (expression == null)
			{
				return null;
			}
			left.brother = expression;

			//STATEMENT_ENDが必要
			if (input[pos].type != TokenType.StatementEnd)
			{
				BeginError(input[pos]);
				if (input[pos].type == TokenType.BlockBegin)
				{
					messageStream.WriteLine("次の行の字下げが多すぎる。行頭の空白は同じであるはずだ。");
				}
				else
				{
					messageStream.WriteLine(string.Format("メモリ変更行が終わるべき場所に\"{0}\"がある。改行してね。", input[pos].ToString()));
				}
				return null;
			}
			++pos;
			return node;
		}
		
		Node ParseWhileOrIfStatement(IList<Token> input, ref int pos)
		{
			//自分ノード
			string whileOrIf = null;
			var node = new Node();

			//英語版ならすぐ決まる。
			if (input[pos].type == TokenType.WhilePre)
			{
				node.type = NodeType.WhileStatement;
				whileOrIf = "while";
				node.token = input[pos];
				++pos;
			}
			else if (input[pos].type == TokenType.IfPre)
			{
				node.type = NodeType.IfStatement;
				whileOrIf = "if";
				node.token = input[pos];
				++pos;
			}

			//式を解釈
			var expression = ParseExpression(input, ref pos);
			if (expression == null)
			{
				return null;
			}
			node.child = expression;

			//日本語版ならここにキーワードがあるはず
			if (whileOrIf == null)
			{ //まだ確定してない
				if (input[pos].type == TokenType.WhilePost)
				{
					node.type = NodeType.WhileStatement;
					whileOrIf = "\"なかぎり\"";
				}
				else if (input[pos].type == TokenType.IfPost)
				{
					node.type = NodeType.IfStatement;
					whileOrIf = "\"なら\"";
				}
				node.token = input[pos];
				++pos;
			}

			//次のトークンがBLOCK_BEGINであれば、中身があるので処理。
			if (input[pos].type == TokenType.BlockBegin)
			{
				++pos;

				//文をパース。複数ある可能性があり、1つもなくてもかまわない
				var lastChild = expression;
				while (true)
				{
					Node child = null;
					if (input[pos].type == TokenType.BlockEnd)
					{ //抜ける
						++pos;
						break;
					}
					else if (input[pos].type == TokenType.Const)
					{ //定数。これはエラーです。
						BeginError(input[pos]);
						messageStream.WriteLine("繰り返しや条件実行内で定数は作れない。");
						return null;
					}
					else
					{
						child = ParseStatement(input, ref pos);
						if (child == null)
						{
							return null;
						}
					}
					lastChild.brother = child;
					lastChild = child;
				}
			}
			else if (input[pos].type == TokenType.StatementEnd)
			{ //中身がない
				++pos;
			}
			else
			{
				BeginError(input[pos]);
				messageStream.WriteLine(string.Format("条件行は条件の終わりで改行しよう。\"{0}\"が続いている。", input[pos].ToString()));
				return null;
			}
			return node;
		}
		
		//Function : id '(' [ Expression [ ',' Expression ]* ] ')'
		Node ParseFunction(IList<Token> input, ref int pos)
		{
			Debug.Assert(input[pos].type == TokenType.Name);
			//以下はバグ
			var node = new Node();
			node.type = NodeType.Function;
			node.token = input[pos];
			++pos;

			//(
			Debug.Assert(input[pos].type == TokenType.LeftBracket); //getTermTypeかgetStatementTypeで判定済み。
			++pos;

			//最初のExpressionがあるかどうかは、次が右括弧かどうかでわかる
			if (input[pos].type != TokenType.RightBracket)
			{ //括弧が出たら抜ける
				var expression = ParseExpression(input, ref pos);
				if (expression == null)
				{
					return null;
				}
				node.child = expression;

				//2個目以降。, Expressionが連続する限り取り込む
				var lastChild = expression;
				while (true)
				{
					if (input[pos].type != TokenType.Comma)
					{
						break; //コンマでないなら抜ける
					}
					++pos;
					expression = ParseExpression(input, ref pos);
					if (expression == null)
					{
						return null;
					}
					lastChild.brother = expression;
					lastChild = expression;
				}
			}

			//)
			if (input[pos].type != TokenType.RightBracket)
			{
				BeginError(input[pos]);
				if (english)
				{
					messageStream.WriteLine(string.Format("部分プログラムの入力は\")\"で終わるはず。だが、\"{0}\"がある。\",\"の書き忘れはないか？", input[pos].ToString()));
				}
				else
				{
					messageStream.WriteLine(string.Format("部分プログラムの入力は\")\"で終わるはず。だが、\"{0}\"がある。\"、\"の書き忘れはないか？", input[pos].ToString()));
				}
				return null;
			}
			++pos;
			return node;
		}

		//ArrayElement : id '[' Expression ']'
		Node ParseArrayElement(IList<Token> input, ref int pos)
		{
			//自分ノード
			var node = ParseVariable(input, ref pos);
			if (node == null)
			{
				return null;
			}
			node.type = NodeType.ArrayElement;
			//[
			Debug.Assert(input[pos].type == TokenType.IndexBegin); //getTermTypeで判定済み。
			++pos;

			//Expression
			var expression = ParseExpression(input, ref pos);
			if (expression == null)
			{
				return null;
			}
			node.child = expression;
			//expressionが数値であれば、アドレス計算はここでやる
			if (expression.type == NodeType.Number)
			{
				node.number += expression.number;
				node.child = null; //子のExpressionを破棄
			}

			//]
			if (input[pos].type != TokenType.IndexEnd)
			{
				BeginError(input[pos]);
				messageStream.WriteLine(string.Format("名前つきメモリ[番号]の\"]\"の代わりに\"{0}\"がある。", input[pos].ToString()));
				return null;
			}
			++pos;
			return node;
		}

		//Variable : id
		Node ParseVariable(IList<Token> input, ref int pos)
		{
			Debug.Assert(input[pos].type == TokenType.Name);
			//自分ノード
			var node = new Node();
			//定数か調べる
			var s = input[pos].str;
			int constValue;
			if (constMap.TryGetValue(s, out constValue)) //ある場合
			{
				node.type = NodeType.Number;
				node.number = constValue;
			}
			else
			{
				node.type = NodeType.Variable;
				node.token = input[pos];
			}
			++pos;
			return node;
		}

		Node ParseOut(IList<Token> input, ref int pos)
		{
			Debug.Assert(input[pos].type == TokenType.Out);
			//自分ノード
			var node = new Node();
			node.type = NodeType.Out;
			node.token = input[pos];
			++pos;
			return node;
		}

		//Expression : Expression (+,-,*,/,<,>,!=,== ) Expression
		//左結合の木になる。途中で回転が行われることがある。
		Node ParseExpression(IList<Token> input, ref int pos)
		{	
			//ボトムアップ構築して、左結合の木を作る。
			//最初の左ノードを生成
			var left = ParseTerm(input, ref pos);
			if (left == null)
			{
				return null;
			}

			//演算子がある場合
			while (input[pos].type == TokenType.Operator)
			{
				//ノードを生成
				var node = new Node();
				node.type = NodeType.Expression;
				node.token = input[pos];
				//演算子を設定
				node.operatorType = input[pos].opType;
				Debug.Assert(node.operatorType != Operator.Unknown);
				++pos;
				//連続して演算子なら親切にエラーを吐いてやる。
				if ((input[pos].opType != Operator.Unknown) && (input[pos].opType != Operator.Minus))
				{
					BeginError(input[pos]);
					messageStream.WriteLine("演算子が連続している。==,++,--はないし、=<,=>あたりは<=,>=の間違いだろう。");
					return null;
				}

				//右の子を生成
				var right = ParseTerm(input, ref pos);
				if (right == null)
				{
					return null;
				}
				//GTとGEなら左右を交換
				if ((node.operatorType == Operator.Gt) || (node.operatorType == Operator.Ge))
				{
					var tmp = left;
					left = right;
					right = tmp;
					if (node.operatorType == Operator.Gt)
					{
						node.operatorType = Operator.Lt;
					}
					else
					{
						node.operatorType = Operator.Le;
					}
				}

				//最適化。左右ノードが両方数値なら計算をここでやる
				var preComputedValue = 0;
				var preComputed = false;
				if ((left.type == NodeType.Number) && (right.type == NodeType.Number))
				{
					var valueA = left.number;
					var valueB = right.number;
					switch (node.operatorType)
					{
						case Operator.Plus: preComputedValue = valueA + valueB; break;
						case Operator.Minus: preComputedValue = valueA - valueB; break;
						case Operator.Mul: preComputedValue = valueA * valueB; break;
						case Operator.Div: preComputedValue = valueA / valueB; break;
						case Operator.Eq: preComputedValue = (valueA == valueB) ? 1 : 0; break;
						case Operator.Ne: preComputedValue = (valueA != valueB) ? 1 : 0; break;
						case Operator.Lt: preComputedValue = (valueA < valueB) ? 1 : 0; break;
						case Operator.Le: preComputedValue = (valueA <= valueB) ? 1 : 0; break;
						default: Debug.Assert(false); break; //LE,GEは上で変換されてなくなっていることに注意
					}
					
					if (System.Math.Abs(preComputedValue) <= ImmUtil.GetMaxS(ImmBitCount.I))
					{ //即値に収まる時だけ
						preComputed = true;
					}
				}

				if (preComputed)
				{ //事前計算によるノードマージ
					node.type = NodeType.Number;
					node.number = preComputedValue;
				}
				else
				{ //マージされない。左右を子に持つ
					node.child = left;
					left.brother = right;
				}
				//左の子を現ノードに変更
				left = node;
			}
			return left;
		}

		//Term :[ - ] Function | Variable | Out | ArrayElement | number | '(' Expression ')' | Allocate
		Node ParseTerm(IList<Token> input, ref int pos)
		{
			var minus = false;
			if (input[pos].opType == Operator.Minus)
			{
				minus = true;
				++pos;
			}
			
			var type = GetTermType(input, pos);
			Node node = null;
			if (type == TermType.Expression)
			{
				Debug.Assert(input[pos].type == TokenType.LeftBracket); //getTermTypeで判定済み
				++pos;
				node = ParseExpression(input, ref pos);
				if (input[pos].type != TokenType.RightBracket)
				{ //)が必要
					BeginError(input[pos]);
					messageStream.WriteLine(string.Format("()で囲まれた式がありそうなのだが、終わりの\")\"の代わりに、\"{0}\"がある。\")\"を忘れていないか？", input[pos].ToString()));
					return null;
				}
				++pos;
			}
			else if (type == TermType.Number)
			{
				node = new Node();
				node.type = NodeType.Number;
				node.token = input[pos];
				node.number = node.token.number;
				++pos;
			}
			else if (type == TermType.Function)
			{
				node = ParseFunction(input, ref pos);
			}
			else if (type == TermType.ArrayElement)
			{
				node = ParseArrayElement(input, ref pos);
			}
			else if (type == TermType.Variable)
			{
				node = ParseVariable(input, ref pos);
			}
			else if (type == TermType.Out)
			{
				node = ParseOut(input, ref pos);
			}
			else
			{
				BeginError(input[pos]);
				if (english)
				{
					messageStream.WriteLine(string.Format("()で囲まれた式、memory[]、数、名前つきメモリ、部分プログラム参照のどれかがあるはずの場所に\"{0}\"がある。", input[pos].ToString()));
				}
				else
				{
					messageStream.WriteLine(string.Format("()で囲まれた式、メモリ[]、数、名前つきメモリ、部分プログラム参照のどれかがあるはずの場所に\"{0}\"がある。", input[pos].ToString()));
				}
			}
		
			if ((node != null) && minus)
			{
				//数字ノードになっていれば、マイナスはこの場で処理
				if (node.type == NodeType.Number)
				{
					node.number *= -1;
				}
				else
				{
					node.negation = !node.negation;
				}
			}
			return node;
		}

		void BeginError(Token token)
		{
			messageStream.Write(string.Format("{0}({1})", token.filename, token.line));
		}
	}
} //namespace Sunaba
