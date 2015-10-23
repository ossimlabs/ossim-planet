/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/

#include <iostream>
#include <ossimPlanet/ossimPlanetFadeText.h>
#include <osg/Notify>
#include <osg/io_utils>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <osgUtil/CullVisitor>
#include <ossim/base/ossimCommon.h>

// using namespace osgText;

// struct ossimPlanetFadeTextData : public osg::Referenced
// {
//     ossimPlanetFadeTextData(ossimPlanetFadeText* fadeText=0):
//         _fadeText(fadeText),
//         _visible(true) {}
        
//     bool operator < (const ossimPlanetFadeTextData& rhs) const
//     {
//         return _fadeText < rhs._fadeText;
//     }    
    
//     double getNearestZ() const
//     {
//         double nearestZ = _vertices[0].z();
//         if (nearestZ < _vertices[1].z()) nearestZ = _vertices[1].z();
//         if (nearestZ < _vertices[2].z()) nearestZ = _vertices[2].z();
//         if (nearestZ < _vertices[3].z()) nearestZ = _vertices[3].z();

//         // osg::notify(osg::NOTICE)<<"getNearestZ()="<<_fadeText->getText().createUTF8EncodedString()<<" "<<nearestZ<<std::endl;

//         return nearestZ;
//     }

//     ossimPlanetFadeText* _fadeText;
//     osg::Vec3d           _vertices[4];
//     bool                 _visible;
// };

// struct ossimPlanetFadeTextPolytopeData : public ossimPlanetFadeTextData, public osg::Polytope
// {
//     ossimPlanetFadeTextPolytopeData(ossimPlanetFadeTextData& fadeTextData):
//         ossimPlanetFadeTextData(fadeTextData)
//     {
//         _referenceVertexList.push_back(_vertices[0]);
//         _referenceVertexList.push_back(_vertices[1]);
//         _referenceVertexList.push_back(_vertices[2]);
//         _referenceVertexList.push_back(_vertices[3]);
//     }
    
//     void addEdgePlane(const osg::Vec3& corner, const osg::Vec3& edge)
//     {
//         osg::Vec3 normal( edge.y(), -edge.x(), 0.0f);
//         normal.normalize();
        
//         add(osg::Plane(normal, corner));
//     }
     
//     void buildPolytope()
//     {
//         osg::Vec3d edge01 = _vertices[1] - _vertices[0];
//         osg::Vec3d edge12 = _vertices[2] - _vertices[1];
//         osg::Vec3d edge23 = _vertices[3] - _vertices[2];
//         osg::Vec3d edge30 = _vertices[0] - _vertices[3];

//         osg::Vec3d normalFrontFace = edge01 ^ edge12;
//         bool needToFlip = normalFrontFace.z()>0.0f;

//         normalFrontFace.normalize();
//         add(osg::Plane(normalFrontFace, _vertices[0]));

//         add(osg::Plane( osg::Vec3d(0.0f,0.0f,0.0f), _vertices[0], _vertices[1]));
//         add(osg::Plane( osg::Vec3d(0.0f,0.0f,0.0f), _vertices[1], _vertices[2]));
//         add(osg::Plane( osg::Vec3d(0.0f,0.0f,0.0f), _vertices[2], _vertices[3]));
//         add(osg::Plane( osg::Vec3d(0.0f,0.0f,0.0f), _vertices[3], _vertices[0]));
        
// #if 0
//         osg::notify(osg::NOTICE)<<" normalFrontFace = "<<normalFrontFace<<std::endl;
//         osg::notify(osg::NOTICE)<<" edge01 = "<<edge01<<std::endl;
//         osg::notify(osg::NOTICE)<<" edge12 = "<<edge12<<std::endl;
//         osg::notify(osg::NOTICE)<<" edge23 = "<<edge23<<std::endl;
//         osg::notify(osg::NOTICE)<<" _vertices[0]= "<<_vertices[0]<<std::endl;
//         osg::notify(osg::NOTICE)<<" _vertices[1]= "<<_vertices[1]<<std::endl;
//         osg::notify(osg::NOTICE)<<" _vertices[2]= "<<_vertices[2]<<std::endl;
//         osg::notify(osg::NOTICE)<<" _vertices[3]= "<<_vertices[3]<<std::endl;
// #endif

//         if (needToFlip) flip();

