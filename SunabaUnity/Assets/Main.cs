using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Sunaba
{
	public class Main : MonoBehaviour
	{
		[SerializeField] RawImage appWindow;
		[SerializeField] Text messageWindow;
		[SerializeField] Button rebootButton;

		void Start()
		{
			pixels = new Texture2D(100, 100);
			messageStream = new System.IO.MemoryStream();
			messageStreamWriter = new System.IO.StreamWriter(messageStream);
		}

		void Update()
		{
			// UpdateごとにappWindowのテクスチャを送信する
	//		pixels.
			
			
		}

		void OnProgramDrop()
		{
//			machine = new Machine();
		}

		// non public --------
		Texture2D pixels;
		Machine machine;
		System.IO.StreamWriter messageStreamWriter;
		System.IO.MemoryStream messageStream;
	}
}