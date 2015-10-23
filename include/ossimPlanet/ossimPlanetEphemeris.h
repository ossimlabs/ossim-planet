#ifndef ossimPlanetEphemeris_HEADER
#define ossimPlanetEphemeris_HEADER
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/Material>

class ossimPlanet;
class ossimPlanetCloudLayer;
class OSSIMPLANET_DLL ossimPlanetEphemeris : public ossimPlanetLayer
{
public:
   /**
    * These are setting that will be used for display purposes.  The members
    * can be ored together to formula the current active set of members to use.
    *
    * SUN_LIGHT enables sun lighting calculations. It will automatically set the sun directional
    *           vector and positioning relative to the current ossimPlanet geospatial model.
    * MOON_LIGHT enables moon lighting calculations. It will automatically set the moon directional
    *           vector and positioning relative to the current ossimPlanet geospatial model.
    */
   enum Members
   {
      NO_MEMBERS    = 0,
      SUN_LIGHT     = 1,  // Enable the default Sun lighting
      MOON_LIGHT    = 2,  // Enable the default moon lighting
      AMBIENT_LIGHT = 4,
      SUN           = 8,      
      MOON          = 16,   
      SKY           = 32,   
      FOG           = 64,
      ALL_MEMBERS   = SKY|SUN_LIGHT|MOON_LIGHT|SUN|MOON|AMBIENT_LIGHT|FOG
   };
   
   enum FogMode
   {
      LINEAR = 0, ///<Linear fog
      EXP,        ///<Exponential fog
      EXP2,       ///<Exponential squared fog
      //ADV         ///<Advanced light scattering "fog" RESERVED FOR FUTURE
   };
   
   typedef std::map<double, double> IntensityTableType; 
   class LightingCallback : public osg::Referenced
   {
   public:
      virtual void operator()(ossimPlanetEphemeris* /*ephemeris*/,
                              osg::LightSource* /*light*/)
      {
      }
   };
   ossimPlanetEphemeris(ossim_uint64 membersBitMap=NO_MEMBERS);
   virtual ~ossimPlanetEphemeris();
   virtual void traverse(osg::NodeVisitor& nv);

   osg::Vec3d eyePositionXyz()const;
   osg::Vec3d eyePositionLatLonHeight()const;
   /**
    * @brief Allows you to set what the current active members are.  See the 
    *        enumeration Members for more detail.  This is an "ored" bit vector
    *        of Members to use.
    */
   void setMembers(ossim_uint64 membersBitMap);
   
   /**
    * @return The members bitmap describing the current members active in the
    *         ephemeris model.  See the enumeration Members for more details
    */
   ossim_uint64 members()const;
   
   /**
    * @param statest The root graph to use for light source settings.
    */
   void setRoot(osg::Group* group);
   
   
   /**
    * @brief Sets the index to use for the moonlight source.
    *
    * @param idxNumber The openGL Light index to use for the moon lighting
    */
   void setMoonLightIndex(ossim_uint32 idxNumber);
   
   /**
    * @return the current moonlight index
    */
   ossim_uint32 moonLightIndex()const;
   
   void setMoonLightCallback(LightingCallback* callback);
   
   void setMoonCullCallback(osg::NodeCallback* callback);

   osg::Vec3d moonPositionXyz()const;
   osg::Vec3d moonPositionLatLonHeight()const;
   
   /**
    * @brief Sets the index to use for the sunlight source.
    *
    * @param idxNumber The openGL Light index to use for the sun lighting
    */
   void setSunLightIndex(ossim_uint32 idxNumber);
   
   /**
    * @return the current moonlight index
    */
   ossim_uint32 sunLightIndex()const;
   
   void setSunLightCallback(LightingCallback* callback);
   
   /**
    * @brief Sets the cull call back to use for the sun model
    *
    * @param callback The call back function to use
    */
   void setSunCullCallback(osg::NodeCallback* callback);

   osg::Vec3d sunPositionXyz()const;
   osg::Vec3d sunPositionLatLonHeight()const;
   /**
    * Allows one to turn off auto update for sun color calculations. 
    */
   void setAutoUpdateSunColorFlag(bool flag);
   
   /**
    * @brief OpenGL light index to use for the ambient lighting.
    *
    * @param idxNumber The openGL Light index to use for the sun lighting
    */
   void setGlobalAmbientLightIndex(ossim_uint32 idx);
   
   /**
    * @brief returns the current ambient index.
    *
    * @return the ambient index.
    */
   ossim_uint32 globalAmbientLightIndex()const;
   
