#ifndef INCLUDED_SUNABA_SOUND_SOUNDCHANNEL_H
#define INCLUDED_SUNABA_SOUND_SOUNDCHANNEL_H

namespace Sunaba{

class SoundChannel{ //ベレ積分
public:
	SoundChannel() :
	mPulseVelocity(0.f),
	mSpring(0.f),
	mFrequency(0.f),
	mDumping(0.f),
	mPosition(0.f),
	mOldPosition(0.f){
	}
	~SoundChannel(){
	}
	void setFrequency(float f, int samplingFrequency){
		float sqrtK = f * (6.28318530718f / static_cast<float>(samplingFrequency));
		mSpring = sqrtK * sqrtK;
		mFrequency = f;
	}
	void setDumping(float d){
		mDumping = d;
	}
	void play(float a, int samplingFrequency){
		float sqrtK = mFrequency * (6.28318530718f / static_cast<float>(samplingFrequency));
		mPulseVelocity = a * sqrtK;
	}
	void startCalculation(){
		float v = mPosition - mOldPosition;
		if (v > 0.f){
			mPosition += mPulseVelocity;
		}else{
			mPosition -= mPulseVelocity;
		}
		mPulseVelocity = 0.f;
	}
	float calculate(){
		float p1 = mPosition;
		float p0 = mOldPosition;
		float a = (-mSpring * mPosition) + (-mDumping * (p1 - p0));
		float p2 = p1 + p1 - p0 + a;
		mOldPosition = p1;
		mPosition = p2;
		return p2;
	}
private:
	float mPulseVelocity;
	float mSpring;
	float mFrequency;
	float mDumping;
	float mPosition;
	float mOldPosition;
};

} //namespace Sunaba

#endif

