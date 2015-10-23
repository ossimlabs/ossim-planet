#include <iostream>
#include <fstream>
#include <sstream>
#include <limits.h>
#ifndef WIN32
#include <sys/param.h>
#else
#include <winsock.h>
#endif

#include <osgDB/FileUtils>

#include <ossimPlanet/ossimPlanetActionRouter.h>
#include <ossimPlanet/ossimPlanetSocketNetworkConnection.h>
#include <ossimPlanet/mkUtils.h>
#include <ossim/base/ossimNotifyContext.h>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>

using namespace mkUtils;


// network 

void ossimPlanetActionRouter::addNetworkConnection(ossimPlanetNetworkConnection* commLink)
{
   if(commLink&&commLink->error().empty())
   {
//     assert(commLink != NULL && commLink->error().empty());
    
      std::vector<ossimPlanetNetworkConnection*>::iterator i = find_if(network_.begin(), network_.end(), NCEqualPred(commLink->name()));
      if (i == network_.end())
         network_.push_back(commLink);
      else 
         std::cerr << "ossimPlanetActionRouter::addNetworkConnection() already has a ossimPlanetNetworkConnection named " << commLink->name() << std::endl;
   }
}

void ossimPlanetActionRouter::removeNetworkConnection(const ossimString& name)
{
    std::vector<ossimPlanetNetworkConnection*>::iterator i = find_if(network_.begin(), network_.end(), NCEqualPred(name));
    if (i != network_.end())
        network_.erase(i);
}

ossimPlanetNetworkConnection* ossimPlanetActionRouter::networkConnection(const ossimString& name) const
{
    std::vector<ossimPlanetNetworkConnection*>::const_iterator i = find_if(network_.begin(), network_.end(), NCEqualPred(name));
    return i == network_.end() ? NULL : *i;
}

// federate name

void ossimPlanetActionRouter::setFederateName(const ossimString& newName)
{
   
   if(ossimString(newName).trim().empty()) return;
   
//     assert(!newName.empty());
//     assert(newName.find(" ", 0) == -1);
    
    federateName_ = newName;
    
//     assert(newName == federateName());
}

void ossimPlanetActionRouter::post(const ossimPlanetAction& a)
{
   if(theThreadQueue.valid())
   {
      theThreadQueue->addAction(a);
   }
}

// route Actions

void ossimPlanetActionRouter::route(const ossimPlanetAction& a)
{
   ossimPlanetActionReceiver * receiver = 0; 
   ossimString target(a.target()); 
 
   // start critical section 
   theReceiverMutex.lock();
   
   MapType::iterator targetObj = receivers_.find(target); 
   if (targetObj != receivers_.end()) 
   { 
      receiver = targetObj->second; 
   }
   theReceiverMutex.unlock(); 
   // end critical section
   
   if (receiver) 
   { 
      // found a receiver so exectute the action 
      receiver->execute(a); 
   } 
   else 
   { 
      a.printError("bad target for action"); 
   } 
}

void ossimPlanetActionRouter::allRoute(const ossimPlanetAction& a)
{
    static const ossimString emptyString;  // static since don't ctor/dtor empty string every time
    remoteRouteImplementation(a, emptyString);
}

void ossimPlanetActionRouter::tellRoute(const ossimPlanetAction& a, const ossimString& destination)
{
   if(destination.empty()) return;
   
//     assert(!destination.empty()); 
    remoteRouteImplementation(a, destination);
}

void ossimPlanetActionRouter::executeFile(const ossimString& filename, const ossimString& origin)
{
#if 0 // XXX crash in osgDB::findFileInPath(), so junk the search path until we figure this out
    ossimString searchResult = osgDB::findFileInPath(filename, osgDB::getDataFilePathList());
    std::ifstream fin(searchResult.empty() ? filename.c_str() : searchResult.c_str());  
#else
    std::ifstream fin(filename.c_str());  
#endif
    
   if (!fin)
      ossimNotify(ossimNotifyLevel_WARN) << "cannot open file " << filename << " for execution in ossimPlanetActionRouter::executeFile()" << std::endl;
   else
   {
      ossimPlanetDestinationCommandAction x("", origin);
      for (fin >> x; fin; fin >> x)
      {
         route(x);
         x.setSourceCode("");
      }
      ossimString code;
      x.sourceCode(code);
      if(fin.eof()&&!code.empty())
      {
         route(x);
      }
   }
}

void ossimPlanetActionRouter::executeNetworkActions()
{
    for (int i = 0; i < (int)network_.size(); i++)
        network_[i]->receive();
}

// ossimPlanetActionReceiver management

void ossimPlanetActionRouter::registerReceiver(ossimPlanetActionReceiver* r)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReceiverMutex);
   if(!r) return;
//     assert(r != NULL);
    
    // If a receiver with this name was already registered, 
    // tell us before we replace it with r.
    MapType::iterator i = receivers_.find(r->pathname());
    if (i != receivers_.end())
	std::cerr << "\aWarning in ossimPlanetActionRouter::registerReceiver: ossimPlanetActionReceiver " << i->second << " with pathname " << r->pathname() << " already registered, replacing with ossimPlanetActionReceiver " << r << std::endl;
    
    receivers_[r->pathname()] = r;
    
//     assert(receiver(r->pathname()) == r);
}

void ossimPlanetActionRouter::unregisterReceiver(ossimPlanetActionReceiver* r)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReceiverMutex);
   if(!r) return;
