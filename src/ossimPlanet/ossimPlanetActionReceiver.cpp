#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetActionRouter.h>

ossimPlanetActionReceiver::~ossimPlanetActionReceiver()
{
   if (pathname_ != ":")
      ossimPlanetActionRouter::instance()->unregisterReceiver(this);
}

std::string ossimPlanetActionReceiver::name() const
{
   return pathname_.substr(pathname_.rfind(':')+1, pathname_.length());
}

void ossimPlanetActionReceiver::setPathname(const std::string& newPath)
{
   pathname_ = newPath;
}

void ossimPlanetActionReceiver::setPathnameAndRegister(const std::string& newPath)
{
   // we will unregister if already registered
   //
   ossimPlanetActionReceiver* r = ossimPlanetActionRouter::instance()->receiver(pathname());
   if(r==this) // if we are ourselves then we can change our pathname
   {
      ossimPlanetActionRouter::instance()->unregisterReceiver(this);
   }      
   setPathname(newPath);
   ossimPlanetActionRouter::instance()->registerReceiver(this);
}
