#include <ossimPlanet/ossimPlanetAnimationPath.h>
#include <ossim/base/ossimXmlDocument.h>
#include <osg/io_utils>
#include <fstream>
#include <sstream>

ossimPlanetAnimationPath::ossimPlanetAnimationPath()
{
}

bool ossimPlanetAnimationPath::openAnimationPathByXmlDocument(const ossimFilename& animationFile)
{
   std::ifstream in(animationFile.c_str());
   if(in.good())
   {
      return setAnimationPathByXmlDocument(in);
   }
   
   return false;
}

bool ossimPlanetAnimationPath::setAnimationPathByXmlDocument(const ossimString& xml)
{
   std::istringstream in(xml.c_str());
   return setAnimationPathByXmlDocument(in);
}

bool ossimPlanetAnimationPath::setAnimationPathByXmlDocument(std::istream& xmlStream)
{
   bool result = false;
   if(!xmlStream) return result;
   
   ossimXmlDocument document;
   
   if(document.read(xmlStream))
   {
      result = setAnimationPathByXmlNode(document.getRoot());
   }
   
   return result;
}

bool ossimPlanetAnimationPath::openAnimationPathByXmlNode(const ossimFilename& animationFile)
{
   std::ifstream in(animationFile.c_str());
   if(in.good())
   {
      return setAnimationPathByXmlNode(in);
   }
   
   return false;
}

bool ossimPlanetAnimationPath::setAnimationPathByXmlNode(const ossimString& xml)
{
   std::istringstream in(xml.c_str());
   return setAnimationPathByXmlNode(in);
}

bool ossimPlanetAnimationPath::setAnimationPathByXmlNode(std::istream& xmlStream)
{
   bool result = false;
   if(!xmlStream) return result;
   
   ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
   
   if(node->read(xmlStream))
   {
      result = setAnimationPathByXmlNode(node.get());
   }
   
   return result;
}


ossimXmlNode* ossimPlanetAnimationPath::saveXml()const
{
   ossimXmlNode* root = new ossimXmlNode;
   ossimXmlNode* coordinates = new ossimXmlNode;
   ossimXmlNode* path = new ossimXmlNode;
   root->setTag("AnimationPath");
   path->setTag("GeospatialPath");
   coordinates->setTag("coordinates");
   path->setAttribute("timeUnit", "seconds", true);
   path->setAttribute("positionType", "latlonhgt", true);
   path->setAttribute("orientationType", "lsrhpr", true);
   
   path->addChildNode(coordinates);
   
   std::ostringstream coordinateList;
   
   root->addChildNode(path);
   
   TimeTupleMap::iterator iter    = thePath->timeTupleMap().begin();
   TimeTupleMap::iterator endIter    = thePath->timeTupleMap().end();
   while(iter!=endIter)
   {
      coordinateList << iter->first 
      << "," << iter->second.position()[0]
      << "," << iter->second.position()[1]
      << "," << iter->second.position()[2]
      << "," << iter->second.orientation()[0]
      << "," << iter->second.orientation()[1]
      << "," << iter->second.orientation()[2]
      << "," << iter->second.scale()[0]
      << "," << iter->second.scale()[1]
      << "," << iter->second.scale()[2]
      << " \n";
      ++iter;
   }
   coordinates->setText(coordinateList.str());
   
   return root;
}

