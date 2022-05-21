namespace Sunaba
{
	public class FunctionInfo
	{	
		public int ArgCount { get; private set; }
		public bool HasOutputValue { get; private set; }

		public void SetHasOutputValue()
		{
			HasOutputValue = true;
		}

		public void SetArgCount(int count)
		{
			ArgCount = count;
		}
	}
} //namespace Sunaba
