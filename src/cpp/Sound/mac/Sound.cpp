#include "Base/Base.h"
#include "Sound/Sound.h"
#include "Sound/SoundChannel.h"
#include "Base/Os.h"

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <cstring>

namespace Sunaba {
    class Sound::Impl{
    public:
        enum{
            FREQUENCY = 48000,
            BUFFER_SIZE = FREQUENCY / 15,
            BUFFER_COUNT = 2,
        };

        Impl( int channelCount, void* ) :
        mDevice(NULL),
        mContext(NULL),
        mBufferIndex(0),
        mChannels(0),
        mChannelCount(0),
        mDropCount(0),
        mEnabled(false){
            mDevice = alcOpenDevice( NULL );
            mContext = alcCreateContext( mDevice, NULL );
            alcMakeContextCurrent( mContext );

            alGenSources( 1, &mMasteringVoice );
            alGenBuffers( 2, mSourceVoice );
            
            mChannelCount = channelCount;
            mChannels = new SoundChannel[mChannelCount];
            mFreeBuffIndex = 0;
            
            for( int i = 0; i < BUFFER_COUNT; ++i ) {
                mBuffers[i] = new short[BUFFER_SIZE];
                std::memset( mBuffers[i], 0, BUFFER_SIZE * sizeof(short) );
            }
            for( int i = 0; i < BUFFER_COUNT; ++i ) {
                ALenum format = AL_FORMAT_MONO16; // 16bit モノラル.
                ALsizei size  = sizeof(short) * BUFFER_SIZE;
                ALsizei sampling = FREQUENCY;
                alBufferData( mSourceVoice[i], format, mBuffers[i], size, sampling );
                alSourceQueueBuffers( mMasteringVoice, 1, &mSourceVoice[i] );
            }
            alSourcePlay( mMasteringVoice );
            mPrevFeedTime = getTimeInMilliSecond();
            //スレッド用意.
            mFeedingThread.start(this);
            mEnabled = true;
        }

        ~Impl() {
            if( !mEnabled ) { //初期化そもそもしてないから抜ける。
                return;
            }
            mEndRequestEvent.set();
            mFeedingThread.wait();
            
            delete[] mChannels;
            mChannels = 0;

            alcMakeContextCurrent( mContext );
            alSourceStop( mMasteringVoice );
            alDeleteSources( 1, &mMasteringVoice );
            alDeleteBuffers( 2, mSourceVoice );

            alcMakeContextCurrent( NULL );
            alcDestroyContext( mContext );
            alcCloseDevice( mDevice );
            mDevice = NULL;
        }

        void feedData() {
            ASSERT( mEnabled );
            for( int i = 0; i < mChannelCount; ++i ) {
                mChannels[i].startCalculation();
            }

            alcMakeContextCurrent( mContext );
            
            short* data = mBuffers[mBufferIndex];
            fill( data, BUFFER_SIZE );
            
            ALenum format = AL_FORMAT_MONO16;
            ALsizei size = sizeof(short) * BUFFER_SIZE;
            ALsizei sampling = FREQUENCY;
            alBufferData( mSourceVoice[mBufferIndex], format, data, size, sampling );
            alSourceQueueBuffers( mMasteringVoice, 1, &(mSourceVoice[mBufferIndex]) );
            
            ALint state = 0;
            alGetSourcei( mMasteringVoice, AL_SOURCE_STATE, &state );
            if( state != AL_PLAYING ) {
                alSourcePlay( mMasteringVoice );
                writeToConsole( L"drop..?\n" );
            }

            ++mBufferIndex;
            if( mBufferIndex == BUFFER_COUNT ) {
                mBufferIndex = 0;
            }
        }

