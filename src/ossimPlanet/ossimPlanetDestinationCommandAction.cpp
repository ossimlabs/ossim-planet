#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>
#include <ossim/base/ossimCommon.h>

const char ossimPlanetDestinationCommandAction::theWhitespace[] = " \t";

ossimPlanetDestinationCommandAction::ossimPlanetDestinationCommandAction(const ossimString& code, 
                                                                         const ossimString& originatingFederate)
:ossimPlanetAction(originatingFederate)
{
   setSourceCode(code);
}

ossimString ossimPlanetDestinationCommandAction::argListSource() const
{
   int start = -1;
   
   if (theCommand != "#") 
   {
      start = theSourceCode.find_first_not_of(theWhitespace, 0);   // eat leading whitespace
      int end = theSourceCode.find_first_of(theWhitespace, start);
      start = theSourceCode.find_first_not_of(theWhitespace, end); // eat target
      end = theSourceCode.find_first_of(theWhitespace, start);
      start = theSourceCode.find_first_not_of(theWhitespace, end); // eat command
      
   }
   return (start > 0) ? ossimString(theSourceCode.substr(start, theSourceCode.length() - start)) :
      ossimString();
}

bool ossimPlanetDestinationCommandAction::setSourceCode(const ossimString& newSourcecode)
{
   bool result = true;
   theSourceCode = newSourcecode;
   
   int start = theSourceCode.find_first_not_of(theWhitespace, 0);
   bool isComment = start < 0 || theSourceCode[(size_t)start] == '#';
   
   if (isComment) 
   {
      theTokens.clear();
      theTokens.push_back(":");
      theTokens.push_back("#");
   }
   else
   {
      bool unbalancedBraces;
      ossim::lexQuotedTokens(theSourceCode, start, theWhitespace, "{}", theTokens, unbalancedBraces);
      if (unbalancedBraces || theTokens.size() == 1)
      {
         theTokens.clear();
         theTokens.push_back(":");
         theTokens.push_back("#syntaxerror");
         result = false;
      }
   }
   theArgCount = 0;
   if(theTokens.size() > 2)
   {
      theArgCount = theTokens.size()-2;
   }
   
   setTarget(theTokens[0]);
   setCommand(theTokens[1]);
   
   return result;
}

void ossimPlanetDestinationCommandAction::print(std::ostream& s)const
{
   const char* pre[]  = {"", "{"};
   const char* postToken[] = {" ", "", "} ", "}" };
   ossim_uint32 i;
   for (i = 0; i < theTokens.size(); i++) 
   {
      ossim_int32 needsQuoting = theTokens[i].empty() || (int)theTokens[i].find_first_of(theWhitespace, 0) != -1;
      ossim_int32 lastToken = i == theTokens.size() - 1;
      s << pre[needsQuoting] << theTokens[i] << postToken[2*needsQuoting + lastToken];
   }
}

void ossimPlanetDestinationCommandAction::read(std::istream& s)
{
   ossimString buffer;
   buffer.reserve(1000);
   
   // read arbitrary length line from s into buffer.
   char c;
   for (s.get(c); s.gcount() == 1 && c != '\n' && !s.eof(); s.get(c))
      buffer.append(1, c);
   
   // init the ossimPlanetAction
   if (s || s.eof()) 
   {
      setSourceCode(buffer);
   } 
}

