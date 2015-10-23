#include <algorithm>
#include <fstream>

#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetActionRouter.h>
#include <ossimPlanet/ossimPlanetInteractionController.h>
#include <ossim/base/ossimCommon.h>

ossimPlanetInteractionController* ossimPlanetInteractionController::instance_ = NULL;

ossimPlanetInteractionController::ossimPlanetInteractionController()
{
    setPathnameAndRegister(":iac");
}

ossimPlanetInteractionController::~ossimPlanetInteractionController()
{
    ossimPlanetActionRouter::instance()->unregisterReceiver(this);
   deviceList_.clear();
}

void ossimPlanetInteractionController::registerDevice(ossimPlanetInputDevice* d)
{
    //     assert(d != NULL);
    
    if(!d) return;
    
    deviceList_.push_back(d);
}

void ossimPlanetInteractionController::unregisterDevice(ossimPlanetInputDevice* d)
{
    //     assert(d != NULL);
    if(!d) return;
    DeviceList::iterator i = find(deviceList_.begin(), deviceList_.end(), d);
    if (i != deviceList_.end())
        deviceList_.erase(i);
}

void ossimPlanetInteractionController::processPendingInputs()
{
    for (int i = 0; i < (int)deviceList_.size(); i++)
	deviceList_[i]->processInput();
}

void ossimPlanetInteractionController::defineInteractionValuator(const std::string& name, double minValue, double maxValue)
{
    if(name.empty() || ossim::isnan(minValue) || ossim::isnan(maxValue)) return;
    //     assert(!name.empty());
    //     assert(!ossim::isnan(minValue));
    //     assert(!ossim::isnan(maxValue));
    
    valuators_[name] = ossimPlanetInteractionValuatorData(minValue, maxValue);
    
    //     assert(!ossim::isnan(interactionValuatorValue(name)));
}

float ossimPlanetInteractionController::interactionValuatorValue(const std::string& interactionValuator) const
{
    float result = ossim::nan();
    
    std::map<std::string, ossimPlanetInteractionValuatorData>::const_iterator i = valuators_.find(interactionValuator);
    if (i != valuators_.end())
	result = i->second.value;
    else
	std::cerr << "ossimPlanetInteractionController::interactionValuatorValue(): no defined interactionValuator " << interactionValuator << std::endl;
    return result;
}

void ossimPlanetInteractionController::bind(const std::string& event, const ossimPlanetAction& a)
{
    if(event.empty()) return;
    //     assert(!event.empty());

   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theBoundActionsMutex);
    boundActions_[event] = a.clone();
}

void ossimPlanetInteractionController::unbind(const std::string& event)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theBoundActionsMutex);
   std::map<std::string, osg::ref_ptr<ossimPlanetAction> >::iterator i = boundActions_.find(event);
    
    if (i != boundActions_.end())
    {
       boundActions_.erase(i);
    }
}

void ossimPlanetInteractionController::unbindAll()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theBoundActionsMutex);
    boundActions_.clear();
}

void ossimPlanetInteractionController::tie(const std::string& valuatorList)
{
   std::vector<std::string> list;
   bool unbalanced;
   mkUtils::lexBraceQuotedTokens(valuatorList, 0, " \t", &list, &unbalanced);
   
   if (!unbalanced && list.size() > 1) {
      std::vector<std::string>& v = deviceTies_[list[0]];  // deviceTies_[] creates a blank entry if needed
      v.clear();
      for (unsigned int i = 1; i < list.size(); i++)
         if (!ossim::isnan(interactionValuatorValue(list[i])))
            v.push_back(list[i]);
   } else
      std::cerr << "ossimPlanetInteractionController::tie() had malformed interactionValuatorList, ignoring: " << valuatorList << std::endl;
}

void ossimPlanetInteractionController::untie(const std::string& deviceValuator)
{
    std::map<std::string, std::vector<std::string> >::iterator i = deviceTies_.find(deviceValuator);
    if (i != deviceTies_.end())
        deviceTies_.erase(i);
}

void ossimPlanetInteractionController::untieAll()
{
    deviceTies_.clear();
}

void ossimPlanetInteractionController::writeConfiguration(std::ostream& stream) const
{
    for (std::map<std::string, std::vector<std::string> >::const_iterator i = deviceTies_.begin(); i != deviceTies_.end(); i++) {
        stream << ":iac tie " << i->first;
        const std::vector<std::string>& v = i->second;
        for (unsigned int j = 0; j < v.size(); j++)
            stream << ' ' << v[j];
        stream << std::endl;
    }
    
   for (std::map<std::string, osg::ref_ptr<ossimPlanetAction> >::const_iterator i = boundActions_.begin(); i != boundActions_.end(); i++)
        stream << ":iac bind " << i->first << " {" << *i->second << '}' << std::endl;
}

void ossimPlanetInteractionController::executeBoundAction(const std::string& event)
{
    // Debug::log("InteractionController_events") << "--- " << event << std::endl;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theBoundActionsMutex);
    
   std::map<std::string,osg::ref_ptr< ossimPlanetAction> >::const_iterator i = boundActions_.find(event);
    if (i != boundActions_.end()) 
	i->second->execute();
}