        void setFrequency( int c, float f){
            if( !mEnabled ) {
                return;
            }
            ASSERT( c < mChannelCount );
            mChannels[c].setFrequency( f, FREQUENCY );
        }
        void setDumping( int c, float d){
            if( !mEnabled ) {
                return;
            }
            ASSERT( c < mChannelCount );
            d *= (48000.0f / static_cast<float>(FREQUENCY));
            mChannels[c].setDumping(d);
        }
        void play( int c, float s ){
            if( !mEnabled ){
                return;
            }
            ASSERT( c < mChannelCount );
            mChannels[c].play( s, FREQUENCY );
        }
        void fill(short* data, int count ) {
            for( int i = 0; i < count; ++i ) {
                float p = 0.f;
                for( int j = 0; j < mChannelCount; ++j ) {
                    p += mChannels[j].calculate();
                    if( p > 0.f ) {
                        p = p / (1.f + p);
                    } else if( p < 0.f ) {
                        p = -p / (-1.f + p );
                    }
                    ASSERT( p < 1.f && p > -1.0f );
                }
                *data = static_cast<short>( p * 32767.f );
                ++data;
            }
        }

        void bufferUpdate() {
            // 空きバッファ（使用完了）があるか？
            int freeBuf = 0;
            alGetSourcei( mMasteringVoice, AL_BUFFERS_PROCESSED, &freeBuf );
            if( freeBuf ) {
                unsigned int freeId = mBufferIndex;
                alSourceUnqueueBuffers( mMasteringVoice, 1, &(mSourceVoice[freeId]) );
                feedData();
            }
        }
        void feed() {
            unsigned int time = getTimeInMilliSecond();
            int freeBufferCount = 0;

            alcMakeContextCurrent( mContext );
            
            alGetSourcei( mMasteringVoice, AL_BUFFERS_PROCESSED, &freeBufferCount );
            if( freeBufferCount == 0 ) {
                return; /* 空バッファがないので次回. */
            }
            if( time == mPrevFeedTime ) {
                return;
            }
            
            /* 空バッファをキューから外す. */
            alSourceUnqueueBuffers( mMasteringVoice, 1, &(mSourceVoice[mFreeBuffIndex]) );
            
            
            unsigned int timeDiff = static_cast<unsigned int>( time - mPrevFeedTime );
            mPrevFeedTime = time;
            
            int t = (FREQUENCY * timeDiff) + mDelayedSampleCount;
            int totalSrcWriteSize = t / 1000;
            mDelayedSampleCount = t -= (totalSrcWriteSize * 1000);
            int srcBufferSize = BUFFER_SIZE;
            if( totalSrcWriteSize > srcBufferSize ) { //処理落ち。仕方ない.
                totalSrcWriteSize = srcBufferSize;
                ++mDropCount;
            }
            
            for( int i = 0; i < mChannelCount; ++i ) {
                mChannels[i].startCalculation();
            }

            short* data = mBuffers[ mFreeBuffIndex ];
            fill( data, totalSrcWriteSize );
            alBufferData( mSourceVoice[ mFreeBuffIndex ], AL_FORMAT_MONO16, data, sizeof(short)*totalSrcWriteSize, FREQUENCY );
            alSourceQueueBuffers( mMasteringVoice, 1, &(mSourceVoice[mFreeBuffIndex]) );

            ALint state = 0;
            alGetSourcei( mMasteringVoice, AL_SOURCE_STATE, &state );
            if( state != AL_PLAYING ) {
                alSourcePlay( mMasteringVoice );
            }

            mFreeBuffIndex = 1 - mFreeBuffIndex;
        }

        class FeedingThread : public Thread{
        public:
            FeedingThread() : Thread(true), mImpl(0) {
            }
            void start( Sound::Impl* soundImpl ) {
                mImpl = soundImpl;
                Thread::start();
            }
            virtual void operator()() {
                mImpl->feedThreadFunc();
            }
        private:
            Sound::Impl* mImpl;
        };

        void feedThreadFunc() {
            bool foreverTrue = true;
            int stopCount = 0;
            while( foreverTrue ) {
                if( mEndRequestEvent.isSet() ) {
                    for( int i = 0; i < mChannelCount; ++i ){
                        mChannels[i].setDumping(0.01f);
                    }
                    ++stopCount;
                    if( stopCount == 20 ) {
                        // Buffer->Stop();
                        break;
                    }
                }
                feed();
                sleepMilliSecond( 5 );
            }
            
        }

        ALCdevice* mDevice;
        ALCcontext* mContext;
        /* AL流儀ではなく XAudio2 の命名に合わせておく. */
        ALuint     mSourceVoice[2]; 
        ALuint     mMasteringVoice;
        
