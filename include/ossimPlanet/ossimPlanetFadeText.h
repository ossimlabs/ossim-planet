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

#ifndef ossimPlanetFadeText_HEADER
#define ossimPlanetFadeText_HEADER 1
#include <ossimPlanet/ossimPlanetExport.h>
#include <osgText/Text>
#include <ossim/base/ossimConstants.h>
#include <osg/ClusterCullingCallback>

class OSSIMPLANET_DLL ossimPlanetFadeText : public osgText::Text
{
public:
   ossimPlanetFadeText();
   ossimPlanetFadeText(const Text& text,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
   
   META_Object(osgText, ossimPlanetFadeText)
      
      
   /**
    * Set the speed that the alpha value changes as the text is occluded or becomes visible.
    *
    */
   void setFadeSpeed(float speed) { theFadeSpeed = speed; }
   float fadeSpeed()const{return theFadeSpeed;}
   void setVisibleFlag(bool visible){theVisibleFlag = visible;}
   bool visibleFlag()const{return theVisibleFlag;}
 	float opacity()const{return theCurrentOpacity;}
	void setOpacity(float value){theCurrentOpacity = value;}
	void setClusterCullingCallback(osg::ref_ptr<osg::ClusterCullingCallback> callback)
      {
         theClusterCull = callback;
      }
   osg::ref_ptr<osg::ClusterCullingCallback> clusterCullingCallback()
      {
         return theClusterCull;
      }
   const osg::ref_ptr<osg::ClusterCullingCallback> clusterCullingCallback()const
      {
         return theClusterCull;
      }
   /** Draw the text.*/
   virtual void drawImplementation(osg::RenderInfo& renderInfo) const;

protected:

   virtual ~ossimPlanetFadeText() {}
   
   void init();
   
   struct FadeTextUpdateCallback;
   struct FadeTextCullCallback;
   friend struct FadeTextUpdateCallback;
   friend struct FadeTextCullCallback;
   
   float theFadeSpeed;
   float theCurrentOpacity;
   bool  theVisibleFlag;
   ossim_uint32 theFrameNumber;
   osg::ref_ptr<osg::ClusterCullingCallback> theClusterCull;
   
   osg::Vec4d theForegroundColor;
   osg::Vec4d theBackgroundColor;
   //    mutable ViewBlendColourMap _viewBlendColourMap;
};

#endif
