#ifndef ossimPlanetPrimaryBody_HEADER
#define ossimPlanetPrimaryBody_HEADER
#include <osg/Referenced>
// data associated with astronomical bodies (see bottom for earth and moon values)

#include <math.h>
#include <ossimPlanet/mkUtils.h>
#include <string>
/* #include "eassert.h" */
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetPrimaryBody : public osg::Referenced
{
public:
    ossimPlanetPrimaryBody(const std::string& name, double equatorialRadius, double polarRadius, double signedSecondsPerDay, double mass, double minLon, double maxLon) :
	name_(name), 
	equatorialRadius_(equatorialRadius), 
	polarRadius_(polarRadius),
	rotationSpeed_(360.0/signedSecondsPerDay), 
	secondsPerDay_(fabs(signedSecondsPerDay)), 
	mass_(mass), 
	mu_(mass*6.67259e-20), // mass * newton's universal gravitational constant in km not meters
	minLon_(minLon), 
	maxLon_(maxLon)
	{
/* 	    require(!name.empty()); */
/* 	    require(equatorialRadius > 0.0); */
/* 	    require(polarRadius > 0.0); */
/* 	    require(mass > 0.0); */
/* 	    require(minLon < maxLon); */
	    
	    if (secondsPerDay_ == 0.0)
		rotationSpeed_ = 0.0;
	}
    
    const std::string& name() const
	// name of the body
	{ return name_; }
    
    double equatorialRadius() const
	// equatorial radius in meters  (aka a)
	{ return equatorialRadius_; }
    
    double polarRadius() const
	// polar radius in meters  (aka b)
	{ return polarRadius_; }
	
    double meanRadius() const
	// mean of equatorialRadius() and polarRadius()
	{ return (equatorialRadius_ + polarRadius_)/2.0; }
	
    double radius(double latitudeDeg) const
	// distance from center of primary to mean sea level (?) at given latitude
	{
           double c  = cos(osg::DegreesToRadians(latitudeDeg)) / equatorialRadius_;
           double s = sin(osg::DegreesToRadians(latitudeDeg)) / polarRadius_;
           return sqrt(1.0/(c*c + s*s));
	}

    double rotationSpeed() const
	// rotation speed in degrees per second, anticlockwise from north pole
	{ return rotationSpeed_; }

    double secondsPerDay() const
	// number of seconds in a day
	{ return secondsPerDay_; }

    double mass() const
	// mass in kg
	{ return mass_; }

    double mu() const
	// mass * newton universal gravitational constant in km^3/s^2
	{ return mu_; }

    double minLon() const
	// min longitude
	{ return minLon_; }

    double maxLon() const
	// max longitude
	{ return maxLon_; }


protected:
    std::string name_;
	// primary name
    
    double equatorialRadius_;
	// equatorial radius in meters  (aka a)
    
    double polarRadius_;
	// polar radius in meters  (aka b)
	
    double rotationSpeed_;
	// rotation speed in degrees per second 

    double secondsPerDay_;
	// number of seconds in a day

    double mass_;
	// mass in kg

    double mu_;
	// mass * newton universal gravitational constant in km^3/s^2

    double minLon_;
	// min longitude

    double maxLon_;
	// max longitude

};

#endif


/*
    # arglist values are:
    #   primary name
    #   equatorial radius in meters
    #   polar radius in meters
    #   seconds per day (negative if rotates clockwise from north pole, 0 if no rotation)
    #   mass in kg
    #   min longitude
    #   max longitude
    
    earth_wgs84   6378137.0    6356752.3142  86400  5.9742e24   -180  180  
    moon          1737400.0    1737400.0     0      7.3483e22   0     360     
 */