   /**
    * Allows one to change the color value for the global ambient lighting.  The
    * color is a normalized r,g,b value and range from 0..1
    *
    * @param ambient The normalized color vector ranging from 0..1 for each RGB component.
    */
   void setGlobalAmbientLight(const osg::Vec3d& ambient);
   /**
    * This will set the date to use.  If this is set then the 
    * auto update of the current time is disabled and will use
    * this date as a fixed shading.  When using this method we basically assume
    * that you are driving the date and time.  To reset back to auto update just call
    * the setAutoUpdateToCurrentTimeFlag.
    *
    * Note, for Ephermis calculations we normalize the date on the fly to GMT time
    * so we will always hold onto the original date passed.
    *
    * @param time an ossimLocalTm object.
    */
   void setDate(const ossimLocalTm& date);
   
   /**
    * This is to allow you to let the ephemeris updates continually 
    * change based on the current time
    */
   void setAutoUpdateToCurrentTimeFlag(bool flag);
   
   /**
    * This will disable the Auto update to current time setting and do a relative
    * displacement of the current date settings to the sim time passed in the osg::NodeVisitor
    *
    * Currently the relative simulation time displacement units must be in seconds.
    */
   void setApplySimulationTimeOffsetFlag(bool flag);
   
   /**
    * This is used by the ossimPlanetViewer to setup a slave camera for the drawing
    * of the sun moon and dome.  We were having problems with large displacements being in 
    * the same graph as the terrain.  The near and far planes were causing the terrain to
    * get clipped at low altitudes.
    *
    * This might go away in the future, but for now I am playing with coupling to the viewer's master
    * camera.
    */
   void setCamera(osg::Camera* camera);
   
   /**
    * Will use the visibility to set the far plane and the density values of the fog.  The visibility is
    * in meters.
    *
    * @param visibility Value specifies visibility in meter distance.
    */
   void setVisibility(ossim_float64 visibility);
   
   
   /**
    * This returns the internal representation of the visibility.
    */
   ossim_float64 visibility()const;
   
   /**
    * Allows one to pass a texture in by file.  It uses this texture to show the position
    * of the sun.
    *
    * @param texture Filename for the texture.  Typically a file format supported by OSG and has
    *        an alpha channel.  Best is an RBA in png format.
    */
   void setSunTextureFromFile(const ossimFilename& texture);

   /**
    * Allows one to pass a texture by osg::Image.  It uses this texture to show the position
    * of the sun.
    *
    * @param texture Image for the texture.  Should be an RGBA type image.
    */
   void setSunTextureFromImage(osg::Image* texture);

   /**
    * @brief Sets the scale to use for the sun model
    *
    * @param scale, The scale to use
    */
   void setSunScale(const osg::Vec3d& scale);

   /**
    * Allows one to pass a texture in by file.  It uses this texture to show the position
    * of the moon.
    *
    * @param texture Filename for the texture.  Typically a file format supported by OSG and has
    *        an alpha channel.  Best is an RBA in png format.
    */
   void setMoonTextureFromFile(const ossimFilename& texture);

   /**
    * Allows one to pass a texture by osg::Image.  It uses this texture to show the position
    * of the moon.
    *
    * @param texture Image for the texture.  Should be an RGBA type image.
    */
   void setMoonTextureFromImage(osg::Image* texture);
   
   void setMoonScale(const osg::Vec3d& scale);
   
   void setSunMinMaxPixelSize(ossim_uint32 minPixelSize,
                              ossim_uint32 maxPixelSize);

   void setMaximumAltitudeToShowDomeInMeters(ossim_float64 maxAltitude);
   void setMaximumAltitudeToShowFogInMeters(ossim_float64 maxAltitude);
 
   /**
    * This specifies the max altitude that you would like Sunlight adjustments to occur for
    * sunrise and sunset calculations.
    *
    * @param maxAltitude The max altitude is specified in meters.
    */
   void setMaxAltitudeToDoSunriseSunsetColorAdjustment(ossim_float64 maxAltitude,
                                                      bool useFading=true);
   
   /**
    * @return the reference value used to identify when to use the eye's altitude to adjust the sun color
    *          used for shading.
    */
   ossim_float64  maxAltitudeToDoSunriseSunsetColorAdjustment()const;
   
   /**
    * This is an intensity/brightness factor multiplied by the sky color based on sun elevation angle
    * with respect to the eye.  The table is a pair of values where the first value is the elevation angle
    * and the second value is the intensity.  This table is used to do a bilinear interpolation of the brightness
    * factor applied to the current sky color.
    *
    * skyColor*brightness
    *
    * where brightness is calculated form the passed in table.
    *
    * @param table a std::map of values that takes pairs elevation angle in degrees and brightness factor.
    */
   void setSkyColorAdjustmentTable(IntensityTableType& table);
   