bool ossimPlanetAnimationPath::setAnimationPathByXmlNode(ossimRefPtr<ossimXmlNode> pathNode)
{
   bool result = true;
   thePath = new GeospatialPath();
   if((pathNode->getTag() == "AnimationPath"))
   {
      ossimRefPtr<ossimXmlNode> path        = pathNode->findFirstNode("GeospatialPath");
      ossimRefPtr<ossimXmlNode> coordinates = pathNode->findFirstNode("GeospatialPath/coordinates");
      if(coordinates.valid()&&path.valid())
      {
         ossimString timeUnit        = path->getAttributeValue("timeUnit");
         ossimString positionType    = path->getAttributeValue("positionType");
         ossimString orientationType = path->getAttributeValue("orientationType");
         
         // only support for now decoding relative time in seconds
         // and lsr space type position and orientation.
         //
         if((timeUnit        == "seconds") &&
            (positionType    == "latlonhgt") &&
            (orientationType == "lsrhpr"))
         {
           std::istringstream in(coordinates->getText());
            ossimString tuple = "";
            while(in.good())
            {
               in >> ossim::skipws;
               tuple = "";
               while(in.good()&&!ossim::isWhiteSpace(in.peek()))
               {
                  tuple += static_cast<char>(in.get());
               }
               if(!tuple.empty())
               {
                  std::vector<ossimString> tupleValues;
                  tuple.split(tupleValues, ",");
                  if(tupleValues.size() >= 4) //time and position only
                  {
                     double t = tupleValues[0].toDouble(); // time
                     osg::Vec3d pos(tupleValues[1].toDouble(), // positional
                                    tupleValues[2].toDouble(),
                                    tupleValues[3].toDouble());
                     osg::Vec3d orient(0.0,0.0,0.0); // identity
                     osg::Vec3d scale(1.0,1.0,1.0); // identity
                     if(tupleValues.size() >= 7) // add orientation
                     {
                        orient = osg::Vec3d(tupleValues[4].toDouble(),
                                            tupleValues[5].toDouble(),
                                            tupleValues[6].toDouble());
                        if(tupleValues.size()==10)  // add scale to the animation path
                        {
                           scale = osg::Vec3d(tupleValues[7].toDouble(),
                                              tupleValues[8].toDouble(),
                                              tupleValues[9].toDouble());
                        }
                     }
                     thePath->timeTupleMap().insert(std::make_pair(t, Tuple(pos, orient, scale)));
                  }
                  else
                  {
                     result = false;
                  }
               }
            }
         }
         else
         {
            result = false;
         }
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = false;
   }
   
   return result;
}

bool ossimPlanetAnimationPath::getInterpolatedControlPoint(double time,
                                                           osg::AnimationPath::ControlPoint& controlPoint) const
{
   if(!theModel.valid()||!thePath.valid()||thePath->timeTupleMap().empty()) return false;
   
   time = adjustTime(time);
   TimeTupleMap::const_iterator second = thePath->timeTupleMap().lower_bound(time);
   if (second==thePath->timeTupleMap().begin())
   {
      // use the model to create the control point in osg form
      //
      osg::Quat q;
      osg::Vec3d pos;
      osg::Matrixd m;
      
      lsrMatrix(m, second->second);
      m = osg::Matrixd::scale(second->second.scale())*m;
      q.set(m);
      pos.set(m(3,0), m(3,1), m(3,2));
      controlPoint = ControlPoint(pos, q);
   }
   else if (second!=thePath->timeTupleMap().end())
   {
      TimeTupleMap::const_iterator first = second;
      --first;        
      
      // we have both a lower bound and the next item.
      
      // delta_time = second.time - first.time
      double delta_time = second->first - first->first;
      
      if (delta_time==0.0)
      {
         osg::Quat q;
         osg::Vec3d pos;
         osg::Matrixd m;
         
         lsrMatrix(m, first->second);
         q.set(m);
         pos.set(m(3,0), m(3,1), m(3,2));
         controlPoint = ControlPoint(pos, q);
      }
      else
      {
         ControlPoint one, two;
         osg::Quat q;
         osg::Vec3d pos;
         osg::Matrixd m;
         
         lsrMatrix(m, first->second);
         q.set(m);
         pos.set(m(3,0), m(3,1), m(3,2));
         one = ControlPoint(pos, q);
         lsrMatrix(m, second->second);
         q.set(m);
         pos.set(m(3,0), m(3,1), m(3,2));
         two = ControlPoint(pos, q);
         controlPoint.interpolate((time - first->first)/delta_time,
                                  one,
                                  two);
      }   
   }
   else // (second==_timeControlPointMap.end())
   {
      osg::Quat q;
      osg::Vec3d pos;
      osg::Matrixd m;
      
      lsrMatrix(m, thePath->timeTupleMap().rbegin()->second);
      q.set(m);
      pos.set(m(3,0), m(3,1), m(3,2));
      controlPoint = ControlPoint(pos, q);
   }
   return true;
}

double ossimPlanetAnimationPath::adjustTime(double t)const
{
   double time = t;
   switch(_loopMode)
   {
      case(SWING):
      {
         double modulated_time = (time - thePath->firstTime())/(thePath->period()*2.0);
         double fraction_part = modulated_time - floor(modulated_time);
         if (fraction_part>0.5) fraction_part = 1.0-fraction_part;
         
         time = thePath->firstTime()+(fraction_part*2.0) * thePath->period();
         break;
      }
      case(LOOP):
      {
         double modulated_time = (time - thePath->firstTime())/thePath->period();
         double fraction_part = modulated_time - floor(modulated_time);
         time = thePath->firstTime()+fraction_part * thePath->period();
         break;
      }
      case(NO_LOOPING):
         // no need to modulate the time.
         break;
   }
   
   return time;
}

void ossimPlanetAnimationPath::lsrMatrix(osg::Matrixd& result,
                                         const Tuple& tuple)const
{
   osg::Vec3d position =  tuple.position();
   theModel->mslToEllipsoidal(position); // adjust to ellipsoidal location
   theModel->orientationLsrMatrix(result,
                                  position,
                                  tuple.orientation()[0],
                                  tuple.orientation()[1],
                                  tuple.orientation()[2]);
   result = osg::Matrixd::scale(tuple.scale())*result;
   
}

bool ossimPlanetAnimationPath::generateWorldCoordinates(PointList& worldPoints)const
{
   if(!theModel.valid()&&thePath.valid())
   {
      return false;
   }
   TimeTupleMap::const_iterator iter    = thePath->timeTupleMap().begin();
   TimeTupleMap::const_iterator endIter = thePath->timeTupleMap().end();
   
   while(iter!=endIter)
   {
      osg::Vec3d worldPoint;
      // convert Lat lon Height to world point
      //
      osg::Vec3d position(iter->second.position());
      
      // shift to ellipsoid
      //
      theModel->mslToEllipsoidal(position);
      theModel->forward(position, worldPoint);
      worldPoints.push_back(worldPoint);
      
      ++iter;
   }
   
   return true;
}

bool ossimPlanetAnimationPath::generateModelCoordinates(PointList& modelPoints)const
{
   TimeTupleMap::const_iterator iter    = thePath->timeTupleMap().begin();
   TimeTupleMap::const_iterator endIter = thePath->timeTupleMap().end();
   
   while(iter!=endIter)
   {
      modelPoints.push_back(iter->second.position());
      ++iter;
   }
   
   return true;
}

bool ossimPlanetAnimationPath::moveToLocationLatLon(const osg::Vec2d& llh)
{
   PointList worldPoints;
   bool result = false;
   if(generateWorldCoordinates(worldPoints)&&
      (worldPoints.size()))
   {
      osg::Vec3d centerWorld;
      ossim_uint32 idx = 0;
      ossim_uint32 size = worldPoints.size();
      for(idx = 0; idx < size; ++idx)
      {
         centerWorld += worldPoints[idx];
      }
      centerWorld *= (1.0/(double)size);
      osg::Vec3d centerWorldLlh;
      theModel->xyzToLatLonHeight(centerWorld, centerWorldLlh);
      osg::Vec3d centerDistinationLlh(llh[0],
                                      llh[1],
                                      centerWorldLlh[2]);
      
      osg::Matrixd lsrSourceMatrix;
      osg::Matrixd lsrDestinationMatrix;
      theModel->lsrMatrix(centerWorldLlh, lsrSourceMatrix);
      theModel->lsrMatrix(centerDistinationLlh, lsrDestinationMatrix);
      
      osg::Matrixd sourceInvert(osg::Matrixd::inverse(lsrSourceMatrix));
      
      for(idx = 0; idx < size; ++idx)
      {
         osg::Vec3d relative = worldPoints[idx]*sourceInvert;
         osg::Vec3d newPoint = relative*lsrDestinationMatrix;
         worldPoints[idx] = newPoint;
      }
      TimeTupleMap::iterator iter    = thePath->timeTupleMap().begin();
      osg::Vec3d newModelPoint;
      for(idx = 0; idx < size; ++idx)
      {
         theModel->xyzToLatLonHeight(worldPoints[idx], newModelPoint);
         theModel->ellipsoidalToMsl(newModelPoint);
         iter->second.setPosition(newModelPoint);
         ++iter;
      }
      result = true;
   }
   
   return result;
}