        short* mBuffers[BUFFER_COUNT];
        int    mBufferIndex;
        SoundChannel* mChannels;
        int    mChannelCount;
        bool   mEnabled;
        unsigned int mPrevFeedTime;
        unsigned int mDropCount;
        unsigned int mDelayedSampleCount;

        int    mFreeBuffIndex;
        
        FeedingThread mFeedingThread;
        Event  mEndRequestEvent;
    };
    
    Sound::Sound(int channelCount, void* windowHandle) : mImpl(0){
        mImpl = new Impl(channelCount, windowHandle);
    }
    
    Sound::~Sound(){
        DELETE(mImpl);
        mImpl = 0;
    }
    
    void Sound::setFrequency(int channel, float frequency){
        mImpl->setFrequency(channel, frequency);
    }
    
    void Sound::setDumping(int channel, float dumping){
        mImpl->setDumping(channel, dumping);
    }
    
    void Sound::play(int channel, float strength){
        mImpl->play(channel, strength);
    }

 //   void Sound::bufferUpdate() {
        // mImpl->bufferUpdate();
        // use Threading..
 //   }
}

#if 0
#ifdef SUNABA_USE_XAUDIO2
#include <Xaudio2.h>
#pragma comment(lib, "xapobase.lib")
#undef DELETE
#elif defined(SUNABA_USE_DSOUND)
#include <Dsound.h>
#pragma comment(lib, "Dsound.lib")
#pragma comment(lib, "dxguid.lib")
#undef DELETE
#endif
#include "Sound/Sound.h"
#include "Base/Os.h"
#include "Base/Base.h"
#include "Sound/SoundChannel.h"

