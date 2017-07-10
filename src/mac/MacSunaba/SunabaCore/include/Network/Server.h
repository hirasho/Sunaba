#ifndef INCLUDED_SUNABA_NETWORK_SERVER_H
#define INCLUDED_SUNABA_NETWORK_SERVER_H

#include "Network/Socket.h"

namespace Sunaba{
class Connection;

class Server{
public:
private:
	friend class NetworkManager;

	Connection* accept();
	Server(int port);
	Server(const Server&); //コピーコンストラクタ封印
	~Server();
	void operator=(const Server&); //代入封印

	Socket mSocket;
};

} //namespace Sunaba

#include "Network/inc/Server.inc.h"

#endif

