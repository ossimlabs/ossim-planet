#ifndef ossimPlanetNetworkConnection_HEADER
#define ossimPlanetNetworkConnection_HEADER

// abstract interface to network.

#include <string>
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossimPlanet/ossimPlanetExport.h>


class OSSIMPLANET_DLL ossimPlanetNetworkConnection
{
public:
   ossimPlanetNetworkConnection(const std::string& name) : name_(name) {}
    virtual ~ossimPlanetNetworkConnection() {}

    const std::string& name() const
        // name of the connection
        { return name_; }

    const std::string& error() const
        // current error status, empty if no error.
        { return error_; }

    virtual void send(const ossimPlanetAction& a, const std::string& destination) = 0;
        // send action to the specified destination host.

    virtual void receive() = 0;
        // read any pending incoming actions and execute them.
        // should discard any incoming sent by me where destination is me or all

protected:
    std::string name_;
        // name of the connection

    std::string error_;
        // current error status, empty if no error.
};


#endif
