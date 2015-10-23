#include <iostream>
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossimPlanet/ossimPlanetActionRouter.h>
#include <ossimPlanet/mkUtils.h>
#include <ossim/base/ossimNotifyContext.h>
#include <ossim/base/ossimCommon.h>

ossimPlanetAction::ossimPlanetAction(const ossimString& originatingFederate) :
    theOrigin(originatingFederate)
{
}



void ossimPlanetAction::printError(const char* message) const
{
   ossimString code;
   sourceCode(code);
   ossimNotify(ossimNotifyLevel_WARN) << "ossimPlanetAction Error (" << message << ") \"" << code << '"' << std::endl;
}

void ossimPlanetAction::printError(const ossimString& message) const
{
    printError(message.c_str());
}

void ossimPlanetAction::execute() const
{
    ossimPlanetActionRouter::instance()->route(*this);
}

void ossimPlanetAction::post()const
{
   ossimPlanetActionRouter::instance()->post(*this);
}

void ossimPlanetAction::allExecute() const
{
    ossimPlanetActionRouter::instance()->allRoute(*this);
}

void ossimPlanetAction::tellExecute(const ossimString& destination) const
{
    ossimPlanetActionRouter::instance()->tellRoute(*this, destination);
}


// protected


const ossimString& ossimPlanetAction::defaultOrigin()
{
    return ossimPlanetActionRouter::instance()->federateName();
}

