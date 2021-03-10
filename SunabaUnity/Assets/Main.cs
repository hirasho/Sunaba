using System.Collections;
using System.Collections.Generic;
using UnityEngine;
public class Main : MonoBehaviour
{
	[SerializeField]
	UnityEngine.UI.RawImage screen;

	Texture2D texture;

	void Start()
	{
		this.texture = new Texture2D(100, 100, TextureFormat.ARGB32, mipChain: false, linear: true);
		this.screen.texture = this.texture;
	}

	void Update()
	{

	}
}
