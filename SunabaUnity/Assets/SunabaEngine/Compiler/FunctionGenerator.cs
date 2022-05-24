using System.Text;
using System.Diagnostics;
using System.Collections.Generic;

namespace Sunaba
{
	public class FunctionGenerator
	{
		public static bool Process(
			StringBuilder output,
			StringBuilder messageStream,
			Node root,
			string name,
			FunctionInfo func,
			Dictionary<string, FunctionInfo> functionMap,
			bool english)
		{
			var generator = new FunctionGenerator(messageStream, name, func, functionMap, english);
			return generator.Process(output, root);
		}
		// non public ------------
		class Variable
		{
			public bool Defined { get; private set; } //実際の行解析時に左辺に現れたらtrueにする。
			public bool Initialized { get; private set; } //始めて左辺に現れた行を解析し終えたところでtrueにする。
			public int Offset { get; private set; } //FP相対オフセット。

			public void Set(int address)
			{
				Offset = address;
			}

			public void Define()
			{
				Defined = true;
			}

			public void Initialize() //初期化済とする
			{
				Initialized = true;
			}
		}
//		typedef std::map<RefString, Variable> VariableMap;

		class Block
		{
			public Block parent;
			public int baseOffset;
			public int frameSize;

			public Block(int baseOffset)
			{
				this.baseOffset = baseOffset;
				this.variables = new Dictionary<string, Variable>();
			}

			public void BeginError(StringBuilder messageStream, Node node)
			{
				var token = node.token;
				Debug.Assert(token != null);
				messageStream.Append(token.filename);
				if (token.line != 0)
				{
					messageStream.AppendFormat("({0}):", token.line);
				}
				else
				{
					messageStream.Append(":");
				}
			}

			public bool AddVariable(string name, bool isArgument = false)
			{
				if (variables.ContainsKey(name)) // もうある
				{
					return false;
				}
				else
				{
					var retVar = new Variable();
					variables.Add(name, retVar);
					retVar.Set(baseOffset + frameSize);
					++frameSize;
					if (isArgument)
					{ //引数なら定義済みかつ初期化済み
						retVar.Define();
						retVar.Initialize();
					}
					return true;
				}
			}

			public void CollectVariables(Node firstStatement)
			{
				var statement = firstStatement;
				while (statement != null)
				{
					//代入文なら、変数名を取得して登録。
					if (statement.type == NodeType.SubstitutionStatement)
					{
						var left = statement.child;
						Debug.Assert(left != null);
						if (left.type == NodeType.Variable)
						{ //変数への代入文のみ扱う。配列要素や定数は無視。
							Debug.Assert(left.token != null);
							var vName = left.token.str;
							var v = FindVariable(vName);
							if (v == null)
							{ //ない。新しい変数を生成
								if (!AddVariable(vName))
								{
									Debug.Assert(false); //ありえん
								}
							}
						}
					}
					statement = statement.brother;
				}
			}

			public Variable FindVariable(string name)
			{
				//まず自分にあるか？
				Variable ret;
				if (variables.TryGetValue(name, out ret))
				{
					return ret;
				}
				else if (parent != null)
				{
					return parent.FindVariable(name);
				}
				else
				{ //こりゃダメだ。
					return null;
				}
			}
			// non public ------
			Dictionary<string, Variable> variables;
		}

		StringBuilder messageStream; //借り物
		Block rootBlock;
		Block currentBlock;
		int labelId;

		string name;
		FunctionInfo info;
		Dictionary<string, FunctionInfo> functionMap;
		bool english;
		bool outputExist;

		FunctionGenerator(
			StringBuilder messageStream,
			string name,
			FunctionInfo info,
			Dictionary<string, FunctionInfo> functionMap,
			bool english)
		{
			this.messageStream = messageStream;
			this.name = name;
			this.info = info;
			this.functionMap = functionMap;
			this.english = english;
		}

