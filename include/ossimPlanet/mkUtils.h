#ifndef MKUTILS_H
#define MKUTILS_H

// Defines namespace mkUtils containing various helpful utility functions.
#ifdef WIN32
#ifndef __MINGW32__
#ifdef __MSC_VER
# if __MSC_VER < 1600
#   include <xmath.h>
# endif
#endif
#define M_LOG2E    1.44269504088896340736
#endif
#endif

#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Vec3d>
#include <osg/Vec4d>
#include <osg/Matrixd>
#include <osg/Matrix>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimXmlNode.h>
//#include <ossim/base/ossimCommon.h>
#include <limits>
#include <ossim/base/ossimXmlNode.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <iostream>


namespace mkUtils
{
#define OSG_INNER_PRODUCT_3X3(a,b,r,c) \
(((a(r,0)) * (b(0,c))) \
+((a(r,1)) * (b(1,c))) \
+((a(r,2)) * (b(2,c)))) 
   
   OSSIMPLANET_DLL std::istream& planetSkipws(std::istream& in);
   
   OSSIMPLANET_DLL bool writeOsgObjectToStream(std::ostream& out, const osg::Object* node, const std::string& extension);
   
   inline void mult3x3(osg::Matrixd& result,
                       const osg::Matrixd& lhs,
                       const osg::Matrixd& rhs)
   {
      result(0,0) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 0, 0);   
      result(0,1) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 0, 1);
      result(0,2) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 0, 2);
      result(1,0) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 1, 0);
      result(1,1) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 1, 1);
      result(1,2) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 1, 2);
      result(2,0) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 2, 0);
      result(2,1) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 2, 1);
      result(2,2) = OSG_INNER_PRODUCT_3X3(lhs, rhs, 2, 2);
   }
      
   inline bool almostEqual(const osg::Matrixd& lhs,
                           const osg::Matrixd& rhs,
                           double epsilon=DBL_EPSILON)
   {
      return ((std::fabs(lhs(0,0)-rhs(0,0)) <= epsilon) &&
              (std::fabs(lhs(1,0)-rhs(1,0)) <= epsilon) &&
              (std::fabs(lhs(2,0)-rhs(2,0)) <= epsilon) &&
              (std::fabs(lhs(3,0)-rhs(3,0)) <= epsilon) &&
              (std::fabs(lhs(0,1)-rhs(0,1)) <= epsilon) &&
              (std::fabs(lhs(1,1)-rhs(1,1)) <= epsilon) &&
              (std::fabs(lhs(2,1)-rhs(2,1)) <= epsilon) &&
              (std::fabs(lhs(3,1)-rhs(3,1)) <= epsilon) &&
              (std::fabs(lhs(0,2)-rhs(0,2)) <= epsilon) &&
              (std::fabs(lhs(1,2)-rhs(1,2)) <= epsilon) &&
              (std::fabs(lhs(2,2)-rhs(2,2)) <= epsilon) &&
              (std::fabs(lhs(3,2)-rhs(3,2)) <= epsilon) &&
              (std::fabs(lhs(0,3)-rhs(0,3)) <= epsilon) &&
              (std::fabs(lhs(1,3)-rhs(1,3)) <= epsilon) &&
              (std::fabs(lhs(2,3)-rhs(2,3)) <= epsilon) &&
              (std::fabs(lhs(3,3)-rhs(3,3)) <= epsilon));
   }
   
   /**
    * Gives you a nice coefficient that can be used to fade values in and out as a multiplier.  
    * Could also be used as an alpha on billboard colors and other color bindings.
    */
   inline double fadeCoefficient(double currentValue, 
                                 double referenceValue)
   {
      return exp( - currentValue / referenceValue );
   }
   
   /*     template<typename T> */
   /*     inline bool almostEqual(T x, T y, T tolerence = std::numeric_limits<T>::epsilon()) */
   /*         // are x and y within tolerence distance of each other */
   /*         { return std::abs(x - y) <= tolerence; } */
   
   inline bool hasPrefix(const std::string& s, const std::string& prefix)
   // does s have the given prefix?
   { return prefix == s.substr(0, prefix.size()); }
   
   OSSIMPLANET_DLL bool hasSuffix(const std::string& s, const std::string& suffix);
   // does s have the given suffix?
   
   OSSIMPLANET_DLL bool isDouble(const std::string& s);
   // does s represent a C-style double constant?
   
   OSSIMPLANET_DLL bool isInt(const std::string& s);
   // does s represent a C-style integer constant?
   
   OSSIMPLANET_DLL double asDouble(const std::string& s);
   // value of s as a double, or NAN if not a double.
   
   inline float asFloat(const std::string& s)
   // value of s as a float, or NAN if not a float.
   { return static_cast<float>(asDouble(s)); }
   
   inline long asInt(const std::string& s)
   // value of s as a long, or 0 if not a long
   { return strtoul(s.c_str(), NULL, 0); }
   
   template <typename T>
   inline std::string asString(const T& x)
   // string representation of x
   {
      std::ostringstream s;
      s.precision(16);
      s << x;
      return s.str();
   }
   
   template <typename T>
   inline bool inInterval(T x, T a, T b)
   // is x in the closed interval [a,b]?
   { return x >= a && x <= b; }
   
   template <typename T>
   inline bool inOpenInterval(T x, T a, T b)
   // is x in the open interval (a,b)?
   { return x > a && x < b; }
   
   template <typename S, typename T> 
   inline T lerp(S x, T begin, T end)
   // linear interpolation from begin to end by x
   { return x*(end - begin) + begin; }
   
   template <typename T> 
   inline T inverseLerp(T x, T begin, T end)
   // inverse of lerp
   { return begin != end ? (x - begin)/(end - begin) : T(0); }
   
   template <typename S, typename T> 
   T quaderp(S x, T begin, T middle, T end)
   // quadratic interpolation through begin,middle,end by x
   {
      // newton interpolation
      const T a1 = S(2)*(middle - begin);
      const T a2 = S(2)*(end - middle) - a1;
      return x*((x - S(0.5))*a2 + a1) + begin;
   }
   
   template <typename T>
   inline T step(T x, T a)
   // x >= a
   { return T(x >= a); }
   
   template <typename T>
   T smoothstep(T x, T a, T b)
   // like step() but smoothly over [a,b] rather than sharply at a
   {
      assert(a <= b);
      
      T t = clamp((x - a)/(b - a), T(0), T(1));
      return t*t*(T(3) - T(2)*t);
   }
   
   template <typename T>
   T boxpulse(T x, T a, T b)
   // box function that is 1 in [a,b] but 0 elsewhere
   {
      assert(a <= b);
      
      return inInterval(x, a, b);
   }
   
   template <typename T> 
   inline T clamp(T x, T a, T b)
   // clamp x to [a, b]
   {
      // assert(a <= b);  // XXX we removed this, but we need to fix the real problem
      
      return std::min(std::max(x, a), b);
   }
   
   template <typename T>
   T wrap(T x, T a, T b)
   // wrap x modularly into [a,b)
   {
      assert(a <= b); 
      
      if (a == b)
         return a;
      else {
         T z = x < a ? b : a;
         return std::fmod(x - z, b - a) + z;
      }
   }
   
   void hprToQuat(osg::Quat& quat, const osg::Vec3d& hpr);
   void quatToHpr(osg::Vec3d& hpr, const osg::Quat& quat);
   bool matrixToHpr(osg::Vec3d& hpr, const osg::Matrixd& rotation );
   bool matrixToHpr(osg::Vec3d& hpr, const osg::Matrixd& lsrMatrix, 
                    const osg::Matrixd& rotationalMatrix);
   
   inline float negativeOneNthPow(int n)
   // compute pow(-1, n)
   {
      // this implementation avoids a conditional branch
      const float possibleResults[] = {1.0, -1.0};
      return possibleResults[n & 0x1];   
   }
   
   template <typename T>
   std::pair<T, T> quadraticRoots(T a, T b, T c)
   // evaluates quadradic formula (positive sqrt is first)
   {
      T s = sqrt(b*b - T(4)*a*c);
      T twoA = T(2)*a;
      return std::pair<T, T>((-b + s)/twoA, (-b - s)/twoA);
   }
   
   template <typename T>
   inline T fmodPos(T a, T b)
   {
      T x = std::fmod(a, b);
      return (x < T(0)) ? x + b : x;
   }
   
   template <typename T>
   inline T fmodPosAvg(T a, T b)
   { return fmodPos(a + b*T(0.5), b) - b*T(0.5); }
   
   template <typename T>
   inline T fmodPosClosest(T a, T b, T c)
   { return c + fmodPosAvg(a - c, b); }
   
   template <typename T>
   inline T grayCode(T x)
   // x's binary reflected Gray code
   { return (x >> 1) ^ x; }
   
   template <typename T>
   inline bool isPower2(T x)
   // is x a power of 2?  T should be integral
   { return !(x & (x - 1)) && x; }
   
   inline double round(double x) 
   { return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5); }
   
   template <typename T>
   inline T logbase(T x, T base) 
   { return log(x)/log(base); }
   
   inline double log2(double x)
   { return M_LOG2E*log(x); }
   
   inline int log2int(double x)
   { return (int)round(log2(x)); }
   
   template <typename T>
   T ceilPow2(T x)
   // return the smallest power of two that is >= x; T should be integral
   {
      T result(1);
      while (result < x) {
         assert(result != 0);  // assert we don't overflow result 
         result <<= 1;
      }
      return result;
   }
   
   inline osg::Vec3d modsphere2xyz(double radius, double theta, double phi) 
   // convert "modified" spherical coordinates (in radians) to cartesian coords.
   //     "modified" since angle measurements don't match mathematicians' standard conventions,
   //     instead use phi = lat and theta = lon convention, but in radians
   { return osg::Vec3d(radius*cos(phi)*cos(theta), radius*cos(phi)*sin(theta), radius*sin(phi)); }
   
   inline std::ostream& operator<<(std::ostream& s, const osg::Vec3f& v)
   { return (s << v.x() << ' ' << v.y() << ' ' << v.z()); } 
   inline std::ostream& operator<<(std::ostream& s, const osg::Vec4f& v)
   { return (s << v.x() << ' ' << v.y() << ' ' << v.z() << ' ' << v.w()); } 
   inline std::ostream& operator<<(std::ostream& s, const osg::Vec3d& v)
   { return (s << v.x() << ' ' << v.y() << ' ' << v.z()); } 
   inline std::ostream& operator<<(std::ostream& s, const osg::Vec4d& v)
   { return (s << v.x() << ' ' << v.y() << ' ' << v.z() << ' ' << v.w()); } 
   // convenient streaming operators not defined by osg
   
   ossim_int64 factorial(int n);
   // compute n!
   // require(n >= 0 && n < 21)
   
   void lexBraceQuotedTokens(const std::string& str, unsigned int startIdx, const char* whitespace, std::vector<std::string>* tokens, bool* unbalancedBraces);
   // lex str into tokens starting at position start using whitespace  
   //    chars as delimiters and curly braces to quote tokens containing
   //    whitespace. unbalancedBraces is true iff it aborted when detecting 
   //    unbalanced braces.
   // assert(whitespace != NULL);
   // assert(tokens != NULL);
   // assert(unbalancedBraces != NULL);
   
   OSSIMPLANET_DLL bool extractObjectAndArg(std::string& resultObject,
                                            std::string& resultArg,
                                            const std::string& inputValue,
                                            const char quotes[2] ="{}");
   template <typename T>
   inline void memClear(T& var)
   // zero out a variable's memory
   { memset(&var, 0, sizeof(T)); }
   
   OSSIMPLANET_DLL ossimRefPtr<ossimXmlNode> newNodeFromObjectMessageRoute(const ossimString& v,
                                                                           const char quotes[2]="{}");
   
   OSSIMPLANET_DLL ossimRefPtr<ossimXmlNode> newNodeFromObjectMessageRoute(const ossimString& receiverPath,
                                                                           const ossimString& command,
                                                                           const ossimString& v,
                                                                           const char quotes[2]);   
}

#endif
