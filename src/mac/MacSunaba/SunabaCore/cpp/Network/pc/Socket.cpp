#ifdef SUNABA_USE_PSEUDO_SOCKET
#ifdef __APPLE__
#include <stdio.h>
#else
#include <windows.h>
#undef DELETE
#endif
#else
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#undef DELETE
#endif

#include "Base/Os.h"
#include "Base/Base.h"
#include "Network/Socket.h"
#include <sstream>

namespace Sunaba{

#ifdef SUNABA_USE_PSEUDO_SOCKET
class Socket::Impl{
public:
	class Buffer{
	public:
		Buffer() : mReadPos(0), mWritePos(0){
		}
		void initializeLock(){
			InitializeCriticalSection(&mLock);
		}
		int write(const unsigned char* data, int size){
			EnterCriticalSection(&mLock);
//std::ostringstream oss;
//oss << "write: " << mReadPos << '\t'<< mWritePos << '\t' << size;
			int transferedSize = 0;
			int endPos = mReadPos - 1;
			if (endPos < 0){
				endPos += BUFFER_SIZE;
			}
			while ((mWritePos != endPos) && (transferedSize < size)){
				mBuffer[mWritePos] = data[transferedSize];
				++mWritePos;
				if (mWritePos == BUFFER_SIZE){
					mWritePos = 0;
				}
				++transferedSize;
			}
			LeaveCriticalSection(&mLock);
//oss << '\t' << transferedSize << std::endl;
//writeToConsole(oss.str().c_str());
			return transferedSize;
		}
		int read(unsigned char* buffer, int bufferSize){
			EnterCriticalSection(&mLock);
//std::ostringstream oss;
//oss << "read: " << mReadPos << '\t' << mWritePos << '\t' << bufferSize;
			int transferedSize = 0;
			while ((mReadPos != mWritePos) && (transferedSize < bufferSize)){
				buffer[transferedSize] = mBuffer[mReadPos];
				++mReadPos;
				if (mReadPos == BUFFER_SIZE){
					mReadPos = 0;
				}
				++transferedSize;
			}
			LeaveCriticalSection(&mLock);
//oss << '\t' << transferedSize << std::endl;
//if (transferedSize > 0){
//writeToConsole(oss.str().c_str());
//}
			return transferedSize;
		}
	private:
		enum{
			BUFFER_SIZE = 4096,
		};
		int mReadPos;
		int mWritePos;
		CRITICAL_SECTION mLock;
		unsigned char mBuffer[BUFFER_SIZE];
	};
	static void beginSystem(){ //空
		if (mBeginSystemCount == 0){
			mBufferFromServer.initializeLock();
			mBufferToServer.initializeLock();
		}
		++mBeginSystemCount;
	}
	static void endSystem(){ //空
		STRONG_ASSERT(mBeginSystemCount > 0);
		--mBeginSystemCount;
	}
	//acceptから呼ばれるデフォルトコンストラクタ。SOCKETを受け取らないのは、intだった時に下のと区別がつかないから
	Impl() : mIsServer(false), mAccepted(true){
	}
	//接続受け入れソケット
	Impl(int port) : mIsServer(true){
		mServerPort = port;
	}
	//クライアントソケット
	Impl(const char*, int port) : mIsServer(false), mAccepted(false){
		STRONG_ASSERT(mServerPort == port); //ポートが一致しないならつながりようがない。あるいはサーバがない。
	}
	Impl* accept(){
		return new Impl;
	}
	~Impl(){
	}
	bool isServer() const{
		return mIsServer;
	}
	int write(const unsigned char* data, int size){
		if (mAccepted){
			return mBufferFromServer.write(data, size);
		}else{
			return mBufferToServer.write(data, size);
		}
	}
	int read(unsigned char* buffer, int bufferSize){
		if (mAccepted){
			return mBufferToServer.read(buffer, bufferSize);
		}else{
			return mBufferFromServer.read(buffer, bufferSize);
		}
	}
	bool isError() const{
		return false;
	}
private:
	bool mIsServer;
	bool mAccepted;
	static int mServerPort;
	static Buffer mBufferFromServer;
	static Buffer mBufferToServer;
	static int mBeginSystemCount;
};
int Socket::Impl::mServerPort = -1;
Socket::Impl::Buffer Socket::Impl::mBufferFromServer;
Socket::Impl::Buffer Socket::Impl::mBufferToServer;
int Socket::Impl::mBeginSystemCount = 0;
#else
class Socket::Impl{
public:
	static void beginSystem(){
		if (mBeginSystemCount == 0){
			WSADATA wsaData;
			int ret = WSAStartup(MAKEWORD(1, 1), &wsaData);
			STRONG_ASSERT(ret == 0); //TODO:エラー処理
		}
		++mBeginSystemCount;
	}
	static void endSystem(){
		STRONG_ASSERT(mBeginSystemCount > 0);
		--mBeginSystemCount;
		if (mBeginSystemCount == 0){
			int ret = WSACleanup();
			STRONG_ASSERT(ret == 0); //TODO:エラー処理
		}
	}
	//acceptから呼ばれるデフォルトコンストラクタ。SOCKETを受け取らないのは、intだった時に下のと区別がつかないから
	Impl() : mSocket(INVALID_SOCKET), mIsServer(false){
	}
	//接続受け入れソケット
	Impl(int port) : mSocket(INVALID_SOCKET), mIsServer(true){
		mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
		STRONG_ASSERT(mSocket != INVALID_SOCKET); //TODO:エラー処理
		sockaddr_in addr;
		memset(&addr, 0, sizeof(sockaddr_in));
		addr.sin_family = AF_INET;
		STRONG_ASSERT((port >= 0) && (port <= 0xffff));
		addr.sin_port = htons(static_cast<unsigned short>(port));
//		addr.sin_addr.S_un.S_addr = INADDR_ANY; //TODO:windowsファイアウォールが文句言う
		addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //TODO:緊急回避
		int ret = bind(mSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)); 
		STRONG_ASSERT(ret == 0); //TODO:エラー処理
		ret = listen(mSocket, SOMAXCONN);
		STRONG_ASSERT(ret == 0); //TODO:エラー処理
	}
	//クライアントソケット
	Impl(const char* hostname, int port) : mSocket(INVALID_SOCKET), mIsServer(false){
		mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
		STRONG_ASSERT(mSocket != INVALID_SOCKET); //TODO:エラー処理
		sockaddr_in addr;
		memset(&addr, 0, sizeof(sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = inet_addr(hostname); //TODO:DNS解決。今は文字列で127.0.0.1とか書いてあることを前提としている
		STRONG_ASSERT((port >= 0) && (port <= 0xffff));
		addr.sin_port = htons(static_cast<unsigned short>(port));
		int ret = connect(mSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)); //TODO:数秒止まるのをなんとかすべき
		if (ret != 0){
			close();
		}
	}
	Impl* accept(){
		STRONG_ASSERT(mSocket != INVALID_SOCKET);
		//受け入れ可能か調べる
		fd_set fdSet;
		FD_ZERO(&fdSet);
		FD_SET(mSocket, &fdSet);
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		int ret = select(1, &fdSet, 0, 0, &timeout);
		Impl* r = 0;
		if (ret == 1){
			SOCKET s = ::accept(mSocket, 0, 0);
			STRONG_ASSERT(s != INVALID_SOCKET);
			r = new Impl;
			r->mSocket = s;
		}else{
			STRONG_ASSERT(ret != SOCKET_ERROR); //TODO:エラー処理
		}
		return r;
	}
	~Impl(){
		close();
	}
	bool isServer() const{
		return mIsServer;
	}
	int write(const unsigned char* data, int size){
		if (isError()){
			return -1;
		}
		STRONG_ASSERT(!mIsServer);
		//書き込み可能か調べる
		fd_set fdSet;
		FD_ZERO(&fdSet);
		FD_SET(mSocket, &fdSet);
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		int ret = select(1, 0, &fdSet, 0, &timeout);
		int transferedSize = 0;
		if (ret == 1){
			transferedSize = send(mSocket, reinterpret_cast<const char*>(data), size, 0);
			if (transferedSize == SOCKET_ERROR){ //何か起こった。閉じちゃう。
				transferedSize = -1;
				close();
			}
		}else{
			STRONG_ASSERT(ret != SOCKET_ERROR); //TODO:エラー処理
		}
		return transferedSize;
	}
	int read(unsigned char* buffer, int bufferSize){
		if (isError()){
			return -1;
		}
		STRONG_ASSERT(!mIsServer);
		//読み込み込み可能か調べる
		fd_set fdSet;
		FD_ZERO(&fdSet);
		FD_SET(mSocket, &fdSet);
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		int ret = select(1, &fdSet, 0, 0, &timeout);
		int transferedSize = 0;
		if (ret == 1){
			transferedSize = recv(mSocket, reinterpret_cast<char*>(buffer), bufferSize, 0);
			if ((transferedSize == 0) || (transferedSize == SOCKET_ERROR)){ //切れたか、何か起こった。閉じちゃう。
				transferedSize = -1;
				close();
			}
		}else{
			STRONG_ASSERT(ret != SOCKET_ERROR); //TODO:エラー処理
		}
		return transferedSize;
	}
	bool isError() const{
		return (mSocket == INVALID_SOCKET);
	}
private:
	void close(){
		if (mSocket != INVALID_SOCKET){
			int ret = closesocket(mSocket);
			STRONG_ASSERT(ret == 0); //TODO:エラー処理
		}
		mSocket = INVALID_SOCKET;
	}
	SOCKET mSocket;
	bool mIsServer;
	static int mBeginSystemCount;
};
int Socket::Impl::mBeginSystemCount = 0;
#endif

void Socket::beginSystem(){
	Impl::beginSystem();
}

void Socket::endSystem(){
	Impl::endSystem();
}

Socket::Socket() : mImpl(0){
}

Socket::Socket(int port) : mImpl(0){
	mImpl = new Impl(port);
}

Socket::Socket(const char* hostname, int port) : mImpl(0){
	mImpl = new Impl(hostname, port);
}

Socket::Socket(Impl* impl) : mImpl(impl){
}

Socket::~Socket(){
	DELETE(mImpl);
}

void Socket::accept(Socket* out){
	out->mImpl = mImpl->accept();
}

bool Socket::isServer() const{
	return mImpl->isServer();
}

bool Socket::isEmpty() const{
	return (mImpl == 0) || (mImpl->isError());
}

int Socket::write(const unsigned char* data, int size){
	return mImpl->write(data, size);
}

int Socket::read(unsigned char* buffer, int bufferSize){
	return mImpl->read(buffer, bufferSize);
}

void Socket::moveTo(Socket* to){
	STRONG_ASSERT(to->isEmpty());
	to->mImpl = mImpl;
	mImpl = 0;
}

} //namespace Sunaba