		bool Process(
			StringBuilder output,
			Node node)
		{
			var headNode = node; //後でエラー出力に使うのでとっておく。
			//FP相対オフセットを計算
			var argCount = info.ArgCount;
			//ルートブロック生成(TODO:このnew本来不要。コンストラクタでスタックに持つようにできるはず)
			this.currentBlock = rootBlock = new Block(-argCount - 3);  //戻り値、引数*N、FP、CPと詰めたところが今のFP。戻り値の位置は-argcount-3

			//戻り値変数を変数マップに登録
			currentBlock.AddVariable("!ret");

			//引数処理
			//みつかった順にアドレスを割り振りながらマップに登録。
			//呼ぶ時は前からプッシュし、このあとFP,PCをプッシュしたところがSPになる。
			var child = node.child;
			while (child != null)
			{ //このループを抜けた段階で最初のchildは最初のstatementになっている
				if (child.type != NodeType.Variable)
				{
					break;
				}
				Debug.Assert(child.token != null);

				var variableName = child.token.str;
				if (!currentBlock.AddVariable(variableName, isArgument: true))
				{
					BeginError(node);
					messageStream.AppendFormat("部分プログラム\"{0}\"の入力\"{1}\"はもうすでに現れた。二個同じ名前があるのはダメ。\n",
						name,
						variableName);
					return false;
				}
				child = child.brother;
			}
			//FP、CPを変数マップに登録(これで処理が簡単になる)
			currentBlock.AddVariable("!fp");
			currentBlock.AddVariable("!cp");

			//ルートブロックのローカル変数サイズを調べる
			rootBlock.CollectVariables(child);

			//関数開始コメント
			output.AppendFormat("\n#部分プログラム\"{0}\"の開始\n", name);
			//関数開始ラベル
			output.AppendFormat("func_{0}:\n", name); //160413: add等のアセンブラ命令と同名の関数があった時にラベルを命令と間違えて誤作動する問題の緊急回避

			//ローカル変数を確保
			var netFrameSize = currentBlock.frameSize - 3 - argCount; //戻り値、FP、CP、引数はここで問題にするローカル変数ではない。呼出側でプッシュしているからだ。
			if (netFrameSize > 0)
			{
				output.AppendFormat("pop {0} #ローカル変数確保\n", -netFrameSize);
			}

			//中身を処理
			Node lastStatement = null;
			while (child != null)
			{
				if (!GenerateStatement(output, child))
				{
					return false;
				}
				lastStatement = child;
				child = child.brother;
			}
			//関数終了点ラベル。上のループの中でreturnがあればここに飛んでくる。
		//	out->add(mName.pointer(), mName.size());
		//	out->addString(L"_end:\n");

			//ret生成(ローカル変数)
			output.AppendFormat("ret {0} #部分プログラム\"{1}\"の終了\n", netFrameSize, name);
			//出力の整合性チェック。
			//ifの中などで出力してるのに、ブロック外に出力がないケースを検出
			if (info.HasOutputValue != outputExist)
			{
				Debug.Assert(outputExist); //outputExistがfalseで、hasOutputValue()がtrueはありえない
				if (headNode.token != null)
				{ //普通の関数ノード
					BeginError(headNode);
					messageStream.AppendFormat("部分プログラム\"{0}\"は出力したりしなかったりする。条件実行や繰り返しの中だけで出力していないか？\n", name);
				}
				else
				{ //プログラムノード
					Debug.Assert(headNode.child != null);
					BeginError(headNode.child);
					messageStream.Append("このプログラムは出力したりしなかったりする。条件実行や繰り返しの中だけで出力していないか？\n");
				}
				return false;
			}
			return true;			
		}

		bool GenerateStatement(StringBuilder output, Node node)
		{
			//ブロック生成命令は別扱い
			if (
				(node.type == NodeType.WhileStatement) ||
				(node.type == NodeType.IfStatement))
			{
				//新ブロック生成
				var newBlock = new Block(currentBlock.baseOffset + currentBlock.frameSize);
				newBlock.parent = currentBlock; //親差し込み
				currentBlock = newBlock;
				currentBlock.CollectVariables(node.child); //フレーム生成
				//ローカル変数を確保
				if (currentBlock.frameSize > 0)
				{
					output.AppendFormat("pop {0} #ブロックローカル変数確保\n", -(currentBlock.frameSize));
				}

				if (node.type == NodeType.WhileStatement)
				{
					if (!GenerateWhile(output, node))
					{
						return false;
					}
				}
				else if (node.type == NodeType.IfStatement)
				{
					if (!GenerateIf(output, node))
					{
						return false;
					}
				}

				//ローカル変数ポップ
				if (currentBlock.frameSize > 0)
				{
					output.AppendFormat("pop {0} #ブロックローカル変数破棄\n", currentBlock.frameSize);
				}
				currentBlock = currentBlock.parent; //スタック戻し
			}
			else if (node.type == NodeType.SubstitutionStatement)
			{
				if (!GenerateSubstitution(output, node))
				{
					return false;
				}
			}
			else if (node.type == NodeType.Function)
			{ //関数だけ呼んで結果を代入しない文
				if (!GenerateFunctionStatement(output, node))
				{
					return false;
				}
			}
			else if (node.type == NodeType.FunctionDefinition)
			{ //関数定義はもう処理済みなので無視。
				; //スルー
			}
			else
			{
				Debug.Assert(false); //BUG
			}
			return true;
		}

