class ossimPlanetSocketNetworkConnection;
#ifndef ossimPlanetSocketNetworkConnection_H
#define ossimPlanetSocketNetworkConnection_H

#include <ossimPlanet/sg_socket.h>
#include <ossimPlanet/netBuffer.h>
#include <osg/ref_ptr>
// point to point socket connection.
// XXX see bugs below

#include <string>
/* #include <sys/time.h>  */
#include "ossimPlanet/ossimPlanetNetworkConnection.h"

class ossimPlanetSocketNetworkConnection : public ossimPlanetNetworkConnection
{
public:
   class ossimPlanetNetSocketRef : public netSocket,
                                   public osg::Referenced
   {
   public:
      ossimPlanetNetSocketRef(){}
      virtual ~ossimPlanetNetSocketRef(){}
   };
    ossimPlanetSocketNetworkConnection(const std::string& hostname, int port, char delimiter = '\0');
    ~ossimPlanetSocketNetworkConnection();

    void send(const ossimPlanetAction& a, const std::string& destination);
        // XXX doesn't really handle destination correctly
        //     we don't encode the destination (or the origin) of the sent Action.
        //     so the remote receiver has to infer origin from which socket he got it off of.
        //     also the remote receiver can't know the intended destination, since we don't
        //     encode it and broadcast it to all NetworkConnections.  
	
    void receive();
        // XXX doesn't yet handle broadcast semantics or destination correctly.  
        // XXX our protocol here doesn't support origin names, so we just use 
        //     the NetworkConnection name as the originating federate name.

protected:
    void attemptToFlushOutBuffer();
        // if outBuffer_ is not empty, write() as much of it as possible and remove the written part from outBuffer_
        // assert(socket_ >= 0)

//    osg::ref_ptr<SGSocket> theSocket;
    osg::ref_ptr<ossimPlanetNetSocketRef> theSocket;
    char actionDelimiter_;
        // character that marks end of action. usually \n or \0

    std::string inBuffer_;
	// buffer for accumulating incoming Actions
        
    std::string outBuffer_;
        // buffer for accumulating outgoing Actions, in the case of write() failing somehow.
    

#if 0
    int socket_;
	// socket descriptor
    
    struct timeval timeout_;
	// select() timeout structure
        
#endif
};


#endif
