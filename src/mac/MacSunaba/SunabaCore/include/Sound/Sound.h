#ifndef INCLUDED_SUNABA_SOUND_SOUND_H
#define INCLUDED_SUNABA_SOUND_SOUND_H

namespace Sunaba{

class Sound{
public:
	Sound(int channelCount, void* windowHandle);
	~Sound();
	void setFrequency(int channel, float frequency);
	void setDumping(int channel, float dumping);
	void play(int channel, float amplitude);
private:
	class Impl;
	Impl* mImpl;
};

} //namespace Sunaba
#endif