		/*
		ブロック開始処理に伴うローカル変数確保を行い、

		1の間ループ->0ならループ後にジャンプと置き換える。

		whileBegin:
		Expression;
		push 0
		eq
		b whileEnd
		Statement ...
		push 1
		b whileBegin //最初へ
		whileEnd:
		*/
		bool GenerateWhile(StringBuilder output, Node node)
		{
			Debug.Assert(node.type == NodeType.WhileStatement);
			
			//開始ラベル
			var labelIdString = labelId.ToString();
			output.AppendFormat("{0}_whileBegin{1}:\n", name, labelIdString);
			++labelId;

			//Expression処理
			var child = node.child;
			Debug.Assert(child != null);
			if (!GenerateExpression(output, child))
			{ //最初の子はExpression

				return false;
			}

			//いくつかコード生成
			output.AppendFormat("bz {0}_whileEnd{1}\n", name, labelIdString); //-1

			//内部の文を処理
			child = child.brother;
			while (child != null)
			{
				if (!GenerateStatement(output, child))
				{
					return false;
				}
				child = child.brother;
			}

			//ループの最初へ飛ぶジャンプを生成
			output.AppendFormat("j {0}_whileBegin{1} #ループ先頭へ無条件ジャンプ\n", name, labelIdString);
			//ループ終了ラベルを生成
			output.AppendFormat("{0}_whileEnd{1}:\n", name, labelIdString);
			return true;
		}

		bool GenerateIf(StringBuilder output, Node node)
		{
			Debug.Assert(node.type == NodeType.IfStatement);

			//Expression処理
			var child = node.child;
			Debug.Assert(child != null);
			if (!GenerateExpression(output, child))
			{ //最初の子はExpression
				return false;
			}

			//コード生成
			var labelIdString = labelId.ToString();
			output.AppendFormat("bz {0}_ifEnd{1}\n", name, labelIdString);
			++labelId;
			
			//内部の文を処理
			child = child.brother;
			while (child != null)
			{
				if (!GenerateStatement(output, child))
				{
					return false;
				}
				child = child.brother;
			}
	
			//ラベル生成
			output.AppendFormat("{0}_ifEnd{1}:\n", name, labelIdString);
			return true;
		}

		bool GenerateFunctionStatement(StringBuilder output, Node node)
		{
			//まず関数呼び出し
			if (!GenerateFunction(output, node, isStatement: true))
			{
				return false;
			}
			//関数の戻り値がプッシュされているので捨てます。
		//	out->addString(L"pop 1 #戻り値を使わないので、破棄\n");
			return true;
		}

		bool GenerateFunction(StringBuilder output, Node node, bool isStatement)
		{
			Debug.Assert(node.type == NodeType.Function);
			//まず、その関数が存在していて、定義済みかを調べる。
			Debug.Assert(node.token != null);
			var funcName = node.token.str;

			FunctionInfo func = null;
			if (!functionMap.TryGetValue(funcName, out func))
			{
				BeginError(node);
				messageStream.AppendFormat("部分プログラム\"{0}\"なんて知らない。\n", funcName);
				return false;
			}

			var popCount = 0; //後で引数/戻り値分ポップ
			if (func.HasOutputValue)
			{ //戻り値あるならプッシュ
				output.AppendFormat("pop -1 #{0}の戻り値領域\n", funcName);
				if (isStatement)
				{ //戻り値を使わないのでポップ数+1
					++popCount;
				}
			}
			else if (!isStatement)
			{ //戻り値がないなら式の中にあっちゃだめ
				BeginError(node);
				messageStream.AppendFormat("部分プログラム\"{0}\"は、\"出力\"か\"out\"という名前付きメモリがないので、出力は使えない。ifやwhileの中にあってもダメ。\n", funcName);
				return false;
			}

			//引数の数をチェック
			var arg = node.child;
			var argCount = 0;
			while (arg != null)
			{
				++argCount;
				arg = arg.brother;
			}
			popCount += argCount; //引数分追加
			if (argCount != func.ArgCount)
			{
				BeginError(node);
				messageStream.AppendFormat("部分プログラム\"{0}\"は、入力を{1}個受け取るのに、ここには{2}個ある。間違い。\n", funcName, func.ArgCount, argCount);
				return false;
			}
			//引数を評価してプッシュ
			arg = node.child;
			while (arg != null)
			{
				if (!GenerateExpression(output, arg))
				{
					return false;
				}
				arg = arg.brother;
			}

			//call命令生成
			output.AppendFormat("call func_{0}\n", funcName); //160413: add等のアセンブラ命令と同名の関数があった時にラベルを命令と間違えて誤作動する問題の緊急回避

			//返ってきたら、引数/戻り値をポップ
			if (popCount > 0)
			{
				output.AppendFormat("pop {0} #引数/戻り値ポップ\n", popCount);
			}			
			return true;
		}

