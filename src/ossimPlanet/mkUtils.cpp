#include <ossimPlanet/mkUtils.h>
#include <ossim/base/ossimCommon.h>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimCommon.h>
#include <iostream>
#include <stack>

std::istream& mkUtils::planetSkipws(std::istream& in)
{
   int c = in.peek();
   while(!in.fail()&&
         ((c == ' ') ||
         (c == '\t') ||
         (c == '\n')||
         (c == '\r')))
   {
      in.ignore(1);
      c = in.peek();
   }
   
   return in;
}

bool mkUtils::writeOsgObjectToStream(std::ostream& out, const osg::Object* node, const std::string& extension)
{
    osg::ref_ptr<osgDB::ReaderWriter> writer = osgDB::Registry::instance()->getReaderWriterForExtension(extension);
    return writer.valid() ? writer->writeObject(*node, out).success() : false;
}

bool mkUtils::isDouble(const std::string& s)
{
    char* end;
    const char* start = s.c_str();
    strtod(start, &end);
    return start != end;
}

bool mkUtils::isInt(const std::string& s)
{
    char* end;
    const char* start = s.c_str();
    strtoul(start, &end, 0);
    return start != end;
}

double mkUtils::asDouble(const std::string& s)
{
    char* end;
    const char* start = s.c_str();
    double x = strtod(start, &end);
    return (start != end) ? x : ossim::nan();
}

bool mkUtils::hasSuffix(const std::string& s, const std::string& suffix)
{
    std::string::size_type sSize = s.size();
    std::string::size_type suffixSize = suffix.size();
    return (suffixSize <= sSize) ? suffix == s.substr(sSize - suffixSize, suffixSize) : false;
}

ossim_int64 mkUtils::factorial(int n)
{
    // only factorials for small n can fit in int64 so just use a 168 byte lookup table.
    
    const int tableLength = 21;
    if((n < 0) || (n > tableLength)) return 0;
//     assert(n >= 0 && n < tableLength);
    
    static const ossim_int64 table[] = { 1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 
        39916800, 479001600, 6227020800LL, 87178291200LL, 1307674368000LL, 20922789888000LL, 
        355687428096000LL, 6402373705728000LL, 121645100408832000LL, 2432902008176640000LL };
    // XXX add this back in later:  static_check(tableLength == sizeof(table)/sizeof(table[0]), tableLength_must_equal_actual_table_length);

    return table[n];
}

void mkUtils::lexBraceQuotedTokens(const std::string& str, unsigned int startIdx, const char* whitespace, std::vector<std::string>* tokens, bool* unbalancedBraces)
{
   ossim::lexQuotedTokens(str, startIdx, whitespace, "{}", *tokens, *unbalancedBraces);
#if 0
   if(!whitespace||!tokens||!unbalancedBraces) return;
//     assert(whitespace != NULL);
//     assert(tokens != NULL);
//     assert(unbalancedBraces != NULL);
    
    const char openQuote('{'), closeQuote('}');
    
    tokens->clear();
    *unbalancedBraces = false;
    
    unsigned int endIdx;
    while (startIdx < str.length())
    {
	if (str[startIdx] == openQuote)
        {
            int openBraceCount = 1;
            
            if (startIdx+1 < str.length())
            {
                startIdx++;
                if (str[startIdx] != closeQuote)
                {
                    endIdx = startIdx+1;
                    while (endIdx < str.length() && openBraceCount > 0)
                    {
                        if (str[endIdx] == openQuote)
                            openBraceCount++;
                        else if (str[endIdx] == closeQuote)
                            openBraceCount--;
                        endIdx++;
                    }
                }
                else
                {
                    openBraceCount = 0;
                    startIdx++;
                    endIdx = startIdx+1;
                }
            }
            
            if (openBraceCount == 0)
            {
                tokens->push_back(str.substr(startIdx, endIdx-1-startIdx));
            }
            else
            {
                *unbalancedBraces = true;
                endIdx = str.length();
            }
            
	}
        else if (str[startIdx] == closeQuote)
        {
            *unbalancedBraces = true;
            endIdx = str.length();
            
	}
        else
        {
            endIdx = str.find_first_of(whitespace, startIdx);
            tokens->push_back(str.substr(startIdx, endIdx-startIdx));
	}
	
	startIdx = str.find_first_not_of(whitespace, endIdx);
    }
#endif
}

void mkUtils::hprToQuat(osg::Quat& quat, const osg::Vec3d& hpr)
{
//   const double yaw(osg::DegreesToRadians(hpr[1]));
//   const double pitch(osg::DegreesToRadians(hpr[2]));
//   const double roll(osg::DegreesToRadians(-hpr[0]));
   const double yaw(osg::DegreesToRadians(-hpr[0]));
   const double pitch(osg::DegreesToRadians(hpr[1]));
   const double roll(osg::DegreesToRadians(hpr[2]));
   const double cy ( cos ( yaw * 0.5 ) );
   const double cp ( cos ( pitch * 0.5 ) );
   const double cr ( cos ( roll * 0.5 ) );
   const double sy ( sin ( yaw * 0.5 ) );
   const double sp ( sin ( pitch * 0.5 ) );
   const double sr ( sin ( roll * 0.5 ) );
   
   const double qw ( cy*cp*cr + sy*sp*sr );
   const double qx ( sy*cp*cr - cy*sp*sr );
   const double qy ( cy*sp*cr + sy*cp*sr );
   const double qz ( cy*cp*sr - sy*sp*cr );
   
   quat.set ( qx, qy, qz, qw );
}