void ossimPlanetInteractionController::updateInteractionValuators(const std::string& deviceValuator, float normalizedValue)
{
    // XXX assert(mkUtils::inInterval(normalizedValue, 0.0f, 1.0f));
    
    // Debug::log("InteractionController_events") << "--- " << deviceValuator << " = " << normalizedValue << std::endl;
    
    std::map<std::string, std::vector<std::string> >::iterator i = deviceTies_.find(deviceValuator);
    if (i != deviceTies_.end())
    {
        std::vector<std::string>& v = i->second;
        for (unsigned int j = 0; j < v.size(); j++)
        {
            std::map<std::string, ossimPlanetInteractionValuatorData>::iterator iter = valuators_.find(v[j]);
            if (iter != valuators_.end())
            {
                // ossimPlanetInteractionValuatorData data& = (valuators_.find(v[j])->second);
                iter->second.value = normalizedValue*iter->second.maxMinusMin + iter->second.min;
            }
        }
    }
}

void ossimPlanetInteractionController::execute(const ossimPlanetAction& action)
{
   const ossimPlanetDestinationCommandAction* a = action.toDestinationCommandAction();
   const ossimPlanetXmlAction* xml = action.toXmlAction();
   if(a)
   {
      destinationCommandExecute(*a);
   }
   else if(xml)
   {
      xmlExecute(*xml);
   }
   
}

void ossimPlanetInteractionController::xmlExecute(const ossimPlanetXmlAction& a)
{
   ossimString command = a.command();
   if(command == "Bind")
   {
      ossimString bindString;
      ossimPlanetXmlAction tempAction;
      // need to create objects for these so we can do a common string encode.  We will do it here
      // for now and move out later for common use.
      //
      ossim_uint32 idx;
      const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();
      for(idx = 0; idx < children.size();++idx)
      {
         if(children[idx]->getTag() == "Keyboard")
         {
            ossimString key     = children[idx]->getAttributeValue("key");
            ossimString metaKey = children[idx]->getAttributeValue("metaKey");
            if(key == " ")
            {
               key = "space";
            }
            bindString = key;
            if(!metaKey.empty())
            {
               bindString = metaKey + "_" + bindString;
            }
            bindString += "_key";
         }
         else if(children[idx]->getTag() == "Mouse")
         {
            
         }
         else if(children[idx]->findAttribute("target").valid())
         {
            tempAction.setXmlNode((ossimXmlNode*)children[idx]->dup());
         }
         
         if(tempAction.xmlNode().valid())
         {
            bind(bindString, tempAction);
         }
      }
   }
   else if(command == "Unbind")
   {
      
   }
   else if(command == "UnbindAll")
   {
      unbindAll();
   }
}

void ossimPlanetInteractionController::destinationCommandExecute(const ossimPlanetDestinationCommandAction& a)
{
   
   ossimString command = a.command();
   if (command == "bind")
   {
      if (a.argCount() == 2)
      {
         if (!a.arg(1).empty())
            bind(a.arg(1), 
                 ossimPlanetDestinationCommandAction(a.arg(2)));
         else
            a.printError("cannot bind empty event name because this name is guaranteed a noop");
      } else
         a.printError("bad argument count");
      
   }
   else if (command == "unbind")
   {
      for (unsigned int i = 1; i <= a.argCount(); i++)
         unbind(a.arg(i));
      
   }
   else if (command == "unbindall")
   {
      unbindAll();
      
   }
   else if (command == "tie")
   {
      if (a.argCount() > 1)
         tie(a.argListSource());
      else
         a.printError("bad argument count: need device valuator followed by 1 or more interaction valuator");
      
   }
   else if (command == "untie")
   {
      for (unsigned int i = 1; i <= a.argCount(); i++)
         untie(a.arg(i));
      
   }
   else if (command == "untieall")
   {
      untieAll();
      
   }
   else if (command == "printvaluators")
   {
      std::map<std::string, ossimPlanetInteractionValuatorData>::const_iterator i;
      for (i = valuators_.begin(); i != valuators_.end(); i++)
         std::cout << i->first << " = " << i->second.value << "\trange [" << i->second.min << ',' << (i->second.maxMinusMin + i->second.min) << ']' << std::endl;
      
   }
   else if (command == "writeconfiguration")
   {
      if (a.argCount() == 0)
      {
         writeConfiguration(std::cout);
      }
      else if (a.argCount() == 1)
      {
         std::ofstream f(a.arg(1).c_str());
         if (!f)
            std::cerr << "cannot open file " << a.arg(1) << " for :iac writeconfiguration" << std::endl;
         else
            writeConfiguration(f);
      }
      else
         a.printError("bad argument count, must specify a filename, or no argument for stdout");
      
   }
   else if (command == "doevents")
   {
      for (unsigned int i = 1; i <= a.argCount(); i++)
         executeBoundAction(a.arg(i));
      
   }
   else if (command == "updatevaluators")
   {
      if (a.argCount() % 2 == 0) {
         for (unsigned int i = 1; i < a.argCount(); i += 2) {
            float x = mkUtils::asFloat(a.arg(i+1));
            if (mkUtils::inInterval(x, 0.0f, 1.0f)) {
               std::map<std::string, ossimPlanetInteractionValuatorData>::iterator iter = valuators_.find(a.arg(i));
               if (iter != valuators_.end())
               {
                  iter->second.value = x*iter->second.maxMinusMin + iter->second.min;
               }
               else 
                  a.printError("Undefined simulation variable " + a.arg(i));
            }
         }
      } else
         a.printError("bad arg count");
      
   } else {
      a.printError("ossimPlanetInteractionController Action not understood");
   }   
}