		/*
		LeftValue
		Expression
		st
		*/
		bool GenerateSubstitution(StringBuilder output, Node node)
		{
			Debug.Assert(node.type == NodeType.SubstitutionStatement);
			//左辺値のアドレスを求める。最初の子が左辺値
			var child = node.child;
			Debug.Assert(child != null);
			//変数の定義状態を参照
			Variable var = null;
			if ((child.type == NodeType.Out) || (child.token != null))
			{ //変数があるなら
				var name = child.token.str;
				if (child.type == NodeType.Out)
				{
					name = "!ret";
				}
				var = currentBlock.FindVariable(name);
				if (var == null)
				{ //配列アクセス時でタイプミスすると変数が存在しないケースがある
					BeginError(child);
					messageStream.AppendFormat("名前付きメモリか定数\"{0}\"は存在しないか、まだ作られていない。\n", name);
					return false; 
				}
				else if (!(var.Defined))
				{ //未定義ならここで定義
					var.Define();
				}
			}
			int staticOffset;
			bool fpRelative;
			if (!PushDynamicOffset(output, out staticOffset, out fpRelative, child))
			{
				return false;
			}

			//右辺処理
			child = child.brother;
			Debug.Assert(child != null);
			if (!GenerateExpression(output, child))
			{
				return false;
			}

			if (fpRelative)
			{
				output.AppendFormat("fst {0} #\"{1}\"へストア\n", staticOffset, node.token.str);
			}
			else
			{
				output.AppendFormat("st {0} #\"{1}\"へストア\n", staticOffset, node.token.str);
			}

			//左辺値は初期化されたのでフラグをセット。すでにセットされていても気にしない。
			if (var != null)
			{
				var.Initialize();
			}
			return true;
		}

		//第一項、第二項、第二項オペレータ、第三項、第三項オペレータ...という具合に実行
		bool GenerateExpression(StringBuilder output, Node node)
		{
			//解決されて単項になっていれば、そのままgenerateTermに丸投げ。ただし単項マイナスはここで処理。
			var ret = false;
			if (node.type != NodeType.Expression)
			{
				ret = GenerateTerm(output, node);
			}
			else
			{
				if (node.negation)
				{
					output.Append("i 0 #()に対する単項マイナス用\n"); //0をプッシュ
				}
				//項は必ず二つある。
				Debug.Assert(node.child != null);
				Debug.Assert(node.child.brother != null);
				if (!GenerateTerm(output, node.child))
				{
					return false;
				}

				if (!GenerateTerm(output, node.child.brother))
				{
					return false;
				}

				//演算子を適用
				string opStr = null;
				switch (node.operatorType)
				{
					case Operator.Plus: opStr = "add"; break;
					case Operator.Minus: opStr = "sub"; break;
					case Operator.Mul: opStr = "mul"; break;
					case Operator.Div: opStr = "div"; break;
					case Operator.Lt: opStr = "lt"; break;
					case Operator.Le: opStr = "le"; break;
					case Operator.Eq: opStr = "eq"; break;
					case Operator.Ne: opStr = "ne"; break;
					default: Debug.Assert(false); break; //これはParserのバグ。とりわけ、LE、GEは前の段階でGT,LTに変換されていることに注意
				}

				output.AppendFormat("{0}\n", opStr);
				//単項マイナスがある場合、ここで減算
				if (node.negation)
				{
					output.Append("sub #()に対する単項マイナス用\n");
				}
				ret = true;
			}
			return ret;
		}

