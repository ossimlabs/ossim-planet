// a data layer loaded by idol, controlled by an ossimPlanetIdolBridge

// Written by Patrick Melody @ NRL

#ifndef ossimPlanetIdolLayer_HEADER
#define ossimPlanetIdolLayer_HEADER

#include <vector>
#include <map>
#include <string>
#include <osg/MatrixTransform>
#include <osg/Group>
#include <osg/Geode>
#include <osgText/Text>
#include <osgDB/ReadFile> 
#include "ossimPlanet/ossimPlanetAction.h"
#include "ossimPlanet/mkUtils.h"

class ossimPlanetIdolLayer {
public:
    ossimPlanetIdolLayer(const std::string& name) :
	name_(name),
	layerRoot_(new osg::Group)
	{ }
    
    ~ossimPlanetIdolLayer()
	{
	    for (std::map<std::string, osg::MatrixTransform*>::iterator i = children_.begin(); i != children_.end(); i++)
		i->second->unref();
	}
    	
    void addChild(const ossimPlanetAction &a)
	// create and add a child;  args are layername childname lat lon elev modeldescription
	{
	    if (children_.find(a.arg(2)) == children_.end()) {
                osg::Node* model = createModelFromDescription(a.arg(6));
                if (model != NULL) {
                    osg::MatrixTransform* xform = new osg::MatrixTransform; 
                    xform->ref();
                    setTransform(xform, mkUtils::asDouble(a.arg(3)), mkUtils::asDouble(a.arg(4)), mkUtils::asDouble(a.arg(5)));
                    xform->addChild(model);            
                    layerRoot_->addChild(xform);
                    children_[a.arg(2)] = xform;
                } else
                    a.printError("could not create model from model description");
	    } else
                a.printError("already have a child with that name");
	}
	
    void moveChild(const ossimPlanetAction &a)
	// move a child;  args are layername childname lat lon elev
	{
	    std::map<std::string, osg::MatrixTransform*>::iterator i = children_.find(a.arg(2));
	    if (i != children_.end())
                setTransform(i->second, mkUtils::asDouble(a.arg(3)), mkUtils::asDouble(a.arg(4)), mkUtils::asDouble(a.arg(5)));
	    else
                a.printError("no such child");
	}
	
    void removeChild(const ossimPlanetAction &a)
	// remove a child
	{
	    std::map<std::string, osg::MatrixTransform*>::iterator i = children_.find(a.arg(2));
	    if (i != children_.end()) {
		layerRoot_->removeChild(i->second);
		i->second->unref();
		children_.erase(i);
	    } else
                a.printError("no such child");
	}
	
    const std::string& name() const 
	// name of this layer
	{ return name_; }
	
    osg::Group* layerRoot() const
	// root of this layer's scene subgraph 
	{ return layerRoot_.get(); }  

protected:
    std::string name_;
	// layer name

    osg::ref_ptr<osg::Group> layerRoot_;
	// root of this layer's scene subgraph 
	
    std::map<std::string, osg::MatrixTransform*> children_;
	// collection of current children indexed by childname
    
    osg::Node* createModelFromDescription(const std::string& modelDescription) const
	// create model
        // modeldescription := (text labeltext r g b a scale) | (model filename scale)
	{
            osg::Node* result = NULL;
            
	    std::vector<std::string> tokens;
            bool unbalancedBraces;
	    mkUtils::lexBraceQuotedTokens(modelDescription, 0, " \t", &tokens, &unbalancedBraces);
	    
            if (!unbalancedBraces && tokens.size() > 0) {
                if (tokens[0] == "text" && tokens.size() == 7) {
                    osgText::Text* t = new osgText::Text;
                    t->setFont();
                    t->setCharacterSize(6.0f*mkUtils::asDouble(tokens[6]));
                    t->setColor(osg::Vec4f(mkUtils::asDouble(tokens[2]), mkUtils::asDouble(tokens[3]), mkUtils::asDouble(tokens[4]), mkUtils::asDouble(tokens[5])));
                    t->setAlignment(osgText::Text::CENTER_CENTER);
                    t->setText(tokens[1]);
                    osg::Geode* geode = new osg::Geode;
                    geode->addDrawable(t);
                    result = geode;
                    
                } else if (tokens[0] == "model" && tokens.size() == 3) {
                    osg::Node* model = osgDB::readNodeFile(tokens[1].c_str());
                    if (model != NULL) {
                        double scale = mkUtils::asDouble(tokens[2]);
                        osg::MatrixTransform* scaleNode = new osg::MatrixTransform(osg::Matrixf::scale(scale,scale,scale)); 
                        scaleNode->addChild(model);
                        result = scaleNode;
                    }
                }
            }
            
            return result;
        }
        
    void setTransform(osg::MatrixTransform* transform, double lat, double lon, double elev) const
	// set transform to place geometry at lat,lon,elev
	{
            const osg::Vec3d zAxis(0.0, 0.0, 1.0);
            
            double planetRadius = 6378137.0; // XXX should really compute current planet's radius at lat/lon
            osg::Vec3d v = mkUtils::modsphere2xyz(1.0 + elev/planetRadius, osg::DegreesToRadians(lon), osg::DegreesToRadians(lat));
            
            transform->setMatrix(osg::Matrixd::rotate(osg::DegreesToRadians(lon + 90.0), zAxis));
            transform->postMult(osg::Matrixd::rotate(zAxis, v));
            transform->postMult(osg::Matrixd::translate(v));
        }
};

#endif
