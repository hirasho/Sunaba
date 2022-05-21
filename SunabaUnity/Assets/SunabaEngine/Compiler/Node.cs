namespace Sunaba
{
	public enum StatementType
	{
		WhileOrIf,
		Def,
		Const,
		Function,
		Substitution,
		
		Unknown,
	}

	public enum TermType
	{
		Expression,
		Number,
		Function,
		ArrayElement,
		Variable,
		Out,
		
		Unknown,
	}

	//構文木 
	//Statementは明示的に節は作らない。Statement = [While | If | Substitution | Function | Return] main以外ではConstは禁止
	public enum NodeType
	{ //コメントは子ノードについて
		Program, //[Statement | FunctionDefinition] ...
		//Statement
		WhileStatement, // Expression,Statement...
		IfStatement, // Expression,Statement...
		SubstitutionStatement, //[ Memory | Variable | ArrayElement ] ,Expression
		FunctionDefinition, //Variable... Statement... [ Return ]
		Expression, //Expression, Expression
		Variable,
		Number,
		Out,
		ArrayElement, //Expression
		Function, //Expression ...

		Unknown,
	}

	public class Node
	{
		public NodeType type = NodeType.Unknown;
		public Node child;
		public Node brother;
		//ノード特性
		public Operator operatorType = Operator.Unknown; //Expressionの時のみ二項演算子
		public Token token; //何かしらのトークン
		public bool negation; //項のときに反転するか。
		public int number; //定数値、定数[]のアドレス

		public bool IsOutputValueSubstitution()
		{
			return (token.type == TokenType.Out);
		}
	}
} //namespace Sunaba