// #if 0        
//         osg::notify(osg::NOTICE)<<"   plane 0 "<< _planeList[0]<<std::endl;
//         osg::notify(osg::NOTICE)<<"   plane 1 "<< _planeList[1]<<std::endl;
//         osg::notify(osg::NOTICE)<<"   plane 2 "<< _planeList[2]<<std::endl;
//         osg::notify(osg::NOTICE)<<"   plane 3 "<< _planeList[3]<<std::endl;
//         osg::notify(osg::NOTICE)<<"   plane 4 "<< _planeList[4]<<std::endl;
// #endif
        
//     }
    
//     inline bool contains(const std::vector<osg::Vec3>& vertices)
//     {
//         for(std::vector<osg::Vec3>::const_iterator itr = vertices.begin();
//             itr != vertices.end();
//             ++itr)
//         {
//             if (osg::Polytope::contains(*itr))
//             {
//                 return true;
//             }
//         }
//         return false;
//     }
    
// };

// struct ossimPlanetFadeTextUserData : public osg::Referenced
// {
//     ossimPlanetFadeTextUserData():
//         _frameNumber(0) {}

//     typedef std::list<ossimPlanetFadeTextData> FadeTextList;
//     unsigned int _frameNumber;
//     FadeTextList _fadeTextInView;
// };

// struct ossimPlanetGlobalFadeText : public osg::Referenced
// {
//     typedef std::set< osg::ref_ptr<ossimPlanetFadeTextUserData> > UserDataSet;
//     typedef std::set<ossimPlanetFadeText*> FadeTextSet;
//     typedef std::multimap<double, osg::ref_ptr<ossimPlanetFadeTextPolytopeData> > FadeTextPolytopeMap;
//     typedef std::map<osg::View*, UserDataSet> ViewUserDataMap;
//     typedef std::map<osg::View*, FadeTextSet > ViewFadeTextMap;

//     ossimPlanetGlobalFadeText():
//         _frameNumber(0xffffffff)
//     {
//     }

    
//     ossimPlanetFadeTextUserData* createNewFadeTextUserData(osg::View* view)
//     {
//         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        
//         ossimPlanetFadeTextUserData* userData = new ossimPlanetFadeTextUserData;

//         if (!userData)
//         {
//             osg::notify(osg::NOTICE)<<"Memory error, unable to create FadeTextUserData."<<std::endl;
//             return 0;
//         }

//         _viewMap[view].insert(userData);
        
//         return userData;
//     }
    
    
//     void update(unsigned int frameNumber)
//     {
//         _frameNumber = frameNumber;
//         for(ossimPlanetGlobalFadeText::ViewUserDataMap::iterator vitr = _viewMap.begin();
//             vitr != _viewMap.end();
//             ++vitr)
//         {
//             osg::View* view = vitr->first;

//             FadeTextSet& fadeTextSet = _viewFadeTextMap[view];
//             fadeTextSet.clear();

//             FadeTextPolytopeMap fadeTextPolytopeMap;

//             for(ossimPlanetGlobalFadeText::UserDataSet::iterator uitr = vitr->second.begin();
//                 uitr != vitr->second.end();
//                 ++uitr)
//             {
//                 ossimPlanetFadeTextUserData* userData = uitr->get();
                
//                 int frameDelta = frameNumber - userData->_frameNumber;
//                 if (frameDelta<=1)
//                 {
//                     for(ossimPlanetFadeTextUserData::FadeTextList::iterator fitr = userData->_fadeTextInView.begin();
//                         (fitr != userData->_fadeTextInView.end());
//                         ++fitr)
//                     {
//                         ossimPlanetFadeTextData& fadeTextData = *fitr;
//                         if (fadeTextSet.count(fadeTextData._fadeText)==0)
//                         {
//                             fadeTextSet.insert(fadeTextData._fadeText);
//                             fadeTextPolytopeMap.insert(FadeTextPolytopeMap::value_type(
//                                 -fadeTextData.getNearestZ(), new ossimPlanetFadeTextPolytopeData(fadeTextData)));
//                         }
//                     }
//                 }
//             }

// #if 0
//             // for each FadeTexPoltopeData            
//             //    create polytopes
//             //    test against all FTPD's later in the list
//             //       test all control points on FTPD against each plane of the current polytope
//             //       if all control points removed or outside then discard FTPD and make FT visible = false;
            
//             FadeTextPolytopeMap::iterator outer_itr = fadeTextPolytopeMap.begin();                
//             while (outer_itr != fadeTextPolytopeMap.end()) 
//             {
//                 FadeTextPolytopeMap::iterator inner_itr = outer_itr;
//                 ++inner_itr;

//                 if (inner_itr == fadeTextPolytopeMap.end()) break;

