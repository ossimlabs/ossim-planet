// Written by Patrick Melody @ NRL

#ifndef ossimPlanetIdolBridge_HEADER
#define ossimPlanetIdolBridge_HEADER

#include <map>
#include <osg/Group>
#include "ossimPlanet/ossimPlanetActionReceiver.h"
#include "ossimPlanet/ossimPlanetIdolLayer.h"

class ossimPlanetIdolBridge : public ossimPlanetActionReceiver {
public:
    ossimPlanetIdolBridge() :
        layerGroup_(new osg::Group)
        {
            setPathnameAndRegister(":idolbridge");
        }
    
    ~ossimPlanetIdolBridge() 
        {
            for (std::map<std::string, ossimPlanetIdolLayer*>::iterator i = layers_.begin(); i != layers_.end(); i++) {
                layerGroup_->removeChild(i->second->layerRoot());
                delete i->second;
            }
        }
    
    void execute(const ossimPlanetAction &a)
        {
            if (a.command() == "gotolatlonelev") {
                // this action updates our current position to idol.
                // we do nothing. this action is being broadcast so all
                // idol data servers see it, and it is intended for them.
            
	    } else if (a.command() == "movechild") {
                if (a.argCount() == 5) {
                    std::map<std::string, ossimPlanetIdolLayer*>::iterator i = layers_.find(a.arg(1));
                    if (i != layers_.end())
                        i->second->moveChild(a);
                    else
                        a.printError("no such layer");
		} else
                    a.printError("bad arg count, need layername,childname,lat,lon,elev");
                    
            } else if (a.command() == "addchild") {
                if (a.argCount() == 6) {
                    std::map<std::string, ossimPlanetIdolLayer*>::iterator i = layers_.find(a.arg(1));
                    if (i != layers_.end())
                        i->second->addChild(a);
                    else
                        a.printError("no such layer");
                } else
                    a.printError("bad arg count, need layername,childname,lat,lon,elev,modeldata");

	    } else if (a.command() == "removechild") {
                if (a.argCount() == 2) {
                    std::map<std::string, ossimPlanetIdolLayer*>::iterator i = layers_.find(a.arg(1));
                    if (i != layers_.end())
                        i->second->removeChild(a);
                    else
                        a.printError("no such layer");
                } else 
                    a.printError("bad arg count, need layername,childname");
                    
            } else if (a.command() == "createlayer") {
                if (a.argCount() == 1) {
                    std::map<std::string, ossimPlanetIdolLayer*>::iterator i = layers_.find(a.arg(1));
                    if (i == layers_.end()) {
                        layers_[a.arg(1)] = new ossimPlanetIdolLayer(a.arg(1));
                        layerGroup_->addChild(layers_[a.arg(1)]->layerRoot());
                    } else
                        a.printError("layer already exists");
                } else
                    a.printError("bad arg count");
                    
            } else if (a.command() == "deletelayer") {
                if (a.argCount() == 1) {
                    std::map<std::string, ossimPlanetIdolLayer*>::iterator i = layers_.find(a.arg(1));
                    if (i != layers_.end()) {
                        layerGroup_->removeChild(i->second->layerRoot());
                        delete i->second;
                        layers_.erase(i);
                    } else
                        a.printError("no such layer");
                } else
                    a.printError("bad arg count");
                    
            } else
                a.printError("bad command");
        }
    
    osg::ref_ptr<osg::Group> layerGroup() const
        // this group is attached to the scenegraph by class ossimPlanet.  idol layer groups live under it.
        { return layerGroup_; }
    
protected:
    osg::ref_ptr<osg::Group> layerGroup_;
        // this group is attached to the scenegraph by class ossimPlanet.  idol layer groups live under it.
    
    std::map<std::string, ossimPlanetIdolLayer*> layers_;
	// all our current layers indexed by layer name
    
};

#endif