   /**
    * @return the adjustment table used to modify the sky color.  This is a table of elevation angles in degrees and
    *         brightness factors used to attenuate the skycolor.
    */
   const IntensityTableType* skyColorAdjustmentTable()const;
   
   void setBaseSkyColor(const osg::Vec3d& color);
   osg::Vec3d getBaseSkyColor()const;
   
   void setBaseFogColor(const osg::Vec3d& color);
   osg::Vec3d getBaseFogColor()const;
   
   /**
    *
    * This mirrors the GL fog.  You can currently have LINEAR, EXP, and EXP2 fog affects.  @see Open gl for further information on
    * the GL fog paramters.  LINEAR uses the near and far to set fog attenuation the others attenuate based on distance.
    *
    */
   void setFogMode(FogMode mode);
   
   /**
    * Sets the near plane where the fog begins.
    *
    * @param value the near plane in meters.
    */
   void setFogNear(ossim_float64 value);
   
   /**
    * Set the far plane where the fog ends.  This is automatically set based on visibility.
    *
    * @param value The far plane value in meters.
    */
   void setFogFar(ossim_float64 value);
   
   /**
    * Density is automatically calculated based on visibility.
    *
    * @param value The density value to use. This should e set after the visibility is set.
    */
   void setFogDensity(ossim_float64 value);
   
   /**
    * Allows one to enable and disable fog in the scene.
    *
    * @param flag enables and disables the fog.  Pass in true to enable and false
    *        to disable.
    */
   void setFogEnableFlag(bool flag);
   
   /**
    * Resizes the list to the specified number of cloud layers.
    *
    * @param numberOfLayers The number of cloud layers to resize the list to.
    */
   void setNumberOfCloudLayers(ossim_uint32 numberOfLayers);
   
   void removeClouds(ossim_uint32 idx, ossim_uint32 count=1);
   /**
    * @param idx The index of the cloud layer to return.
    * @return The cloud layer specified at idx.  If idx is out of
    *         range then a value of null or 0 is returned.
    */
   ossimPlanetCloudLayer* cloudLayer(ossim_uint32 idx);
   
   /**
    * @return the number of cloud layers
    */
   ossim_uint32 numberOfCloudLayers()const;
   
   /**
    * This is a utility method that allows one to create a patch of clouds centered
    * at the give lat lon height.  In short, we shift the creation of the patch to lat lon height
    * to 0,0,0 so the patch is square and then use a Matrix transform to move it to a new location defined
    * by the center lat lon height. The seed, coverange and sharpness are exposed to give you some
    * control over the texture generated. The seed is used to initialize the random number generator so you should
    * be able to give the same paramters and generate the same texture.  The coverage and sharpness are used to control
    * how much clouds there are and how soft they appear.  For example coverage of say 20 and sharpness of .96 gives a nice
    * look.  If you want them to be more overcast and softer, then increase the coverage to say 150 and the sharpness to around
    * .99.
    *
    * @param cloudLayerIndex The cloud layer to create a patch for.
    * @param theCenterLatLonHeight The center point to place the cloud patch.
    * @param numberOfMeshSamples  This is square so a value of 128 will
    *        create a mesh sample of 128x128.
    * @param patchSizeInDegrees  Defines the patch size in depgrees.
    * @param seed This is the seed value used to generate a cloud texture.
    * @param coverage This is used to generate a texture.  This value is not a percentage
    *         and is a value to controll the way the clouds look.
    * @param sharpness  The closer the value is to 1.0 the softer the clouds and the closer you go to 
    *        0 the harder the clouds look.
    */
   void createCloudPatch(ossim_uint32 cloudLayerIndex,
                         const osg::Vec3d& theCenterLatLonHeight,
                         ossim_float64 numberOfMeshSamples,
                         ossim_float64 patchSizeInDegrees,
                         ossim_uint64  seed,
                         ossim_float64 coverage,
                         ossim_float64 sharpness);
   
   void createGlobalCloud(ossim_uint32 cloudLayerIndex,
                          ossim_float64 altitude,
                          ossim_float64 numberOfMeshSamples,
                          ossim_uint64  seed,
                          ossim_float64 coverage,
                          ossim_float64 sharpness);
   
   virtual osg::BoundingSphere computeBound() const;
   
protected:
   class EphemerisData;
   /**
    * This is used to hide the gpstk library we are using.  This way we can conditionally compile
    * out.
    */
   EphemerisData* theEphemerisData;
};

#endif