//                 ossimPlanetFadeTextPolytopeData& outer_ftpm = *(outer_itr->second);
//                 outer_ftpm.buildPolytope();

//                 // osg::notify(osg::NOTICE)<<"Outer z "<<outer_ftpm.getNearestZ()<<std::endl;

//                 while(inner_itr != fadeTextPolytopeMap.end())
//                 {
//                     ossimPlanetFadeTextPolytopeData& inner_ftpm = *(inner_itr->second);
                    
//                     // osg::notify(osg::NOTICE)<<"Inner z "<<inner_ftpm.getNearestZ()<<std::endl;

//                     if (outer_ftpm.contains(inner_ftpm.getReferenceVertexList()))
//                     {
//                         FadeTextPolytopeMap::iterator erase_itr = inner_itr;
//                         // move to next ftpm
//                         ++inner_itr;
                        
//                         fadeTextSet.erase(inner_ftpm._fadeText);

//                         // need to remove inner_ftpm as its occluded.
//                         fadeTextPolytopeMap.erase(erase_itr);
                        
//                     }
//                     else
//                     {
//                         // move to next ftpm
//                         ++inner_itr;
//                     }
//                 }

//                 ++outer_itr;

//             }
// #endif
//         }
//     }
    
//     inline void updateIfRequired(unsigned int frameNumber)
//     {
//         if (_frameNumber!=frameNumber) update(frameNumber);
//     }

//     unsigned int _frameNumber;
//     OpenThreads::Mutex _mutex;
//     ViewUserDataMap _viewMap;
//     ViewFadeTextMap _viewFadeTextMap;
// };

// ossimPlanetGlobalFadeText* getGlobalFadeText()
// {
//     static osg::ref_ptr<ossimPlanetGlobalFadeText> s_globalFadeText = new ossimPlanetGlobalFadeText;
//     return s_globalFadeText.get();
// }
struct ossimPlanetFadeText::FadeTextCullCallback : public osg::Drawable::CullCallback
{
public:
   virtual bool cull(osg::NodeVisitor* nv,
                     osg::Drawable* drawable,
                     osg::RenderInfo* renderInfo) const
      {
         ossimPlanetFadeText* fadeText = dynamic_cast<ossimPlanetFadeText*>(drawable);
         if(!fadeText) return false;
         fadeText->setVisibleFlag(true);
         if(fadeText->theClusterCull.get())
         {
            if(fadeText->theClusterCull->cull(nv, drawable, renderInfo->getState()))
            {
               fadeText->setVisibleFlag(false);
            }
         }
         osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
         if(cv&&fadeText->visibleFlag())
         {
            const osg::Polytope& p = cv->getCurrentCullingSet().getFrustum();
            if(!p.contains(fadeText->getPosition()))
            {
               fadeText->setVisibleFlag(false);
            }
         }
         
         return !fadeText->theVisibleFlag;
      }
};

struct ossimPlanetFadeText::FadeTextUpdateCallback : public osg::Drawable::UpdateCallback
{
//     ossimPlanetFadeTextData _ftd;

    virtual void update(osg::NodeVisitor* nv, osg::Drawable* drawable)
    {
       
       ossimPlanetFadeText* fadeText = dynamic_cast<ossimPlanetFadeText*>(drawable);
       if (!fadeText) return;

       
       unsigned int frameNumber = nv->getFrameStamp()->getFrameNumber();
       if(fadeText->theFrameNumber!=frameNumber)
       {
          fadeText->theFrameNumber = frameNumber;
          
          if(!fadeText->theVisibleFlag)
          {
             if( fadeText->theCurrentOpacity != 0.0)
             {
                fadeText->theCurrentOpacity -= fadeText->theFadeSpeed;
                if(fadeText->theCurrentOpacity < 0.0) 
                {
                   fadeText->theCurrentOpacity = 0.0;
                }
            }
          }
          else if(fadeText->theCurrentOpacity != 1.0)
          {
             fadeText->theCurrentOpacity += fadeText->theFadeSpeed;
             if(fadeText->theCurrentOpacity > 1.0) fadeText->theCurrentOpacity = 1.0;
          }
       }
//         ossimPlanetGlobalFadeText* gft = getGlobalFadeText();
//         gft->updateIfRequired(frameNumber);
        
//         ossimPlanetFadeText::ViewBlendColourMap& vbcm = fadeText->getViewBlendColourMap();

//         _ftd._fadeText = fadeText;
        
//         float fadeSpeed = fadeText->getFadeSpeed();

//         ossimPlanetGlobalFadeText::ViewFadeTextMap& vftm = gft->_viewFadeTextMap;
//         for(ossimPlanetGlobalFadeText::ViewFadeTextMap::iterator itr = vftm.begin();
//             itr != vftm.end();
//             ++itr)
//         {
//             osg::View* view = itr->first;
//             ossimPlanetGlobalFadeText::FadeTextSet& fadeTextSet = itr->second;
//             bool visible = fadeTextSet.count(fadeText)!=0;

//             osg::Vec4& tec = vbcm[view];
//             tec[0] = 1.0f;
//             tec[1] = 1.0f;
//             tec[2] = 1.0f;
//             if (visible)
//             {
//                 if (tec[3]<1.0f)
//                 {
//                     tec[3] += fadeSpeed;
//                     if (tec[3]>1.0f) tec[3] = 1.0f;
//                 }

//             }
//             else
//             {
//                 if (tec[3]>0.0f)
//                 {
//                     tec[3] -= fadeSpeed;
//                     if (tec[3]<0.0f) tec[3] = 0.0f;
//                 }
//             }
//         }
    }
};


