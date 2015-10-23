#ifndef ossimPlanetViewApi_HEADER
#define ossimPlanetViewApi_HEADER
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimConstants.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
   /**
    * ossimPlanet_StatePtr is the main pointer.  All API calls will manpulate this state pointer.  
    * It will hold the planet layers, views, manipulators, ... etc. ossimPlanet_PlanetPtr is the native pointer
    * to a Planet Object.  The ossimPlanet_LayerPtr is the native pointer to a Planet layer.     * 
    */
   typedef void*         ossimPlanet_StatePtr;
   typedef void*         ossimPlanet_PlanetPtr;
   typedef void*         ossimPlanet_LayerPtr;
   typedef ossim_int64   ossimPlanet_IndexType;
   typedef ossim_uint64  ossimPlanet_SizeType;
   typedef const char*   ossimPlanet_ConstStringType;
   typedef char*         ossimPlanet_StringType;
   
   /**
    * The context type determines how much you want the API to manage and handle.  There
    * are 3 ways you probably would like to use the library.  
    * 
    * - ossimPlanet_VIEWER_CONTEXT option is typical of a full command line application that all 
    *   you are concerned about is scripting and running a full screen or windowed planet.  This
    *   will interface into osg::Viewer.  
    * - ossimPlanet_NOVIEWER_CONTEXT option is used to embed  the scenegraph and bridge view parameters.
    *   There are 2 situations when you want to do this and both require that you create your own GL context and manage it.
    *   The first situation is if Planet is the drawable and you are in a GUI environment that creates a Gl context for you.
    *   you just make it current and then draw the planet and use the API to bridge view parameters and manipulate layers. 
    *   The second situation is if you have a full GL application that you wrote and all you want to do is embed the planet
    *   as a drawable then use this context.
    * - ossimPlanet_PLANET_ONLY_CONTEXT option is used when you are wanting to use the API only to interface into the manipulation of the planet layers.
    *   Typically this is done if you have your own OpenSceneGraph and your own View and gl context.  We will give you native access to
    *   root planet class.
    */
   enum ossimPlanet_ContextType
   {
      ossimPlanet_NOVIEWER_CONTEXT    = 0,
      ossimPlanet_PLANET_ONLY_CONTEXT = 1,
      ossimPlanet_VIEWER_CONTEXT      = 2
   };
   enum ossimPlanet_BOOL
   {
      ossimPlanet_FALSE = 0,
      ossimPlanet_TRUE  = 1
   };
 
   enum ossimPlanetFragShaderType
   {
      ossimPlanet_NO_SHADER = 0,
      ossimPlanet_TOP,
      ossimPlanet_REFERENCE,
      ossimPlanet_OPACITY,
      ossimPlanet_HORIZONTAL_SWIPE,
      ossimPlanet_VERTICAL_SWIPE,
      ossimPlanet_BOX_SWIPE,
      ossimPlanet_CIRCLE_SWIPE,
      ossimPlanet_ABSOLUTE_DIFFERENCE,
      ossimPlanet_FALSE_COLOR_REPLACEMENT
   };
   
