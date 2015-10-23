
//#include <assert.h>
//#include <errno.h>
#include <string> 
//#include <netdb.h> 
//#include <netinet/in.h> 
//#include <unistd.h>
//#include <sys/types.h> 
//#include <sys/socket.h> 
#include <ossimPlanet/ossimPlanetSocketNetworkConnection.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>

ossimPlanetSocketNetworkConnection::ossimPlanetSocketNetworkConnection(const std::string& hostname, int port, char delimiter) :
    ossimPlanetNetworkConnection(hostname + ":" + mkUtils::asString(port)),
    actionDelimiter_(delimiter)
{
   theSocket = new ossimPlanetSocketNetworkConnection::ossimPlanetNetSocketRef;
   if(theSocket->open(true)) // make a tcp stream
   {
      theSocket->setBlocking(false);
      if(theSocket->connect(hostname.c_str(), port) > 0)
      {
         error_ = "unable to connect socket";
         
      }
   }
   else
   {
      theSocket = 0;
      error_ = "unable to create socket";
   }

#if 0
    // create and connect the socket
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ >= 0) {
	struct hostent* host = gethostbyname(hostname.c_str());
	if (host != NULL) {
	    // Initialize the socket address structure
	    struct sockaddr_in connectAddress;
	    memset(&connectAddress, 0, sizeof(connectAddress));
	    memcpy((char *)&connectAddress.sin_addr, host->h_addr, host->h_length);
	    connectAddress.sin_family = host->h_addrtype; // Should be AF_INET
	    connectAddress.sin_port = htons((unsigned short)port);
	    
	    // try to connect, repeat if EINTRrupted, abort on any other failure
	    while (connect(socket_, (struct sockaddr*)&connectAddress, sizeof(connectAddress)) < 0) {
		if (errno != EINTR) {
		    error_ = "unable to connect socket";
		    break;
		}
	    }
	} else {
	    error_ = "cannot gethostbyname(\"" + hostname + "\") : " + hstrerror(h_errno);
	}
    } else {
	error_ = "unable to create socket";
    }
    
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 0;
#endif
}

ossimPlanetSocketNetworkConnection::~ossimPlanetSocketNetworkConnection() 
{
   if(theSocket.valid())
   {
      theSocket->close();
   }
   theSocket = 0;
#if 0
    if (socket_ >= 0) {
	close(socket_);
	socket_ = -1;
    }
#endif
}

void ossimPlanetSocketNetworkConnection::send(const ossimPlanetAction& a, const std::string& /*destination*/) 
{
   ossimString code;
   a.sourceCode(code);
   if (outBuffer_.empty())
   {
      int bytesWritten = theSocket->send(code.c_str(), code.length());
      if (bytesWritten != (int)code.length())
      {
         outBuffer_.append(code.substr(bytesWritten == -1 ? 0 : bytesWritten));
         outBuffer_.append(1, actionDelimiter_);
      }
      else
      {
         if (theSocket->send(&actionDelimiter_, 1) != 1)
         {
            outBuffer_.append(1, actionDelimiter_);
         }
      }
   }
   else
   {
      outBuffer_.append(code.string());
      outBuffer_.append(1, actionDelimiter_);
      attemptToFlushOutBuffer();
   }
}

void ossimPlanetSocketNetworkConnection::receive()
{
#if 1
    if(theSocket.valid())
    {
       attemptToFlushOutBuffer();
       std::vector<char> readBuffer(4096);
       int bytesRead = theSocket->recv(&readBuffer.front(), readBuffer.size());
       if (bytesRead > 0)
       {
          inBuffer_.append(readBuffer.begin(), readBuffer.begin() + bytesRead);
       }
       else if (bytesRead == -1)
       {
          //std::cerr << "ossimPlanetSocketNetworkConnection::receive() failed to read data. NetworkConnection::name() = " << name_ << std::endl;
       }

       for (ossim_int64 eoc = inBuffer_.find(actionDelimiter_); eoc != std::string::npos; eoc = inBuffer_.find(actionDelimiter_))
       {
          ossimPlanetDestinationCommandAction(inBuffer_.substr(0, eoc), 
                                              "ossimPlanetSocketNetworkConnection-" + name_).execute();
          inBuffer_.erase(0, eoc + 1);
       }
    }
#endif
}

// protected

inline void ossimPlanetSocketNetworkConnection::attemptToFlushOutBuffer()
{
   if (!outBuffer_.empty())
   {
       int bytesWritten = theSocket->send(outBuffer_.c_str(), outBuffer_.length());
      
      if (bytesWritten != -1)
      {
         outBuffer_.erase(0, bytesWritten);
      }

      if (outBuffer_.length() > 64*1024)
      {
//          std::cerr << "Warning: ossimPlanetSocketNetworkConnection " << name_ << " has " << outBuffer_.length() << " bytes of backed up outgoing Actions!" << std::endl;
      }
   }
#if 0
   if(!theSocket.valid()) return;
   if (!outBuffer_.empty())
   {
      int bytesWritten = theSocket->send(outBuffer_.c_str(), outBuffer_.length());
//       int bytesWritten = theSocket->write(outBuffer_.c_str(), outBuffer_.length());
      
      if (bytesWritten != -1)
      {
         outBuffer_.erase(0, bytesWritten);
      }

      if (outBuffer_.length() > 64*1024)
      {
         std::cerr << "Warning: ossimPlanetSocketNetworkConnection " << name_ << " has " << outBuffer_.length() << " bytes of backed up outgoing Actions!" << std::endl;
      }
   }
#endif
#if 0
    assert(socket_ >= 0);

    if (!outBuffer_.empty()) {
        int bytesWritten = write(socket_, outBuffer_.c_str(), outBuffer_.length());
        if (bytesWritten != -1)
            outBuffer_.erase(0, bytesWritten);
            
        if (outBuffer_.length() > 64*1024)
            std::cerr << "Warning: ossimPlanetSocketNetworkConnection " << name_ << " has " << outBuffer_.length() << " bytes of backed up outgoing Actions!" << std::endl;
    }
#endif
}