ossimPlanetFadeText::ossimPlanetFadeText()
{
    init();
}

ossimPlanetFadeText::ossimPlanetFadeText(const Text& text,const osg::CopyOp& copyop):
    Text(text,copyop)
{
    init();
}

void ossimPlanetFadeText::init()
{
   theFadeSpeed = 0.01f;
   setUpdateCallback(new FadeTextUpdateCallback());
   setCullCallback(new FadeTextCullCallback());
   theCurrentOpacity = 0.0f;
   theVisibleFlag = false;
   theFrameNumber = 0;   
}

void ossimPlanetFadeText::drawImplementation(osg::RenderInfo& renderInfo) const
{
   if(ossim::almostEqual(theCurrentOpacity, 0.0f)) return;
   
   ossimPlanetFadeText* constCast = const_cast<ossimPlanetFadeText*>(this);
   ossim_float64 preserveColorBackground = constCast->_backdropColor[3];
   constCast->_backdropColor[3] = theCurrentOpacity*preserveColorBackground;
   constCast->_color[3] = theCurrentOpacity*preserveColorBackground;
   Text::drawImplementation(renderInfo);
   constCast->_backdropColor[3] = preserveColorBackground;
#if 0
   
   // now pass on new details 
   
   ossimPlanetFadeTextUserData* userData = dynamic_cast<ossimPlanetFadeTextUserData*>(renderInfo.getUserData());
   if (!userData)
   {
      if (renderInfo.getUserData())
      {
         osg::notify(osg::NOTICE)<<"Warning user data not of supported type."<<std::endl;
         return;
      }
      
      userData = getGlobalFadeText()->createNewFadeTextUserData(renderInfo.getView());
      
      if (!userData)
      {
         osg::notify(osg::NOTICE)<<"Memory error, unable to create FadeTextUserData."<<std::endl;
         return;
      }
      
      renderInfo.setUserData(userData);
   }
   
   unsigned int frameNumber = renderInfo.getState()->getFrameStamp()->getFrameNumber();
   if (frameNumber != userData->_frameNumber)
   {
      // new frame so must reset UserData structure.
      userData->_frameNumber = frameNumber;
      userData->_fadeTextInView.clear();
   }
   
   
   
   osgText::Text::AutoTransformCache& atc = _autoTransformCache[renderInfo.getContextID()];
   
   osg::Matrix lmv = atc._matrix;
   lmv.postMult(state.getModelViewMatrix());
   
   if (renderInfo.getView() && renderInfo.getView()->getCamera())
   {
        // move from camera into the view space.
        lmv.postMult(state.getInitialInverseViewMatrix());
        lmv.postMult(renderInfo.getView()->getCamera()->getViewMatrix());
    }
    
    ossimPlanetFadeTextData ftd(const_cast<ossimPlanetFadeText*>(this));
    
    ftd._vertices[0].set(osg::Vec3d(_textBB.xMin(),_textBB.yMin(),_textBB.zMin())*lmv);
    ftd._vertices[1].set(osg::Vec3d(_textBB.xMax(),_textBB.yMin(),_textBB.zMin())*lmv);
    ftd._vertices[2].set(osg::Vec3d(_textBB.xMax(),_textBB.yMax(),_textBB.zMin())*lmv);
    ftd._vertices[3].set(osg::Vec3d(_textBB.xMin(),_textBB.yMax(),_textBB.zMin())*lmv);

    userData->_fadeTextInView.push_back(ftd);
#endif
}