		//右辺値。
		bool GenerateTerm(StringBuilder output, Node node)
		{
			//単項マイナス処理0から引く
			if (node.negation)
			{
				output.Append("i 0 #単項マイナス用\n"); //0をプッシュ
			}

			//タイプで分岐
			if (node.type == NodeType.Expression)
			{
				if (!GenerateExpression(output, node))
				{
					return false;
				}
			}
			else if (node.type == NodeType.Number)
			{ //数値は即値プッシュ
				output.AppendFormat("i {0} #即値プッシュ\n", node.number);
			}
			else if (node.type == NodeType.Function)
			{
				if (!GenerateFunction(output, node, isStatement: false))
				{
					return false;
				}
			}
			else
			{ //ARRAY_ELEMENT,VARIABLEのアドレスプッシュ処理
				//変数の定義状態を参照
				Variable var = null;
				if (node.token != null)
				{ //変数があるなら
					var name = node.token.str;
					if (node.type == NodeType.Out)
					{
						name = "!ret";
					}
					var = currentBlock.FindVariable(name);
					//知らない変数。みつからないか、あるんだがまだその行まで行ってないか。				
					if (var == null)
					{
						BeginError(node);
						messageStream.AppendFormat("名前付きメモリか定数\"{0}\"は存在しない。\n", name);
						return false; 
					}

					if (!var.Defined)
					{
						BeginError(node);
						messageStream.AppendFormat("名前付きメモリ\"{0}\"はまだ作られていない。\n", name);
						return false; //まだ宣言してない
					}

					if (!(var.Initialized))
					{
						BeginError(node);
						messageStream.AppendFormat("名前付きメモリ\"{0}", name);
						if (english)
						{
							messageStream.Append("\"は数をセットされる前に使われている。「a->a」みたいなのはダメ。\n");
						}
						else
						{
							messageStream.Append("\"は数をセットされる前に使われている。「a→a」みたいなのはダメ。\n");
						}
						return false; //まだ宣言してない
					}
				}

				int staticOffset;
				bool fpRelative;
				if (!PushDynamicOffset(output, out staticOffset, out fpRelative, node))
				{
					return false;
				}

				if (fpRelative)
				{
					output.AppendFormat("fld {0}", staticOffset);
				}
				else
				{
					output.AppendFormat("ld {0}", staticOffset);
				}
	
				if (node.token != null)
				{
					output.AppendFormat(" #変数\"{0}\"からロード\n", node.token.str);
				}
				else
				{
					output.Append("\n");
				}
			}

			//単項マイナスがある場合、ここで減算
			if (node.negation)
			{
				output.Append("sub #単項マイナス用\n");
			}
			return true;
		}

		//添字アクセスがあれば、添字を計算してpush、addを追加する。また、変数そのもののオフセットと、絶対アドレスか否か(memory[]か否か)を返す
		bool PushDynamicOffset(
			StringBuilder output,
			out int staticOffset,
			out bool fpRelative,
			Node node)
		{
			fpRelative = false;
			staticOffset = -0x7fffffff; //あからさまにおかしな値を入れておく。デバグのため。
			Debug.Assert((node.type == NodeType.Out) || (node.type == NodeType.Variable) || (node.type == NodeType.ArrayElement));

			//トークンは数字ですか、名前ですか
			if (node.token != null)
			{
				if (node.token.type == TokenType.Out)
				{
					var var = currentBlock.FindVariable("!ret");
					Debug.Assert(var != null);
					fpRelative = true; //変数直のみFP相対
					staticOffset = var.Offset;
					outputExist = true;
				}
				else if (node.token.type == TokenType.Name)
				{
					//変数の定義状態を参照
					var name = node.token.str;
					var var = currentBlock.FindVariable(name);
					//配列ならExpressionを評価してプッシュ
					if (node.type == NodeType.ArrayElement)
					{
						output.AppendFormat("fld {0} #ポインタ\"{1}\"からロード\n", var.Offset, name);

						if (node.child != null)
						{ //変数インデクス
							if (!GenerateExpression(output, node.child))
							{ //アドレスオフセットがプッシュされる
								return false;
							}
							output.Append("add\n");
							staticOffset = 0;
						}
						else
						{ //定数インデクス
							staticOffset = node.number;
						}
					}
					else
					{
						fpRelative = true; //変数直のみFP相対
						staticOffset = var.Offset;
					}
				}
			}
			else
			{ //定数アクセス。トークンがない。
				Debug.Assert(node.type == NodeType.ArrayElement); //インデクスがない定数アクセスはアドレスではありえない。
				if (node.child != null)
				{ //変数インデクス
					if (!GenerateExpression(output, node.child))
					{ //アドレスをプッシュ
						return false;
					}
				}
				else
				{
					output.Append("i 0 #絶対アドレスなので0\n"); //絶対アドレスアクセス
				}
				staticOffset = node.number;
			}
			return true;
		}

		void BeginError(Node node)
		{
			var token = node.token;
			Debug.Assert(token != null);
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