void mkUtils::quatToHpr(osg::Vec3d& hpr, const osg::Quat& quat)
{
   // From the reference material the h p r is about
   // Y, Z, and X
   // our LSR axiz says Z, X, Y
   const double q0 ( quat[3] );
   const double q1 ( quat[0] );
   const double q2 ( quat[1] );
   const double q3 ( quat[2] );
   const double sqx(q1*q1);
   const double sqy(q2*q2);
   const double sqz(q3*q3);
   const double sqw(q0*q0);
   
   double h = atan2(2.0 * (q1*q2 + q3*q0),(sqx - sqy - sqz + sqw));
   double p = atan2(2.0 * (q2*q3 + q1*q0),(-sqx - sqy + sqz + sqw));
   double r = asin(-2.0 * (q1*q3 - q2*q0)/(sqx + sqy + sqz + sqw));
//   const double h(atan2 ( 2 * (q2*q3 + q0*q1), (q0*q0 - q1*q1 - q2*q2 + q3*q3)));
//   double p(asin ( -2 * (q1*q3 - q0*q2)));
//   double r(atan2 ( 2 * (q1*q2 + q0*q3), (q0*q0 + q1*q1 - q2*q2 - q3*q3)));
   //   hpr = osg::Vec3d(osg::RadiansToDegrees(-r),
   //                    osg::RadiansToDegrees(h),
   //                    osg::RadiansToDegrees(p));
      hpr = osg::Vec3d(osg::RadiansToDegrees(-h),
                       osg::RadiansToDegrees(p),
                       osg::RadiansToDegrees(r));
}

bool mkUtils::matrixToHpr( osg::Vec3d& hpr, 
                          const osg::Matrixd& rotation )
{
//   std::cout << "HEADING = " << ossim::radiansToDegrees(atan(rotation(0,1)/rotation(0,0))) << std::endl;
//   std::cout << "PITCH   = " << ossim::radiansToDegrees(asin(-rotation(0,2))) << std::endl;
//   std::cout << "ROLL    = " << ossim::radiansToDegrees(atan(rotation(1,2)/rotation(2,2))) << std::endl;
   
   osg::Quat q;
   
   q.set(rotation);
   
   quatToHpr(hpr, q);

   return true;
}

bool mkUtils::matrixToHpr( osg::Vec3d& hpr,
                           const osg::Matrixd& lsrMatrix,
                           const osg::Matrixd& rotationalMatrix)
{
   bool result = false;
   osg::Matrixd invertLsr;
    
    hpr[0] = 0.0;
    hpr[1] = 0.0;
    hpr[2] = 0.0;
   osg::Matrixd m(lsrMatrix(0,0), lsrMatrix(0, 1), lsrMatrix(0,2), 0.0,
                  lsrMatrix(1,0), lsrMatrix(1, 1), lsrMatrix(1,2), 0.0,
                  lsrMatrix(2,0), lsrMatrix(2, 1), lsrMatrix(2,2), 0.0,
                  0.0,0.0,0.0, 1.0);
    if(invertLsr.invert(m))
    {
        result = matrixToHpr(hpr, rotationalMatrix*invertLsr);
        if(std::abs(hpr[0]) < FLT_EPSILON)
        {
            hpr[0] = 0.0;
        }
        if(std::abs(hpr[1]) < FLT_EPSILON)
        {
            hpr[1] = 0.0;
        }
        if(std::abs(hpr[2]) < FLT_EPSILON)
        {
            hpr[2] = 0.0;
        }
    }
    return result;
}