namespace Sunaba{

class Sound::Impl{
public:
	enum{
		FREQUENCY = 48000,
		BUFFER_SIZE = FREQUENCY / 60,
		BUFFER_COUNT = 2,
	};
	void setFrequency(int c, float f){
		if (mEnabled){
			ASSERT(c < mChannelCount);
			mChannels[c].setFrequency(f, FREQUENCY);
		}
	}
	void setDumping(int c, float d){
		if (mEnabled){
			ASSERT(c < mChannelCount);
			d *= (48000.f / static_cast<float>(FREQUENCY));
			mChannels[c].setDumping(d);
		}
	}
	void play(int c, float s){
		if (mEnabled){
			ASSERT(c < mChannelCount);
			mChannels[c].play(s, FREQUENCY);
		}
	}
	void fill(short* data, int count){
		for (int i = 0; i < count; ++i){
			float p = 0.f;
			for (int j = 0; j < mChannelCount; ++j){
				p += mChannels[j].calculate();
				//トーンマッピング的な何か
				if (p > 0.f){
					p = p / (1.f + p);
				}else if (p < 0.f){
					p = -p / (-1.f + p);
				}
				ASSERT(p < 1.f && p > -1.f);
			}
			*data = static_cast<short>(p * 32767.f);
			++data;
		}
	}
#ifdef SUNABA_USE_XAUDIO2
	class Callback : public IXAudio2VoiceCallback{
	public:
		void __stdcall OnStreamEnd(){}
		void __stdcall OnVoiceProcessingPassEnd(){}
		void __stdcall OnVoiceProcessingPassStart(UINT32){}
		void __stdcall OnBufferEnd(void* context){
			Impl* impl = static_cast<Impl*>(context);
			impl->feedData();
		}
		void __stdcall OnBufferStart(void*){}
		void __stdcall OnLoopEnd(void*){}
		void __stdcall OnVoiceError(void*, HRESULT){}
	};
	Impl(int channelCount, void*) : 
	mXAudio2(0), 
	mMasteringVoice(0),
	mSourceVoice(0),
	mBufferIndex(0),
	mChannels(0),
	mChannelCount(0),
	mEnabled(false){
		HRESULT hr = XAudio2Create(&mXAudio2, 0);
		if (FAILED(hr)){
			char buf[1024];
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				buf,
				1024,
				NULL);
			WRITE_LOG(buf);
			mXAudio2 = 0;
			return;
		}
		//サウンドあるの？
		UINT32 count;
		hr = mXAudio2->GetDeviceCount(&count);
		if (FAILED(hr) || (count == 0)){
			WRITE_LOG("IXAudio2::GetDeviceCount failed");
			return;
		}
		hr = mXAudio2->CreateMasteringVoice(
			&mMasteringVoice,
			1); //モノラル
		if (FAILED(hr)){
			WRITE_LOG("IXAudio2::CreateMasteringVoice failed");
			return;
		}
		mChannelCount = channelCount;
		mChannels = new SoundChannel[mChannelCount];
		WAVEFORMATEX wfx;
		memset(&wfx, 0, sizeof(WAVEFORMATEX));
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = 1;
		wfx.nSamplesPerSec = FREQUENCY;
		wfx.wBitsPerSample = 16; //short決め打ち
		wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		hr = mXAudio2->CreateSourceVoice(
			&mSourceVoice, 
			&wfx,
			XAUDIO2_VOICE_NOPITCH,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			&mCallback);
		if (FAILED(hr)){
			WRITE_LOG("IXAudio2::CreateSourceVoice failed");
			return;
		}
		for (int i = 0; i < BUFFER_COUNT; ++i){
			mBuffers[i] = new short[BUFFER_SIZE];
			memset(mBuffers[i], 0, BUFFER_SIZE * sizeof(short));
		}
		//初回充填して再生開始。バッファ数だけ無音を放りこむ
		XAUDIO2_BUFFER buffer;
		memset(&buffer, 0, sizeof(XAUDIO2_BUFFER));
		buffer.AudioBytes = BUFFER_SIZE * sizeof(short);
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
		buffer.LoopLength = 0;
		buffer.LoopCount = 0;
		buffer.pContext = this;
		buffer.Flags = 0;
		for (int i = 0; i < BUFFER_COUNT; ++i){
			buffer.pAudioData = reinterpret_cast<BYTE*>(mBuffers[i]);
			mSourceVoice->SubmitSourceBuffer(&buffer);
		}
		mSourceVoice->Start(0);
		mEnabled = true;
	}
	~Impl(){
		if (!mEnabled){ //初期化そもそもしてないから抜ける。
			return;
		}
		mEndRequestEvent.set();
		while (!mEndEvent.isSet()){
			sleepMilliSecond(10);
		}
		DELETE_ARRAY(mChannels);
		mChannels = 0;
		if (mSourceVoice){
			mSourceVoice->DestroyVoice();
			mSourceVoice = 0;
		}
		if (mMasteringVoice){
			mMasteringVoice->DestroyVoice();
			mMasteringVoice = 0;
		}
		if (mXAudio2){
			mXAudio2->Release();
			mXAudio2 = 0;
		}
	}
	void feedData(){
		ASSERT(mEnabled);
		if (mEndRequestEvent.isSet()){
			mSourceVoice->Stop();
			mEndEvent.set();
			return;
		}
		//チャネルごとに発音
		for (int i = 0; i < mChannelCount; ++i){
			mChannels[i].startCalculation();
		}
		short* data = mBuffers[mBufferIndex];
		fill(data, BUFFER_SIZE);

		XAUDIO2_BUFFER buffer;
		memset(&buffer, 0, sizeof(XAUDIO2_BUFFER));
		buffer.AudioBytes = BUFFER_SIZE * sizeof(short);
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
		buffer.LoopLength = 0;
		buffer.LoopCount = 0;
		buffer.pContext = this;
		buffer.Flags = 0;
		buffer.pAudioData = reinterpret_cast<BYTE*>(mBuffers[mBufferIndex]);
		mSourceVoice->SubmitSourceBuffer(&buffer);

		++mBufferIndex;
		if (mBufferIndex == BUFFER_COUNT){
			mBufferIndex = 0;
		}
	}
	IXAudio2* mXAudio2;
	IXAudio2MasteringVoice* mMasteringVoice;
	IXAudio2SourceVoice* mSourceVoice;
	Callback mCallback;
	short* mBuffers[BUFFER_COUNT];
	int mBufferIndex;
	Event mEndEvent;
#elif defined(SUNABA_USE_DSOUND)
	enum{
		DST_BUFFER_SIZE = FREQUENCY / 15, //66ms。これくらいが安全か？
	};
	class FeedingThread : public Thread{
	public:
		FeedingThread() : Thread(true), mImpl(0){ //最大優先度
		}
		void start(Sound::Impl* soundImpl){
			mImpl = soundImpl;
			Thread::start();
		}
		virtual void operator()(){
			mImpl->feedThreadFunc();
		}
	private:
		Sound::Impl* mImpl;
	};
	Impl(int channelCount, void* windowHandle) : 
	mDirectSound(0),
	mPrimaryBuffer(0),
	mSecondaryBuffer(0),
	mPrevFeedTime(0),
	mSrcWritePos(0),
	mSrcReadPos(0),
	mDstWritePos(0),
	mSrcWriteCount(0),
	mDstWriteCount(0),
	mDropCount(0),
	mDelayedSampleCount(0),
	mChannels(0),
	mChannelCount(0),
	mEnabled(false){
		HRESULT hr;
		hr = DirectSoundCreate8(NULL, &mDirectSound, NULL);
		if (FAILED(hr)){
			WRITE_LOG("DirectSoundCreate8 failed");
			if (hr == DSERR_ALLOCATED){
				WRITE_LOG("¥tDSERR_ALLOCATED");
			}else if (hr == DSERR_INVALIDPARAM){
				WRITE_LOG("¥tDSERR_INVALIDPARAM");
			}else if (hr == DSERR_NOAGGREGATION){
				WRITE_LOG("¥tDSERR_NOAGGREGATION");
			}else if (hr == DSERR_NODRIVER){
				WRITE_LOG("¥tDSERR_NODRIVER");
			}else if (hr == DSERR_OUTOFMEMORY){
				WRITE_LOG("¥tDSERR_OUTOFMEMORY");
			}else{
				WRITE_LOG("¥tunknown error.");
			}
			return;
		}
		hr = mDirectSound->SetCooperativeLevel(static_cast<HWND>(windowHandle), DSSCL_PRIORITY);
		if (FAILED(hr)){
			WRITE_LOG("IDirectSound8::SetCooperativeLevel failed");
			if (hr == DSERR_ALLOCATED){
				WRITE_LOG("¥tDSERR_ALLOCATED");
			}else if (hr == DSERR_INVALIDPARAM){
				WRITE_LOG("¥tDSERR_INVALIDPARAM");
			}else if (hr == DSERR_UNINITIALIZED){
				WRITE_LOG("¥tDSERR_UNINITIALIZED");
			}else if (hr == DSERR_UNSUPPORTED){
				WRITE_LOG("¥tDSERR_UNSUPPORTED");
			}else{
				WRITE_LOG("¥tunknown error.");
			}
			return;
		}
		DSBUFFERDESC desc;
		ZeroMemory(&desc, sizeof(DSBUFFERDESC));
		desc.dwSize = sizeof(DSBUFFERDESC);
		desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

		//プライマリバッファフォーマット変更
		hr = mDirectSound->CreateSoundBuffer(&desc, &mPrimaryBuffer, NULL);
		if (FAILED(hr)){
			WRITE_LOG("IDirectSound8::CreateSoundBuffer failed");
			if (hr == DSERR_ALLOCATED){
				WRITE_LOG("¥tDSERR_ALLOCATED");
			}else if (hr == DSERR_BADFORMAT){
				WRITE_LOG("¥tDSERR_BADFORMAT");
			}else if (hr == DSERR_BUFFERTOOSMALL){
				WRITE_LOG("¥tDSERR_BUFFERTOOSMALL");
			}else if (hr == DSERR_CONTROLUNAVAIL){
				WRITE_LOG("¥tDSERR_CONTROLUNAVAIL");
			}else if (hr == DSERR_DS8_REQUIRED){
				WRITE_LOG("¥tDSERR_DS8_REQUIED");
			}else if (hr == DSERR_INVALIDCALL){
				WRITE_LOG("¥tDSERR_INVALIDCALL");
			}else if (hr == DSERR_INVALIDPARAM){
				WRITE_LOG("¥tDSERR_INVALIDPARAM");
			}else if (hr == DSERR_NOAGGREGATION){
				WRITE_LOG("¥tDSERR_NOAGGREGATION");
			}else if (hr == DSERR_OUTOFMEMORY){
				WRITE_LOG("¥tDSERR_OUTOFMEMORY");
			}else if (hr == DSERR_UNINITIALIZED){
				WRITE_LOG("¥tDSERR_UNINITIALIZED");
			}else if (hr == DSERR_UNSUPPORTED){
				WRITE_LOG("¥tDSERR_UNSUPPORTED");
			}else{
				WRITE_LOG("¥tunknown error.");
			}
			return;
		}
		WAVEFORMATEX format;
		ZeroMemory(&format, sizeof(WAVEFORMATEX));
		format.wBitsPerSample = 16;
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 1;
		format.nSamplesPerSec = FREQUENCY;
		format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		hr = mPrimaryBuffer->SetFormat(&format);
		if (FAILED(hr)){
			WRITE_LOG("IDirectSoundBuffer8::SetFormat failed");
			return;
		}
		desc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		desc.dwBufferBytes = DST_BUFFER_SIZE * sizeof(short);
		desc.lpwfxFormat = &format;

		IDirectSoundBuffer* buffer = 0;
		hr = mDirectSound->CreateSoundBuffer(&desc, &buffer, NULL);
		if (FAILED(hr)){
			WRITE_LOG("IDirectSound8::CreateSoundBuffer failed");
			return;
		}
		hr = buffer->QueryInterface(IID_IDirectSoundBuffer8, reinterpret_cast<void**>(&mSecondaryBuffer));
		if (FAILED(hr)){
			WRITE_LOG("IDirectSoundBuffer::QueryInterface failed");
			return;
		}
		buffer->Release();
		buffer = 0;
		//初回充填
		void* p = 0;
		DWORD size = 0;
		hr = mSecondaryBuffer->Lock(0, DST_BUFFER_SIZE, &p, &size, NULL, NULL, 0);
		if (FAILED(hr)){
			WRITE_LOG("IDirectSoundBuffer8::Lock failed");
			return;
		}
		if (static_cast<int>(size) != DST_BUFFER_SIZE){
			WRITE_LOG("IDirectSoundBuffer8::Lock return wrong size.");
			return;
		}
		memset(p, 0, size);
		hr = mSecondaryBuffer->Unlock(p, size, NULL, 0);
		if (FAILED(hr)){
			WRITE_LOG("IDirectSoundBuffer8::Unlock failed");
			return;
		}
		mChannelCount = channelCount;
		mChannels = new SoundChannel[mChannelCount];
		//ソースバッファ用意
		int srcBufferSampleCount = BUFFER_SIZE;
		int srcBufferSize = srcBufferSampleCount * sizeof(short);
		mSrcBuffer = new short[srcBufferSampleCount];
		memset(mSrcBuffer, 0, srcBufferSize);
		mPrevFeedTime = getTimeInMilliSecond();
		//スレッド用意
		mFeedingThread.start(this);
		//再生開始
		hr = mSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING); //ループ再生
		if (FAILED(hr)){
			WRITE_LOG("IDirectSoundBuffer8::Play failed");
			return;
		}
		mEnabled = true;
	}
	~Impl(){
		if (!mEnabled){ //初期化そもそもしてないから抜ける。
			return;
		}
		mEndRequestEvent.set();
		mFeedingThread.wait();
		DELETE_ARRAY(mChannels);
		mChannels = 0;
		if (mSecondaryBuffer){
			mSecondaryBuffer->Release();
			mSecondaryBuffer = 0;
		}
		if (mPrimaryBuffer){
			mPrimaryBuffer->Release();
			mPrimaryBuffer = 0;
		}
		if (mDirectSound){
			mDirectSound->Release();
			mDirectSound = 0;
		}
		DELETE_ARRAY(mSrcBuffer);
	}
	void feed(){
		//srcBufferへの充填
		unsigned time = getTimeInMilliSecond();
		if (time == mPrevFeedTime){ //時間が経ってない。やることがない。
			return;
		}
		int timeDiff = static_cast<int>(time - mPrevFeedTime);
		mPrevFeedTime = time;

		int t = (FREQUENCY * timeDiff) + mDelayedSampleCount;
		int totalSrcWriteSize = t / 1000;
		mDelayedSampleCount = t -= (totalSrcWriteSize * 1000);
		int srcBufferSize = BUFFER_SIZE;
		if (totalSrcWriteSize > srcBufferSize){ //処理落ち。仕方ないですね。TODO:記録
			totalSrcWriteSize = srcBufferSize;
			++mDropCount;
		}
		short* src[2];
		int srcWriteSizes[2];
		src[0] = mSrcBuffer + mSrcWritePos;
		src[1] = mSrcBuffer;
		if ((mSrcWritePos + totalSrcWriteSize) < srcBufferSize){
			srcWriteSizes[0] = totalSrcWriteSize;
			srcWriteSizes[1] = 0;
			ASSERT(totalSrcWriteSize >= 0);
			mSrcWritePos += totalSrcWriteSize;
		}else{
			srcWriteSizes[0] = srcBufferSize - mSrcWritePos;
			srcWriteSizes[1] = totalSrcWriteSize - srcWriteSizes[0];
			ASSERT(srcWriteSizes[1] >= 0);
			mSrcWritePos = srcWriteSizes[1];
		}
		ASSERT(srcWriteSizes[0] >= 0 && srcWriteSizes[1] >= 0);
		ASSERT(mSrcWritePos >= 0 && mSrcWritePos < srcBufferSize);
		for (int i = 0; i < mChannelCount; ++i){
			mChannels[i].startCalculation();
		}
		for (int b = 0; b < 2; ++b){
			if (srcWriteSizes[b] > 0){
				ASSERT(src[b] + srcWriteSizes[b] <= mSrcBuffer + srcBufferSize);
				ASSERT(src[b] + srcWriteSizes[b] >= mSrcBuffer);
				fill(src[b], srcWriteSizes[b]);
			}
		}
		++mSrcWriteCount;
		//書ける最大サイズ
		//場所を尋ねる。
		DWORD playU, writeU;
		HRESULT hr = mSecondaryBuffer->GetCurrentPosition(&playU, &writeU);
		if (FAILED(hr)){
			return;
		}
		int play = static_cast<int>(playU / sizeof(short));
		if (play == mDstWritePos){ //まだ書けない。
			return;
		}
		//次はバッファへの充填
		//読める最大サイズ
		int totalSrcReadableSize = mSrcWritePos - mSrcReadPos;
		if (totalSrcReadableSize < 0){
			totalSrcReadableSize += srcBufferSize;
		}
		//書ける最大サイズ
		const int dstBufferSize = DST_BUFFER_SIZE;
		int totalDstWritableSize = play - mDstWritePos;
		if (totalDstWritableSize < 0){
			totalDstWritableSize += dstBufferSize;
		}
		//この最小値を取る
		int copySize = (totalSrcReadableSize < totalDstWritableSize) ? totalSrcReadableSize : totalDstWritableSize;
		if (copySize == 0){
			return;
		}
		int write = static_cast<int>(writeU / sizeof(short));
		if (play > write){
			write += dstBufferSize;
		}
		if ((mDstWritePos < write) && ((mDstWritePos + copySize) > play)){ //間に合ってない！！
			++mDropCount;
			return;
		}
		void* p[2];
		DWORD sizeU[2];
		hr = mSecondaryBuffer->Lock(
			mDstWritePos * sizeof(short), 
			copySize * sizeof(short), &p[0], &sizeU[0], &p[1], &sizeU[1], 0);
		if (FAILED(hr)){
			return;
		}
		short* dst[2];
		int dstWriteSizes[2];
		int dstWriteOffsets[2];
		dstWriteOffsets[0] = mDstWritePos;
		dstWriteOffsets[1] = 0;
		if ((mDstWritePos + copySize) < dstBufferSize){
			dstWriteSizes[0] = copySize;
			dstWriteSizes[1] = 0;
			mDstWritePos += copySize;
		}else{
			dstWriteSizes[0] = dstBufferSize - mDstWritePos;
			dstWriteSizes[1] = copySize - dstWriteSizes[0];
			mDstWritePos = dstWriteSizes[1];
		}
		ASSERT(dstWriteSizes[0] >= 0 && dstWriteSizes[1] >= 0);
		ASSERT(mDstWritePos >= 0 && mDstWritePos < dstBufferSize);
		ASSERT(static_cast<int>(sizeU[0] / sizeof(short)) == dstWriteSizes[0]);
		ASSERT(static_cast<int>(sizeU[1] / sizeof(short)) == dstWriteSizes[1]);
		dst[0] = static_cast<short*>(p[0]);
		dst[1] = static_cast<short*>(p[1]);

		int srcReadSizes[2];
		int srcReadOffsets[2];
		srcReadOffsets[0] = mSrcReadPos;
		srcReadOffsets[1] = 0;
		src[0] = mSrcBuffer + mSrcReadPos;
		src[1] = mSrcBuffer;
		if ((mSrcReadPos + copySize) < srcBufferSize){
			srcReadSizes[0] = copySize;
			srcReadSizes[1] = 0;
			mSrcReadPos += copySize;
		}else{
			srcReadSizes[0] = srcBufferSize - mSrcReadPos;
			srcReadSizes[1] = copySize - srcReadSizes[0];
			mSrcReadPos = srcReadSizes[1];
		}
		ASSERT(srcReadSizes[0] >= 0 && srcReadSizes[1] >= 0);
		int srcIndex = 0;
		int dstIndex = 0;
		while (copySize > 0){
			ASSERT(srcIndex < 2 && dstIndex < 2);
			int tmpCopySize = (srcReadSizes[srcIndex] < dstWriteSizes[dstIndex]) ? srcReadSizes[srcIndex] : dstWriteSizes[dstIndex];
			memcpy(dst[dstIndex], src[srcIndex], tmpCopySize * sizeof(short));
			dst[dstIndex] += tmpCopySize;
			src[srcIndex] += tmpCopySize;
			srcReadSizes[srcIndex] -= tmpCopySize;
			dstWriteSizes[dstIndex] -= tmpCopySize;
			copySize -= tmpCopySize;
			ASSERT(srcReadSizes[srcIndex] >= 0 && dstWriteSizes[dstIndex] >= 0);
			if (srcReadSizes[srcIndex] == 0){
				++srcIndex;
			}
			if (dstWriteSizes[dstIndex] == 0){
				++dstIndex;
			}
		}
		mSecondaryBuffer->Unlock(p[0], sizeU[0], p[1], sizeU[1]);
		++mDstWriteCount;
	}
	void feedThreadFunc(){
		bool foreverTrue = true;
		int stopCount = 0;
		while (foreverTrue){
			if (mEndRequestEvent.isSet()){
				for (int i = 0; i < mChannelCount; ++i){
					mChannels[i].setDumping(0.01f);//減衰強化
				}
				++stopCount;
				if (stopCount == 20){ //減衰してから止める。数は適当。
					mSecondaryBuffer->Stop();
					break;
				}
			}
			feed();
			sleepMilliSecond(5); //とりあえずこれくらい
		}
	}
	IDirectSound8* mDirectSound;
	IDirectSoundBuffer* mPrimaryBuffer;
	IDirectSoundBuffer8* mSecondaryBuffer;
	FeedingThread mFeedingThread;
	short* mSrcBuffer;
	unsigned mPrevFeedTime;
	int mSrcWritePos;
	int mSrcReadPos;
	int mDstWritePos;
	int mSrcWriteCount;
	int mDstWriteCount;
	int mDropCount;
	int mDelayedSampleCount;
#else
	Impl(int, void*) : mChannels(0), mChannelCount(0), mEnabled(false){}
	~Impl(){}
#endif
	SoundChannel* mChannels;
	int mChannelCount;
	bool mEnabled;
	Event mEndRequestEvent;
};

Sound::Sound(int channelCount, void* windowHandle) : mImpl(0){
	mImpl = new Impl(channelCount, windowHandle);
}

Sound::~Sound(){
	DELETE(mImpl);
	mImpl = 0;
}

void Sound::setFrequency(int channel, float frequency){
	mImpl->setFrequency(channel, frequency);
}

void Sound::setDumping(int channel, float dumping){
	mImpl->setDumping(channel, dumping);
}

void Sound::play(int channel, float strength){
	mImpl->play(channel, strength);
}

} //namespace Sunaba


#endif
