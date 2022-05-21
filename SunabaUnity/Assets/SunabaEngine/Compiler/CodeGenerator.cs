using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

namespace Sunaba
{
	public class CodeGenerator
	{
		public static bool Process(
			StringBuilder output,
			System.IO.StreamWriter messageStream,
			Node root,
			bool english,
			bool outputIntermediates)
		{
			var generator = new CodeGenerator(messageStream, english);
			return generator.Process(output, root, outputIntermediates);
		}

		// non public --------------
		System.IO.StreamWriter messageStream; //借り物
		Dictionary<string, FunctionInfo> functionMap; //関数マップ。これはグローバル。
		bool english;

		CodeGenerator(System.IO.StreamWriter messageStream, bool englishGrammer)
		{
			this.messageStream = messageStream;
			this.english = english;
		}

		bool Process(StringBuilder output, Node root, bool outputIntermediates)
		{
			if (!GenerateProgram(output, root))
			{ //エラー
				return false;
			}

			//デバグ出力
			if (outputIntermediates)
			{
				System.IO.File.WriteAllText("compiled.txt", output.ToString());				
			}
			return true;
		}

		bool GenerateProgram(StringBuilder output, Node node)
		{
			Debug.Assert(node.type == NodeType.Program);
			output.Append("pop -1 #$mainの戻り値領域\n");
			output.Append("call func_!main\n"); //main()呼び出し 160413: add等のアセンブラ命令と同名の関数があった時にラベルを命令と間違えて誤作動する問題の緊急回避
			output.Append("j !end #プログラム終了点へジャンプ\n"); //プログラム終了点へジャンプ

			//$mainの情報を足しておく
			var mainFuncInfo = new FunctionInfo();
			functionMap.Add("!main", mainFuncInfo);

			//関数情報収集。関数コールを探しだして、見つけ次第、引数、出力、名前についての情報を収集してmapに格納
			Node child = node.child;
			while (child != null)
			{
				if (child.type == NodeType.FunctionDefinition)
				{
					if (!CollectFunctionDefinitionInformation(child))
					{ //main以外
						return false;
					}
				}
				child = child.brother;
			}

			//関数コールを探しだして、見つけ次第コード生成
			child = node.child;
			while (child != null)
			{
				if (child.type == NodeType.FunctionDefinition)
				{
					if (!GenerateFunctionDefinition(output, child))
					{ //main以外
						return false;
					}
				}
				else if (child.IsOutputValueSubstitution())
				{ //なければ出力があるか調べる
					mainFuncInfo.SetHasOutputValue(); //戻り値があるのでフラグを立てる。
				}
				child = child.brother;
			}

			//あとはmain
			if (!GenerateFunctionDefinition(output, node))
			{
				return false;
			}

			//最後にプログラム終了ラベル
			output.Append("\n!end:\n");
			output.Append("pop 1 #!mainの戻り値を破棄。最終命令。なくてもいいが。\n");
			return true;
		}

		bool GenerateFunctionDefinition(StringBuilder output, Node node)
		{
			//まず、関数マップに項目を足す
			string funcName;
			if (node.token != null)
			{
				funcName = node.token.str;
			}
			else
			{
				funcName = "!main";
			}

			//関数重複チェック
			FunctionInfo funcInfo;
			if (functionMap.TryGetValue(funcName, out funcInfo))
			{
				if (!FunctionGenerator.Process(
					output, 
					messageStream, 
					node, 
					funcName, 
					funcInfo, 
					functionMap, 
					english))
				{
					return false;
				}
			}
			else
			{
				Debug.Assert(false);
				return false; // バグ
			}
			return true;
		}
		
		bool CollectFunctionDefinitionInformation(Node node)
		{
			var argCount = 0; //引数の数
			//まず、関数マップに項目を足す
			var child = node.child;
			Debug.Assert(node.token != null);
			var funcName = node.token.str;

			//関数重複チェック
			if (functionMap.ContainsKey(funcName))
			{
				BeginError(node);
				messageStream.WriteLine(string.Format("部分プログラム\"{0}\"はもう作られている。", funcName));
				return false;
			}

			var funcInfo = new FunctionInfo();
			functionMap.Add(funcName, funcInfo);

			//引数の処理
			//まず数を数える
			{ //argが後ろに残ってるとバグ源になるので閉じ込める
				var arg = child; //childは後で必要なので、コピーを操作
				while (arg != null)
				{
					if (arg.type != NodeType.Variable)
					{
						break;
					}
					++argCount;
					arg = arg.brother;
				}
			}
			funcInfo.SetArgCount(argCount);

			//出力値があるか調べる
			Node lastStatement = null;
			while (child != null)
			{
				if (child.IsOutputValueSubstitution())
				{
					funcInfo.SetHasOutputValue(); //戻り値があるのでフラグを立てる。
				}
				lastStatement = child;
				child = child.brother;
			}
			return true;
		}
		
		void BeginError(Node node)
		{
			var token = node.token;
			Debug.Assert(token != null);
			messageStream.Write(token.filename);
			if (token.line != 0)
			{
				messageStream.Write(string.Format("({0})", token.line));
			}
			else
			{
				messageStream.Write(' ');
			}
		}
	}
} //namespace Sunaba