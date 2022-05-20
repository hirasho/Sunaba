using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace Sunaba
{
	public class Main : MonoBehaviour
	{
		[SerializeField] RawImage appWindow;
		[SerializeField] Text messageWindow;
		[SerializeField] Button rebootButton;
		[SerializeField] InputField filenameInput;

		void Start()
		{
			rebootButton.onClick.AddListener(OnClickReboot);
			pixels = new Texture2D(100, 100);
			messageStream = new System.IO.MemoryStream();
			messageStreamWriter = new System.IO.StreamWriter(messageStream);
		}

		void Update()
		{
			// UpdateごとにappWindowのテクスチャを送信する
	//		pixels.
			
			
		}

		// non public --------
		Texture2D pixels;
		Machine machine;
		System.IO.StreamWriter messageStreamWriter;
		System.IO.MemoryStream messageStream;

		void OnClickReboot()
		{
			// ファイル名取って
			// 開けて読んで
			// コンパイラに渡して
			// アセンブラに渡して
			// Machineを起動
//			machine = new Machine();			
		}
	}
}