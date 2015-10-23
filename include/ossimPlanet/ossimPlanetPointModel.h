#ifndef ossimPlanetPointModel_HEADER
#define ossimPlanetPointModel_HEADER
#include <ossimPlanet/ossimPlanetAnnotationLayerNode.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossimPlanet/ossimPlanetLsrSpaceTransform.h>

/**
 * This will be the root class to handle all point based models.  
 * Typically this may pertain to buildings or other 3-D
 * point models.  This will also own an ossimLsrSpaceTransform that 
 * allows one to talk to the Matrix transform in a common Euler orientation
 * and lat lon altitude placement and scale settings.  The Lsr stands for Local Space
 * Reference and mirrors the conepts in the ossim core engine.
 *
 * Caveat:  If the setMatrix is called on a MatrixTransform which is a base of
 *          ossimPlanetLsrSpaceTransform then it will not sync the euler and lat lon paramters
 *          until the next call to computeBound.  Note setMatrix will dirty the bound.  So if you want immediate
 *          sync to occurr then you must manually say getBound immediately after a setMatrix.  This should
 *          force a recompute.
 *
 *
 * <pre>
 *  EXAMPLE USAGE:
 *  We will assume we have an annotation layer defined in ossimPlanetViewer and will access through the Viewer
 *  
 *  osg::ref_ptr<ossimPlanetPointModel> pointModel = new ossimPlanetPointModel;
 *  // adding the object should automatically setup the pointers for model and layer on the Node.
 *  viewer->addAnnotation(pointModel.get());
 *
 *  // node is some local 3-D comdel that you wish to posistion.
 *  pointModel->setNode(node);
 * 
 * // set the point at 25 degrees lat, -120 degrees lon and 5000 meters above the ellipsoid.  Note this call
 * // assumes ellipsoidal height.  Use mean sea level call for heights above the geoidal egm grid.
 *  pointModel->lsrSpace()->setLatLonAltitude(osg::Vec3d(25.0, -120, 5000));
 * 
 *
 * </pre>
 */
class OSSIMPLANET_DLL ossimPlanetPointModel : public ossimPlanetAnnotationLayerNode
{
public:
   class LsrSpaceCallback : public ossimPlanetLsrSpaceTransformCallback
   {
   public:
      LsrSpaceCallback(ossimPlanetPointModel* model)
      :thePointModel(model)
      {
      }
      virtual void lsrSpaceChanged(ossimPlanetLsrSpaceTransform* /*lsrSpace*/)
      {
         if(thePointModel)
         {
            thePointModel->dirtyBound();
            thePointModel->setRedrawFlag(true);
         }
      }
   protected:
      ossimPlanetPointModel* thePointModel;
   };
   ossimPlanetPointModel();
   virtual ~ossimPlanetPointModel();
   
   /**
    * For now we will do nothing for the update.  This is reserved for future background operations on a node
    * to update itself.  It iwll be up to the node's responsibility to update only its dirty components.  Note
    *  this is not synched to the Update Visitor but this virtual method is an interface to an UpdateOperation.
    */
   virtual void update(){}
   
   
   /**
    * It will keep in sync during the update stage the node this point model is 
    * controlling with a LsrSpace transform.
    *
    * @param nv Node visitor base for all visitors.
    */
   virtual void traverse(osg::NodeVisitor& nv);
   
   /**
    * This will be the point model controlled by this class.  
    * During the update traversal it will be aded to the ossimPlanetLsrSpaceTransform
    *
    * @param node This node is assumed to be relative values from the LsrSpaceTransform
    */
   void setNode(osg::Node* node);
   
   /**
    * @return Returns a constant reference to a Local Space Reference for this point model.
    */
   const ossimPlanetLsrSpaceTransform* lsrSpace()const{return theLsrSpaceTransform.get();}
   
   /**
    * @return Returns a reference to a Local Space Reference for this point model.
    */
   ossimPlanetLsrSpaceTransform* lsrSpace(){return theLsrSpaceTransform.get();}
   
   /**
    * This Will copy the LSR parameters.
    *
    * @param lsr  The Local Space Reference to copy the parameters from.
    */
   void copyLsrSpaceParameters(const ossimPlanetLsrSpaceTransform& lsr);
   
   /**
    * overrides set layer so we can pass the model immediatley to the Local Space Reference
    * transform.
    *
    * @param layer The ossimPlanetLayer we are part of.
    */
   virtual void setLayer(ossimPlanetLayer* layer);
   
   virtual osg::BoundingSphere computeBound()const
   {
      if(theLsrSpaceTransform.valid())
      {
         return theLsrSpaceTransform->computeBound();
      }
      return osg::BoundingSphere(osg::Vec3d(0.0,0.0,0.0), -1);
   }
protected:
   /**
    * Checks if the required pointers are set.  Will see if layer and model exist.
    */
   bool checkPointers();
   
   /**
    * General mutex to sync access to the parameters with other threads.
    */
   ossimPlanetReentrantMutex thePointModelPropertyMutex;
   
   /**
    * The Local space node will be added to the LsrSpace transform.
    */
   osg::ref_ptr<ossimPlanetLsrSpaceTransform> theLsrSpaceTransform;
   
   /**
    * Indication that we need to update the matrix transform to encapsulate the node.
    */
   bool                    theNodeChangedFlag;
   
   /**
    * Direct access to the node that is a child of MatrixTransform.
    */
   osg::ref_ptr<osg::Node> theNode;
   
   osg::ref_ptr<LsrSpaceCallback> theLsrSpaceCallback;
};

#endif