//     assert(r != NULL);

    MapType::iterator i = receivers_.find(r->pathname());
	if(i!=receivers_.end())
	{
		if (i->second == r) 
		{
		// r is registered under r->pathname().
		// this is the normal case.
			receivers_.erase(i);

		} 
		else if (i->second == NULL) 
		{
			// r not registered under r->pathname(), unknown yet if r is registered
			// under a different name.  if so, find it and remove it.
			for (i = receivers_.begin(); i != receivers_.end() && i->second != r; i++) 
				;
			if (i != receivers_.end()) 
			{
				receivers_.erase(i);
			}
		} 
		else 
		{
		  // some other receiver is registered under r->pathname().
		  // the Reaper'd case.  maybe other cases?  noop is the correct response.
		}
	}
	else
	{
			for (i = receivers_.begin(); i != receivers_.end() && i->second != r; i++) 
				;
			if (i != receivers_.end()) 
			{
				receivers_.erase(i);
			}
	}
//     assert(receiver(r->pathname()) != r);
}

ossimPlanetActionReceiver* ossimPlanetActionRouter::receiver(const ossimString& receiverPathname) const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReceiverMutex);
    ossimPlanetActionReceiver* result = NULL;
    MapType::const_iterator r = receivers_.find(receiverPathname);
    
    if (r != receivers_.end())
	result = r->second; 
    
//     assert(result == NULL || result->pathname() == receiverPathname);
    
    return result;
}

void ossimPlanetActionRouter::printReceivers()
{
    int i = 0;
    MapType::iterator r;
    
    for (r = receivers_.begin(); r != receivers_.end(); r++)
	std::cout << i++ << " = " << r->first << std::endl;
}

// ossimPlanetActionReceiver features

void ossimPlanetActionRouter::execute(const ossimPlanetAction& action)
{
   const ossimPlanetDestinationCommandAction* a = action.toDestinationCommandAction();
   // for now only support :destination command <args> style actions;
   if(!a) return;
   
   ossimString command;
   a->command(command);
   if (command == "#") {
	// this action is a comment, so do nothing
    
    } else if (command == "#syntaxerror") {
	a->printError("illegal Action syntax");
	
    } else if (command == "echo") {
	std::cout << a->argListSource() << std::endl;

    } else if (command == "execfiles") {
	for (unsigned int i = 1; i <= a->argCount(); i++)
	    executeFile(a->arg(i));

    } else if (command == "execactions") {
	ossimPlanetDestinationCommandAction x;
    	for (unsigned int i = 1; i <= a->argCount(); i++) {
	    x.setSourceCode(a->arg(i));
	    route(x);
	}
	
    } else if (command == "shell") {
	system(a->argListSource().c_str());

    } 
    else if (command == "manualaction") 
    {
       std::cout << "Enter action\n> " << std::flush;
       ossimPlanetDestinationCommandAction x;
        std::cin >> x;
	route(x);

    } else if (command == "printreceivers") {
	printReceivers();

    } else if (command == "setfederatename") {
	if (a->argCount() == 1)
	    setFederateName(a->arg(1));
	else
	    a->printError("bad argument count");
        
    }
    else if (command == "opensocket")
    {
        if ((a->argCount() == 2 && isInt(a->arg(2))) || (a->argCount() == 3 && isInt(a->arg(2)) && isInt(a->arg(3)))) {
            char delimiter = a->argCount() == 2 ? '\n' : static_cast<char>(asInt(a->arg(3)));
	    ossimPlanetSocketNetworkConnection* s = new ossimPlanetSocketNetworkConnection(a->arg(1), asInt(a->arg(2)), delimiter);
	    if (!s->error().empty()) {
		a->printError("SocketNetworkConnection had this error: " + s->error());
		delete s;
	    } else
		addNetworkConnection(s);
	} else
	    a->printError("bad argument count or type, needs one string (host) and one int (port) and optionally one string ('nul' or 'newline')");
        
    }
    else if (command == "closesocket")
    {
	if (a->argCount() == 2 && isInt(a->arg(2)))
        {
            ossimString name = a->arg(1) + ":" + a->arg(2);
	    ossimPlanetSocketNetworkConnection* s = dynamic_cast<ossimPlanetSocketNetworkConnection*>(networkConnection(name));
	    if (s == NULL) 
		a->printError("could not find a SocketNetworkConnection named: " + name);
	    else
		removeNetworkConnection(s->name());
	    delete s;
	}
        else
	    a->printError("bad argument count or type, needs one string (host) and one int (port)");
        
    }
    else
    {
	a->printError("ActionRouter action not understood");
    }
}

// protected


ossimPlanetActionRouter* ossimPlanetActionRouter::instance_ = 0;

ossimPlanetActionRouter::ossimPlanetActionRouter() :
    federateName_("DefaultFederateName-")
{
    // NB: can't use setPathnameAndRegister here, it will trigger infinite
    // recursive calls to this ctor since instance_ is still NULL right now.
    setPathname(":");
    registerReceiver(this);
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif
    // Build a default process name that includes hostname and pid.
    char buffer[MAXHOSTNAMELEN];
    buffer[0] = '\0';
    gethostname(buffer, MAXHOSTNAMELEN);
    federateName_.append(buffer);
	federateName_.append("-");
#ifdef WIN32
	federateName_.append(asString(GetCurrentProcessId()));
#else
	federateName_.append(asString(getpid()));
#endif
}

ossimPlanetActionRouter::~ossimPlanetActionRouter()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReceiverMutex);
   receivers_.clear();
	if(theThreadQueue.valid())
	{
		theThreadQueue->cancel();
	}
    for (int i = 0; i < (int)network_.size(); i++)
        delete network_[i];
}

void ossimPlanetActionRouter::remoteRouteImplementation(const ossimPlanetAction& a, const ossimString& destination)
{
    if (destination != federateName_)
	for (int i = 0; i < (int)network_.size(); i++)
	    network_[i]->send(a, destination);
    
    if (destination.empty() || destination == federateName_)
        route(a);
}