#  define ossimPlanet_INVALID_INDEX ((ossimPlanet_IndexType)-1) 
   /******************************** Setup and Initialization API *****************************/
   
   /**
    * Add the passed in path to OpenSceneGraph's Library path.  This will be used mainly for finding OpenSceneGraph
    * plugins.  If this needs to be setup it should be done before you call ossimPlanet_init and start adding layers
    * to planet.
    *
    * @param path Path to add to the Library search path.
    * @param insertFrontFlag Inserts the path to the front of the search path list if its ossimPlanet_TRUE and
    *                        appends to the end otherwise.
    */
   OSSIMPLANET_DLL void ossimPlanet_addOpenSceneGraphLibraryPath(ossimPlanet_ConstStringType path,
                                                                 ossimPlanet_BOOL insertFrontFlag);
   
   /**
    * Adds the passed in path to OpenSceneGraph's data path.  This allows one to give relative path naming to 
    * data that is loaded.  For example:  if you have a font called arial.ttf and it's located in /fonts then you can add
    * /fonts to the search path and you can load a font by just the name "arial.ttf".  The /fonts will be prepended.  If 
    * this needs to be setup it should be done before you call ossimPlanet_init and start adding layers to planet.
    *
    * @param path Path to add to the Data search path.
    * @param insertFrontFlag Inserts the path to the front of the search path list if its ossimPlanet_TRUE and
    *                        appends to the end otherwise.
    */
   OSSIMPLANET_DLL void ossimPlanet_addOpenSceneGraphDataPath(ossimPlanet_ConstStringType path,
                                                              ossimPlanet_BOOL insertFrontFlag);
   
   /**
    * Loads an ossim preference file.
    *
    * @param preferenceFile The preference file to load.
    */
   OSSIMPLANET_DLL void ossimPlanet_loadOssimPreferenceFile(ossimPlanet_ConstStringType preferenceFile);
   
   /**
    * This is a utility call that allows one to set preference variables directly.  This will allow one to
    * modify preference variables.  If this needs to be setup it should be done before you call ossimPlanet_init 
    * and start adding layers to planet.
    *
    * @param name The name of the preference variable.
    * @param value The value of the preference variable.
    */ 
   OSSIMPLANET_DLL void ossimPlanet_setOssimPreferenceNameValue(ossimPlanet_ConstStringType name,
                                                                ossimPlanet_ConstStringType value);
   
   /**
    * Adds an ossim core plugin.  If this is a directory then all files in the directoy that are plugins are added
    *
    * @param path Path to add to the OSSIM Plugin search path.
    * @param insertFrontFlag Inserts the path to the front of the search path list if its ossimPlanet_TRUE and
    *                        appends to the end otherwise.
    */
   OSSIMPLANET_DLL void ossimPlanet_addOssimPlugin(ossimPlanet_ConstStringType path,
                                                   ossimPlanet_BOOL insertFrontFlag);
   
  
   /**
    * Add elevation to the OSSIM core system.
    *
    * @param path Path to add to the Elevation.
    */
   OSSIMPLANET_DLL void ossimPlanet_addOssimElevation(ossimPlanet_ConstStringType path);
   
   /**
    *
    * Elevation data is typically relative to mean sea level.  The geoid grids will be used to shift the Mean Sea Level
    * to ellipsoidal heights. 
    * 
    * @param path Path to add to the Geoid search path.
    * @param byteOrder.  Most of the goid grids don't have Endian indication.  You can download little endian or big endian type geoid grids
    *        The basic 96 grid that comes with OSSIM egm96.grd is a big endian byte order.  You can downlod from 
    * @param insertFrontFlag Inserts the path to the front of the search path list if its ossimPlanet_TRUE and
    *                        appends to the end otherwise.
    */
   OSSIMPLANET_DLL void ossimPlanet_addGeoid(ossimPlanet_ConstStringType path,
                                             ossimByteOrder byteOrder,
                                            ossimPlanet_BOOL insertFrontFlag);
	
   /**
    * Used to initialize the system.  This is typically called one time.  It initializes internal registries.
    */
   OSSIMPLANET_DLL void ossimPlanet_init();
   
   /**
    * Allows one to init by passing command line arguments.  This is typically used by command line
    * applications.
    *
    * @param argc A pointer to an integer.
    * @param argv Pointer to the argv.
    */
   OSSIMPLANET_DLL void ossimPlanet_initWithArgs(int* argc, char** argv[]);
   
   /**
    * Allows you to set the trace on and off for various classes in the system.  This is in the format of a 
    * regular expression.  Some of the special characters found in the regular expression are:
    * ^        Matches at beginning of a line
    * $        Matches at end of a line
    * .        Matches any single character
    * [ ]      Matches any character(s) inside the brackets
    * [^ ]     Matches any character(s) not inside the brackets
    * -        Matches any character in range on either side of a dash
    * *        Matches preceding pattern zero or more times
    * +        Matches preceding pattern one or more times
    * ?        Matches preceding pattern zero or once only
    *
    * ()       Saves a matched expression and uses it in a  later match
    *
    * If you want to trace all
    * ossim classes then do :
    * 
    * ossimPlanet_setTracePattern("ossim.*");
    *
    * @param pattern A regular expression pattern to describe what to trace.  
    */
   OSSIMPLANET_DLL void ossimPlanet_setTracePattern(ossimPlanet_ConstStringType pattern);
   
   /**
    * This is called to do any cleanup of the library.
    */
   OSSIMPLANET_DLL void ossimPlanet_finalize();
   
   /**
    * Gives access to a microsecond sleep.
    *
    * @param microSeconds Specify how many microseconds to sleep.
    */
   OSSIMPLANET_DLL void ossimPlanet_microSecondSleep(ossimPlanet_SizeType microSeconds);
   
   /**
    * Gives access to a millisecond sleep.
    *
    * @param milliSeconds Specify how many milli seconds to sleep.
    */
   OSSIMPLANET_DLL void ossimPlanet_milliSecondSleep(ossimPlanet_SizeType milliSeconds);

   /**
    * Gives access to a seconds sleep.
    *
    * @param seconds Specify how many seconds to sleep.
    */
   OSSIMPLANET_DLL void ossimPlanet_secondSleep(ossimPlanet_SizeType seconds);

   /********************** DO STATE INTERFACE ***********************/
   
   /**
    * This will return an internal class that contains all the needed 
    * state information.  It will basically create a self contained manager for
    * view, layers, manipulator, ... etc.
    *
    * Note: Currently there can be only 1 view assigned per state. Once the state is called you can create view, layers 
    *       add manipulator, ...etc for the given state.
    *
    * @param contextType This specifies the type of context you would like to create.  @see ossimPlanet_contextType.
    *
    * @return State handle;
    */
   OSSIMPLANET_DLL ossimPlanet_StatePtr ossimPlanet_newState(ossimPlanet_ContextType type);
       
   /**
    * ossimPlanet_deleteState()
    *
    * Will delete the passed in state.  
    *
    * @param state This is the state to delete.
    */
   OSSIMPLANET_DLL void ossimPlanet_deleteState(ossimPlanet_StatePtr state);
   
   
   /********************* ACCESS TO NATIVE POINTERS *********************/
   
   /**
    * This will be mainly used in native C++ that wants to add callbacks to get direct feedback from 
    * various actions that occur within planet.  People should be able to do reinterpret_cast<ossimPlanet*> on
    * the native pointer returned.
    *
    * @return Returns the address of the ossimPlanet object.  
    */
   OSSIMPLANET_DLL ossimPlanet_PlanetPtr ossimPlanet_getNativePlanetPointer(ossimPlanet_StatePtr state);
   
   /************************************ API FOR THE VIEW ********************************/
  
   /**
    *  Allows one to disable the texture shaders.
    */
   OSSIMPLANET_DLL void ossimPlanet_setLandFragShaderType(ossimPlanet_StatePtr state,
                                                          ossimPlanetFragShaderType type);
   
   /**
    *
    * When initially setting up your scene it is not set to the root Scene view or viewer.  This only has affect in ossimPlanet_NOVIEWER_CONTEXT
    * and ossimPlanet_VIEWER_CONTEXT where you have a Scene managers and manipulators.
    *
    * @param state The state to modify.
    */
   OSSIMPLANET_DLL void ossimPlanet_setSceneDataToView(ossimPlanet_StatePtr state);
   
   /**
    *  Only support ossimPlanetManipulator type for now.
    *
    * @param state The state to modify.
    * @param typeName The type of Manipulator to create.  The current one is ossimPlanetManipulator.
    * @param receverPathName The recever path name given to this object.  We can pass setup commands to it.
    */
   OSSIMPLANET_DLL void ossimPlanet_setViewManipulator(ossimPlanet_StatePtr state,
                                                       ossimPlanet_ConstStringType typeName,
                                                       ossimPlanet_ConstStringType receiverPathName);
   /**
    * This will assume that a Gl context is created and that the Viewport is set.  It will use 
    * GL calls to get the current Viewport values and set to the internal structures associated 
    * with the passed in state.  This is used when you are manipulating the Viewport through
    * GL calls.  If this is the case then you can call this before rendering the planet.
    *
    * @param state State to modify.
    */
   OSSIMPLANET_DLL void setViewportToCurrentGlSettings(ossimPlanet_StatePtr state);
   
   
   /**
    * This will assume that a Gl context is created and that the Projection Matrix is set.  It will use 
    * GL calls to get the current Projection Matrix values and set to the internal structures associated 
    * with the passed in state.  This is used when you are manipulating the Projection Matrix through
    * GL calls.  If this is the case then you can call this before rendering the planet.
    *
    * @param state State to modify.
    */
   OSSIMPLANET_DLL void setProjectionMatrixToCurrentGlSettings(ossimPlanet_StatePtr state);
   
   /**
    * This will assume that a Gl context is created and that the ModelView Matrix is set.  It will use 
    * GL calls to get the current ModelView Matrix values and set to the internal structures associated 
    * with the passed in state.  This is used when you are manipulating the ModelView Matrix through
    * GL calls.  If this is the case then you can call this before rendering the planet.
    *
    * @param state State to modify.
    */
   OSSIMPLANET_DLL void setModelViewMatrixToCurrentGlSettings(ossimPlanet_StatePtr state);
   
   
   /***** DO PROJECTION MATRIX INTERFACE *****/
   
   /**
    * This allows one to set the perspective.
    *
    * @param state The state to modify.
    * @param fov Field of view in degrees.
    * @param aspectRatio The aspect ratio of the view.
    * @param near The near plane distance.
    * @param far The far plane distance.
    */
   OSSIMPLANET_DLL void ossimPlanet_setProjectionMatrixAsPerspective(ossimPlanet_StatePtr state,
                                                                     double fov,
                                                                     double aspectRatio,
                                                                     double near, 
                                                                     double far);
   
   /**
    * This is identical to the glFrustum settings.
    *
    * @param state The state to modify.
    * @param left Left value.
    * @param right Right value.
    * @param bottom Bottom value.
    * @param top Top value.
    * @param zNear zNear value.
    * @param zFar zFar value.
    */
   OSSIMPLANET_DLL void ossimPlanet_setProjectionMatrixAsFrustum(ossimPlanet_StatePtr state,
                                                                 double left, double right,
                                                                 double bottom, double top,
                                                                 double zNear, double zFar);
   
   /**
    * This is identical to the glOrtho call and creates and orthographic projection.
    *
    * @param state The state to modify.
    * @param left Left value.
    * @param right Right value.
    * @param bottom Bottom value.
    * @param top Top value.
    * @param zNear zNear value.
    * @param zFar zFar value.
    */
   OSSIMPLANET_DLL void ossimPlanet_setProjectionMatrixAsOrtho(ossimPlanet_StatePtr state,
                                                               double left, double right,
                                                               double bottom, double top,
                                                               double zNear, double zFar);
   
   /** 
    * 
    * Set to a 2D orthographic projection. See OpenGL glOrtho2D documentation for further details.
    * @param state The state to modify.
    * @param left Left value.
    * @param right Right value.
    * @param bottom Bottom value.
    * @param top Top value.
    */
   OSSIMPLANET_DLL void ossimPlanet_setProjectionMatrixAsOrtho2D(ossimPlanet_StatePtr state,
                                                                 double left, double right,
                                                                 double bottom, double top);
   /**
    * This assumes a 4x4 array or 16 consecutive values.  The values are stored row ordered.  
    * This means that the first 4 values are for the first row and the second four values are
    * for the second row, ... etc.
    *
    * @param m The array of 16 values
    */
   OSSIMPLANET_DLL void ossimPlanet_setProjectionMatrixAsRowOrderedArray(ossimPlanet_StatePtr state,
                                                                         double* m);
   
   /**
    * This will set the projection matrix to the passed in values.  Note m00 - m03 identifies the
    * first row of the matrix, then the next is the second row, ... etc.
    *
    * @param state The state to modify the view orientation matrix.
    * @param m00 row 0 col 0
    * @param m01 row 0 col 1
    * @param m02 row 0 col 2
    * @param m03 row 0 col 3
    * @param m10 row 1 col 0
    * @param m11 row 1 col 1
    * @param m12 row 1 col 2
    * @param m13 row 1 col 3
    * @param m20 row 2 col 0
    * @param m21 row 2 col 1
    * @param m22 row 2 col 2
    * @param m23 row 2 col 3
    * @param m30 row 3 col 0
    * @param m31 row 3 col 1
    * @param m32 row 3 col 2
    * @param m33 row 3 col 3
    *
    */
   OSSIMPLANET_DLL void ossimPlanet_setProjectionMatrix(ossimPlanet_StatePtr state,
                                                        double m00, double m01, double m02, double m03,
                                                        double m10, double m11, double m12, double m13,
                                                        double m20, double m21, double m22, double m23,
                                                        double m30, double m31, double m32, double m33);
   /****** NOW EXPOSE ORIENTATION MATRIX DEFINITIONS ****/
   
   /**
    * This allows one to pass in the lat, lon height and the euler angles to define the view matrix.
    * The Euler angles are relative to the tangent plane at the given lat lon position.  Note this is
    * for the camera and the Z-Axis is the plumb/Nadir axis.  This means that a heading, pitch and roll of all
    * 0 degrees will have the eye looking straight down.
    *
    * Note: all lat lon values are assumed to be relative to the WGS84 ellipsoid.  
    *
    * @param state The state to modify.
    * @param lat The latitude of the view camera in degrees.
    * @param lon The longitude of the view camera in degrees.
    * @param height The height of the view camera in meters relative to the WGS84 ellipsoid.
    * @param heading The heading of the view camera in degrees.
    * @param pitch  The pitch of the view camera in degrees.
    * @param roll  The roll of the view camera in degrees.
    */
   OSSIMPLANET_DLL void ossimPlanet_setViewMatrixAsLlhHprRelativeTangent(ossimPlanet_StatePtr state,
                                                                         double lat,
                                                                         double lon,
                                                                         double height,
                                                                         double heading,
                                                                         double pitch,
                                                                         double roll);
   
   /**
    * This allows one to pass in the lat, lon height and the euler angles to define the view matrix.
    * The Euler angles are not relative to the tangent plane at the given lat lon position but instead the unit axis is used.  
    * for the camera and the Z-Axis is the plumb/Nadir axis.  
    *
    * Note: all lat lon values are assumed to be relative to the WGS84 ellipsoid.  
    *
    * @param state The state to modify.
    * @param lat The latitude of the view camera in degrees.
    * @param lon The longitude of the view camera in degrees.
    * @param height The height of the view camera in meters relative to the WGS84 ellipsoid.
    * @param heading The heading of the view camera in degrees.
    * @param pitch  The pitch of the view camera in degrees.
    * @param roll  The roll of the view camera in degrees.
    */
   OSSIMPLANET_DLL void ossimPlanet_setViewMatrixAsLlhHprAbsolute(ossimPlanet_StatePtr state,
                                                                  double lat,
                                                                  double lon,
                                                                  double height,
                                                                  double heading,
                                                                  double pitch,
                                                                  double roll);
   
   /**
    * This Will set the matrix as a row ordered array.
    *
    * @param state The state to modify the view orientation matrix.
    * @param m Row ordered view matrix.
    *
    */
   OSSIMPLANET_DLL void ossimPlanet_setViewMatrixAsRowOrderedArray(ossimPlanet_StatePtr state,
                                                                   const double *m);
   
   /**
    * This will set the matrix to the passed in values.  Note m00 - m03 identifies the
    * first row of the matrix, then the next is the second row, ... etc.
    *
    * @param state The state to modify the view orientation matrix.
    * @param m00 row 0 col 0
    * @param m01 row 0 col 1
    * @param m02 row 0 col 2
    * @param m03 row 0 col 3
    * @param m10 row 1 col 0
    * @param m11 row 1 col 1
    * @param m12 row 1 col 2
    * @param m13 row 1 col 3
    * @param m20 row 2 col 0
    * @param m21 row 2 col 1
    * @param m22 row 2 col 2
    * @param m23 row 2 col 3
    * @param m30 row 3 col 0
    * @param m31 row 3 col 1
    * @param m32 row 3 col 2
    * @param m33 row 3 col 3
    *
    */
   OSSIMPLANET_DLL void ossimPlanet_setViewMatrix(ossimPlanet_StatePtr state,
                                                  double m00, double m01, double m02, double m03,
                                                  double m10, double m11, double m12, double m13,
                                                  double m20, double m21, double m22, double m23,
                                                  double m30, double m31, double m32, double m33);
   
   /*** NOW EXPOSE VIEWPORT DEFINITIONS *******/
   
   /**
    * ossimPlanet_setViewport
    *
    * Sets the viewport of the for the view of the passed in state.
    *
    * @param state  The state to modify the viewport definitions.
    * @param x The x location in pixels of the viewport.  Typically 0
    * @param y The y location in pixels of the viewport.  Typically 0
    * @param w The width in pixels of the viewport.
    * @param h The height in pixels of the viewport.
    *
    */
   OSSIMPLANET_DLL void ossimPlanet_setViewport(ossimPlanet_StatePtr state,
                                                int x, 
                                                int y, 
                                                int w, 
                                                int h);
   
   /**
    * Sets the color used to clear the viewport area.
    *
    * @param state  The state information to modify the clear color.
    * @param red The red component of the clear color.
    * @param green The green component of the clear color.
    * @param blue The blue component of the clear color.
    * @param alpha The alpha component of the clear color.
    */
   OSSIMPLANET_DLL void ossimPlanet_setViewportClearColor(ossimPlanet_StatePtr state,
                                                          float red,
                                                          float green,
                                                          float blue,
                                                          float alpha);
   
   
   /**
    * Save the gl attributes.  This is mainly only used if you are embedding in
    * a non Open scenegraph application and you need the state attributes to be preserved
    * after calling the ossimPlanet_frame.
    *
    * This will preserve the Matrix stacks (MODEL, TEXTURE, PROJECTION) and the
    * GL_ALL_ATTRIB_BITS.  After the call to frame then you should call
    * ossimPlanet_popState();
    */
   OSSIMPLANET_DLL void ossimPlanet_pushState();
   
   /**
    * @see ossimPlanet_pushState();
    */
   OSSIMPLANET_DLL void ossimPlanet_popState();
   
   OSSIMPLANET_DLL void ossimPlanet_setStateReceiverPathName(ossimPlanet_StatePtr state,
															 ossimPlanet_ConstStringType path);
   OSSIMPLANET_DLL void ossimPlanet_setPlanetReceiverPathName(ossimPlanet_StatePtr state,
															  ossimPlanet_ConstStringType path);
   
   
   /*************************** General Layer interfaces *********************/
   
   /**
    * This adds a layer to planet. the layer type is used in the ossimPlanetLayerRegistry 
    * to construct a layer of the passed layerType.  The receiverName is used to set the receiver
    * name of the layer created so action messages can be routed to it.
    *
    * @param state The state to modify.
    * @param layerType Layer type name to add.  This is used by the layer registry to construct a layer
    *                  specified by the layerType.
    * @param name This is the name you wish to give the layer.
    * @param id This is the id for the layer.
    * @param description This is the description for the layer.
    * @param receiverPathName Allows one to create a receiver name for the layer.  This is the target name for
    *                         Action messages(@see ossimPlanet_routeAction).
    *
    * @return The natve pointer to the layer added.
    */
   OSSIMPLANET_DLL ossimPlanet_LayerPtr ossimPlanet_addLayer(ossimPlanet_StatePtr state,
                                                             ossimPlanet_ConstStringType layerType,
                                                             ossimPlanet_ConstStringType name,
                                                             ossimPlanet_ConstStringType id,
                                                             ossimPlanet_ConstStringType description,
                                                             ossimPlanet_ConstStringType receiverPathName);
   
   OSSIMPLANET_DLL void ossimPlanet_removeLayerGivenPtr(ossimPlanet_StatePtr state,
                                                        ossimPlanet_LayerPtr layerPtr);
   /**
    * @param state The state to use.
    *
    * @return Returns the number of layers defined in planet for the give state.
    */
   OSSIMPLANET_DLL ossimPlanet_SizeType ossimPlanet_getNumberOfLayers(ossimPlanet_StatePtr state);

   /**
    * @param state The state to use.
    * @param layerPtr The layer pointer to use.
    *
    * @return Returns the index of the layer given it's pointer.
    */
   OSSIMPLANET_DLL ossimPlanet_IndexType ossimPlanet_getIndexOfLayerGivenPtr(ossimPlanet_StatePtr state,
                                                                             ossimPlanet_LayerPtr layerPtr);
   
   /**
    * @param state The state to use.
    * @param idx The index number of the layer to return.
    *
    * @return Returns the layer pointer given an index.  NULL or 0 is returned if idx is out of range.
    */
   OSSIMPLANET_DLL ossimPlanet_LayerPtr ossimPlanet_getLayerGivenIndex(ossimPlanet_StatePtr state,
                                                                       ossimPlanet_IndexType idx);
   
   /**
    * @param state The state to use.
    * @param idThe id of the layer to search.
    *
    * @return Returns the layer pointer identified by id.
    */
   OSSIMPLANET_DLL ossimPlanet_LayerPtr ossimPlanet_getLayerGivenId(ossimPlanet_StatePtr state,
                                                                    ossimPlanet_ConstStringType id);
   /**
    * @param state The state to use.
    * @param idx The index of the layer to access.
    *
    * @return The layer name for the layer at the specified index.
    */
   OSSIMPLANET_DLL ossimPlanet_ConstStringType ossimPlanet_getLayerName(ossimPlanet_LayerPtr layer);
   
   /**
    * @param layer The layer to set.
    * @param name The name to set the layer to.
    */
   OSSIMPLANET_DLL void ossimPlanet_setLayerName(ossimPlanet_LayerPtr layer,
                                                 ossimPlanet_ConstStringType name);   
   
   /**
    * @param state The state to use.
    * @param idx The index of the layer to access.
    *
    * @return The layer name for the layer at the specified index.
    */
   OSSIMPLANET_DLL ossimPlanet_ConstStringType ossimPlanet_getLayerId(ossimPlanet_LayerPtr layer);
      
   /**
    * @param layer The layer to set.
    * @param id The id to set the layer to.
    */
   OSSIMPLANET_DLL void ossimPlanet_setLayerId(ossimPlanet_LayerPtr layer,
                                               ossimPlanet_ConstStringType id);

   /**
    * @param state The state to use.
    * @param idx The index of the layer to access.
    *
    * @return The layer description for the layer at the specified index.
    */
   OSSIMPLANET_DLL ossimPlanet_ConstStringType ossimPlanet_getLayerDescription(ossimPlanet_LayerPtr layer);

   /**
    * @param layer The layer to set.
    * @param description The description to set the layer to.
    */
   OSSIMPLANET_DLL void ossimPlanet_setLayerDescription(ossimPlanet_LayerPtr layer,
                                                        ossimPlanet_ConstStringType description);

   /**
    * @param state The state to use.
    * @param idx The index of the layer to access.
    *
    * @return The recever path name for the layer at the specified index.
    */
   OSSIMPLANET_DLL ossimPlanet_ConstStringType ossimPlanet_getLayerReceverPathName(ossimPlanet_LayerPtr layer);
      
   /**
    * @param layer The layer to set.
    * @param receiverName The description to set the layer to.
    */
   OSSIMPLANET_DLL void ossimPlanet_setLayerReceiverPathName(ossimPlanet_LayerPtr layer,
                                                             ossimPlanet_ConstStringType receiverPathName);
   
   
   /**
    * @param state The state to use.
    * @param idx The index of the layer to access.
    *
    * @return The layer enable flag for the layer at the specified index.
    */
   OSSIMPLANET_DLL ossimPlanet_BOOL ossimPlanet_getLayerEnableFlag(ossimPlanet_LayerPtr layer);
      
   /**
    *
    * @param layer The layer to set.
    * @param flag This specifies if the layer is enabled or not.  A value of ossimPlanet_TRUE will
    *             enable the layer and a value of ossimPlanet_FALSE will disable the layer.
    */
   OSSIMPLANET_DLL void ossimPlanet_setLayerEnableFlag(ossimPlanet_LayerPtr layer,
                                                       ossimPlanet_BOOL flag);

	
	/**
	 *
	 * Adds an image to the reference layer. Note:  location is currently for local data and cann be a directory or
	 * a file.  We currently do not recurse directories so if the passed in file is a directory no subdirectories will be scanned.
	 *
	 */
	OSSIMPLANET_DLL void ossimPlanet_addImageToReceiver(ossimPlanet_ConstStringType location,
																		 ossimPlanet_ConstStringType receverName,
																		 ossimPlanet_BOOL addInTheBackgroundFlag);
   
   /**
    * This is for legacy KWL support and is only for texture layers.
    */
   OSSIMPLANET_DLL void ossimPlanet_addTextureLayersFromKwlFile(ossimPlanet_StatePtr state,
                                                                const char* kwlFile);
   
   /******************************* Action Routing Interface****************************/
   /**
    * This will use ossimPlanet's Action router to route the message to the destination.  For example: lets say
    * we have a receiver name as foo and we have an action called Set.
    * 
    * ossimPlanet_routeActionForm1("<Set target=":foo">.....</Set>);
    *
    * This will route a action "Set" to the receiver "foo".
    *
    * @param action
    */
   OSSIMPLANET_DLL void ossimPlanet_executeXmlAction(ossimPlanet_ConstStringType completeAction);
   
   /**
    * Will post the action to a threaded action queue so it executes in the background.
    */ 
   OSSIMPLANET_DLL void ossimPlanet_postXmlAction(ossimPlanet_ConstStringType completeAction);
   
   /**************************** Rendering Interfaces ***********************************/
   
   /**
    * This is the main rendering step.  This will render 1 frame.     
    * @param state The state to modify.
    *
    * @return Returns ossimPlanet_TRUE if the render frame can continue again or
    *                 ossimPlanet_FALSE if it was canceled.  This is typically only important for ossimPlanet_VIEWER type
    *                 context.
    */
   OSSIMPLANET_DLL ossimPlanet_BOOL ossimPlanet_renderFrame(ossimPlanet_StatePtr state);
   
   /**
    * This is the main rendering step.  This will render 1 frame and preserve the state.  This will 
    * call osismPlanet_pushState before rendering and ossimPlanet_popState after rendering and will preserve
    * gl attributes, View, Projection and Texture matrices.
    *
    * @param state The state to modify.
    *
    * @return Returns ossimPlanet_TRUE if the render frame can continue again or
    *                 ossimPlanet_FALSE if it was canceled.  This is typically only important for ossimPlanet_VIEWER type
    *                 context.
    */
   OSSIMPLANET_DLL ossimPlanet_BOOL ossimPlanet_renderFramePreserveState(ossimPlanet_StatePtr state);
   
   /**
    * This is used determine if another frame is required to be rendered.  Call this before
    * you call ossimPlanet_Frame.  
    *
    * <pre>
    *    if(ossimPlanet_needsRendering(state))
    *    {
    *        ossimPlanet_renderFrame(state);
    *    }
    * </pre>
    *
    * @param state The state to modify.
    * @return ossimPlanet_TRUE if needs to continue rendering or ossimPlanet_FALSE if
    *         no more rendering is required.
    */
   OSSIMPLANET_DLL ossimPlanet_BOOL ossimPlanet_needsRendering(ossimPlanet_StatePtr state);
   
	
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // end ossimPlanetViewApi_HEADER