bool mkUtils::extractObjectAndArg(std::string& resultObject,
                                  std::string& resultArg,
                                  const std::string& inputValue,
                                  const char quotes[2])
{
   const char openQuote(quotes[0]), closeQuote(quotes[1]);
   std::vector<std::string> tokens;
   bool unbalancedQuotes=false;
   ossim_uint32 end = 1;
   ossim_uint32 start=0;
   ossim_uint32 totalOpenQuoteCount = 0;
   const char* whitespace = " \t\n\r";
   // while (start < inputValue.length() && start >= 0)
   start = inputValue.find_first_not_of(whitespace, start);
   end = start + 1;
   if(inputValue[start] != openQuote)
   {
      while (start < inputValue.length())      
      {
         if (inputValue[start] == openQuote)
         {
            int openBraceCount = 1;
            ++totalOpenQuoteCount;
            if (start+1 < inputValue.length())
            {
               ++start;
               // skip white space since this is the object portion and need te start of the first character
               //
               start = inputValue.find_first_not_of(whitespace, start);
               if (inputValue[start] != closeQuote)
               {
                  end = start+1;
                  while (static_cast<ossim_uint32>(end) < inputValue.length() &&
                         openBraceCount > 0)
                  {
                     if (inputValue[end] == openQuote)
                     {
                        openBraceCount++;
                        ++totalOpenQuoteCount;
                     }
                     else if (inputValue[end] == closeQuote)
                        openBraceCount--;
                     end++;
                  }
               }
               else
               {
                  openBraceCount = 0;
                  start++;
                  end = start+1;
               }
            }
            
            if (openBraceCount == 0)
            {
               tokens.push_back(inputValue.substr(start, end-1-start));
            }
            else
            {
               unbalancedQuotes = true;
               end = inputValue.length();
            }
            
         }
         else if (inputValue[start] == closeQuote)
         {
            unbalancedQuotes = true;
            end = inputValue.length();
            
         }
         else
         {
            end = inputValue.find_first_of(whitespace, start);
            tokens.push_back(inputValue.substr(start, end-start));
         }
         
         start = inputValue.find_first_not_of(whitespace, end);
      }
   }
   else
   {
      end = inputValue.find_first_of(whitespace, start);
      tokens.push_back(inputValue.substr(start, end-start));
      tokens.push_back(inputValue.substr(end, inputValue.size()));
   }
   if(unbalancedQuotes) return false;
   if(tokens.size() < 2) return false;
   bool requoteArg = totalOpenQuoteCount >0;
   resultObject = tokens[0];
   resultArg = "";
   ossim_uint32 maxSize = tokens.size()-1;
   ossim_uint32 idx = 0;
   for(idx =1; idx < maxSize; ++idx)
   {
      if(requoteArg)
      {
         resultArg    += (quotes[0] + tokens[idx]+quotes[1]);
      }
      else
      {
         resultArg    += tokens[idx];
      }
      resultArg += " ";
   }
   if(requoteArg)
   {
      resultArg    += (quotes[0] + tokens[idx]+quotes[1]);
   }
   else
   {
      resultArg    += tokens[idx];
   }
   
   return true;
}

ossimRefPtr<ossimXmlNode> mkUtils::newNodeFromObjectMessageRoute(const ossimString& v,
                                                                 const char quotes[2])
{
   std::istringstream in(v);
   const char openQuote = quotes[0];
   const char closeQuote = quotes[1];
   std::stack<ossimRefPtr<ossimXmlNode> > xmlStack;
   ossimRefPtr<ossimXmlNode> current;
   in >> mkUtils::planetSkipws;
   if(!in.good()) return current;
   current = new ossimXmlNode;
   xmlStack.push(current.get());
   
   if(in.peek() == openQuote)
   {
      in.ignore();
   }
   ossimString name;
   ossimString value;
   while(in.good()&&!xmlStack.empty())
   {
      in>>mkUtils::planetSkipws;
      if(in.peek() == openQuote)
      {
         // create a new xml node
         // add it to current and skip white space to parse out the name
         //
         ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
         name = "";
         value = "";
         current->addChildNode(node.get());
         xmlStack.push(node.get());
         current = node;
         in.ignore();
      }
      else if(in.peek() == closeQuote)
      {
         xmlStack.pop();
         if(xmlStack.size() > 0)
         {
            current = xmlStack.top();
         }
         in.ignore();
      }
      else
      {
         if(name.empty())
         {
            // read object name
            //
            while(in.good()&&(in.peek()!=' ')&&(in.peek()!=openQuote)&&(in.peek()!=closeQuote))
            {
               name += (char)in.get();
            }
            // on last pop there is a possibility that the name will be parsed to empty so don't overwrite.
            // we are basically done at this point and the main loop should kick out
            //
            if(!name.empty())
            {
               current->setTag(name);
            }
         }
         else if(value.empty())
         {
            while(in.good()&&(in.peek()!=closeQuote))
            {
               value += (char)in.get();
            }
            current->setText(value);
            // reset name and value;
            name  = "";
            value = "";
         }
      }
   }
   // check for error
   //
   if(!in.good())
   {
      if(xmlStack.size() != 1)
      {
         current = 0;
      }
   }

   return current;
}

ossimRefPtr<ossimXmlNode> mkUtils::newNodeFromObjectMessageRoute(const ossimString& receiverPath,
                                                                 const ossimString& command,
                                                                 const ossimString& v,
                                                                 const char quotes[2])
{
   ossimRefPtr<ossimXmlNode> objectDefinition = newNodeFromObjectMessageRoute(v, quotes);
   ossimRefPtr<ossimXmlNode> result;
   if(objectDefinition.valid())
   {
      result = new ossimXmlNode;
      result->setTag("Message");
      result->addAttribute("receiver", receiverPath);
      result->addAttribute("command", receiverPath);
      
   }
   return result.get();
}
