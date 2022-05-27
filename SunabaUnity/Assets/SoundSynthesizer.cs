using System.Collections.Generic;
using UnityEngine;

public class SoundSynthesizer : MonoBehaviour
{
	[SerializeField] float VolumeDb = 0f;
	[SerializeField] int harmonicsCount = 8;

	public void ManualStart(int channelCount)
	{
		oscillators = new Oscillator[channelCount * harmonicsCount];
		deltaTime = 1f / (float)AudioSettings.outputSampleRate;
		goalVolume = currentVolume = 0f;
	}

	public void Play(int channel, float frequency, float dumping, float amplitude)
	{
		goalVolume = 1f;
		if ((channel >= 0) && (channel < oscillators.GetLength(0)))
		{
			for (var i = 0; i < harmonicsCount; i++)
			{
				var f = frequency * (1f + i);
				if (f < ((float)AudioSettings.outputSampleRate * 0.25f))
				{
					var a = amplitude / (1f + i) * (((i % 2) == 0) ? 1f : -1f);
					var d = dumping * (float)AudioSettings.outputSampleRate;
					oscillators[(channel * harmonicsCount) + i].Attack(f, d, a);
				}
			}
		}		
	}

	public void StopAll()
	{
		goalVolume = 0f;
	}

	// MonoBehaviour
	void OnAudioFilterRead(float[] data, int channels)
	{
		var sampleCount = data.Length / channels;
		for (int i = 0; i < sampleCount; i++)
		{
			float v = UpdateOsccilators();
			for (int c = 0; c < channels; c++)
			{
				data[(i * channels) + c] = v;
			}
		}
	}

	// non public -----------
	struct Oscillator
	{
		public void Attack(float f, float d, float a)
		{
			this.d = d;
			var w = f * Mathf.PI * 2f;
			this.k = w * w;
			this.p = a;
			this.v = 0f;
		}

		public float Update(float deltaTime)
		{
			v -= ((v * d) + (p * k)) * deltaTime;
			p += v * deltaTime;
			return p;
		}
		public float p;
		public float v;
		public float k; // Stiffess = 	(f * 2PI)^2;
		public float d;
	}
	Oscillator[] oscillators;
	float deltaTime;
	float currentVolume;
	float goalVolume;

	float UpdateOsccilators()
	{
		currentVolume += ((goalVolume - currentVolume) * deltaTime * 16f);
		var v = 0f;
		for (var i = 0; i < oscillators.Length; i++)
		{
			v += oscillators[i].Update(deltaTime);
		}
		return v * currentVolume;
	}
}
