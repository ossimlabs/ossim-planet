#ifndef ossimPlanetInteractionController_HEADER
#define ossimPlanetInteractionController_HEADER

// Control mapping configuration, notify sim of user input.
// Events are discrete inputs, eg button presses, bound to actions. 
// Events beginning with a '-' are reserved for commandline arguments.
// Device valuators are continuous inputs, eg mouse position, tied    
// to interaction valuators.  

#include <assert.h>
#include <iostream>
#include <map>
#include <vector>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossimPlanet/ossimPlanetInputDevice.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <OpenThreads/ScopedLock>
#include <osg/ref_ptr>

class OSSIMPLANET_DLL ossimPlanetInteractionController : public ossimPlanetActionReceiver
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetInputDevice> > DeviceList;
   
   // singleton routines
   static ossimPlanetInteractionController* instance();
	// pointer to the lazy initialized InteractionController singleton
	// assert(instance() != NULL)
   
   static void shutdown();
	// clean up the singleton
   
   // input management
   void registerDevice(ossimPlanetInputDevice* d);
	// add an input device to the list of devices we watch
	// assert(d != NULL)
	
   void unregisterDevice(ossimPlanetInputDevice* d);
	// remove an input device from the list of devices we watch
	// assert(d != NULL)
   
   void processPendingInputs();
	// generate all actions and update all variables required  
	//    by any user input to a registered device that occured  
	//    since last call.
   
   void defineInteractionValuator(const std::string& name, double minValue, double maxValue);
	// define an interaction valuator
	// assert(!name.empty());
	// assert(!ossim::isnan(minValue));
	// assert(!ossim::isnan(maxValue));
   
   float interactionValuatorValue(const std::string& interactionValuator) const;
	// value of an interaction valuator, or NAN if undefined valuator
	
   // user control configuration
   void bind(const std::string& event, const ossimPlanetAction& a);
	// bind an event to an action
	// assert(!event.empty())    // reserve empty event name for noops
	
   void unbind(const std::string& event);
	// unbind event
	
   void unbindAll();
	// unbind all events
	
   void tie(const std::string& valuatorList);
	// given a brace quoted token list, treat the first token as a device valuator name,
	//    and the rest of the tokens as virtual interaction valuator names.  untie the
	//    device valuator from any previous ties and then tie it to all the listed 
	//    interaction valuators that are currently defined.
	
   void untie(const std::string& deviceValuator);
	// untie a device valuator from all its virtual interaction valuators
	
   void untieAll();
	// untie all device valuators from all their virtual interaction valuators
   
   void writeConfiguration(std::ostream& stream) const;
	// output an action script that will restore the current bindings/ties
   
   // routines called by InputDevice objects
   void executeBoundAction(const std::string& event);
	// execute the action bound to event, no op if no binding
   
   void updateInteractionValuators(const std::string& deviceValuator, float normalizedValue);
	// update value of all interaction valuators tied to deviceValuator
	// assert(mkUtils::inInterval(normalizedValue, 0.0f, 1.0f))
   
   // ActionReceiver routines
   void execute(const ossimPlanetAction& a);
	// execute the given action
   
protected:
   ossimPlanetInteractionController();
   ossimPlanetInteractionController(const ossimPlanetInteractionController& src):ossimPlanetActionReceiver(src) {}
   ~ossimPlanetInteractionController();
   ossimPlanetInteractionController& operator=(ossimPlanetInteractionController& ) {assert(false); return *this;}
   
   void xmlExecute(const ossimPlanetXmlAction& a);
   void destinationCommandExecute(const ossimPlanetDestinationCommandAction& a);
   static ossimPlanetInteractionController* instance_;
	// our singleton
   
   DeviceList deviceList_;
	// registered input devices 
   
   mutable ossimPlanetReentrantMutex theBoundActionsMutex;
   std::map<std::string, osg::ref_ptr<ossimPlanetAction> > boundActions_;
	// current event to action bindings
   
   class ossimPlanetInteractionValuatorData 
      {
      public:
         double min;
         double maxMinusMin;
         double value;
         ossimPlanetInteractionValuatorData() : min(0.0), maxMinusMin(1.0), value(0.0) {}
         ossimPlanetInteractionValuatorData(float minVal, float maxVal) : min(minVal), maxMinusMin(maxVal-minVal), value(minVal) {}
      };
   // valuator data tuple
	
   std::map<std::string, ossimPlanetInteractionValuatorData> valuators_;
   // all defined interaction valuators
   
   std::map<std::string, std::vector<std::string> > deviceTies_;
	// device valuators and the interaction valuators they're tied to
};

inline ossimPlanetInteractionController* ossimPlanetInteractionController::instance()
{
   if (ossimPlanetInteractionController::instance_ == 0)
   {
      ossimPlanetInteractionController::instance_ = new ossimPlanetInteractionController();
   }
   assert(ossimPlanetInteractionController::instance_ != NULL);
   return ossimPlanetInteractionController::instance_;
}

inline void ossimPlanetInteractionController::shutdown()
{
   if(ossimPlanetInteractionController::instance_)
   {
      delete ossimPlanetInteractionController::instance_;
      ossimPlanetInteractionController::instance_ = 0;
   }
}

#endif

