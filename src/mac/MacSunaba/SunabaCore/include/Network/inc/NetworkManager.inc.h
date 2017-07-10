#include "Network/Server.h"
#include "Network/Connection.h"
#include "Network/Socket.h"

namespace Sunaba{

inline NetworkManager::NetworkManager(){
	Socket::beginSystem();
}

inline NetworkManager::~NetworkManager(){
	STRONG_ASSERT(mConnections.empty());
	Socket::endSystem();
}

inline Server* NetworkManager::createServer(int port){
	return new Server(port);
}

inline Connection* NetworkManager::createClient(const char* hostname, int port){
	Connection* c = new Connection(hostname, port);
	mConnections.push_front(c);
	c->mIterator = mConnections.begin();
	return c;
}

inline Connection* NetworkManager::accept(Server* server){
	Connection* c = server->accept();
	if (c){
		mConnections.push_front(c);
		c->mIterator = mConnections.begin();
	}
	return c;
}

inline void NetworkManager::destroyServer(Server** s){
	if (s && *s){
		DELETE(*s);
	}
}

inline void NetworkManager::destroyConnection(Connection** c){
	if (c && *c){
		std::list<Connection*>::iterator it = (*c)->mIterator;
		mConnections.erase(it);
		DELETE(*c);
	}
}

inline void NetworkManager::update(){
	std::list<Connection*>::iterator it = mConnections.begin();
	std::list<Connection*>::iterator end = mConnections.end();
	while (it != end){
		(*it)->update();
		++it;
	}
}

} //namespace Sunaba