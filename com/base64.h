
#ifndef __BASE64_H__
#define __BASE64_H__
#include <string>  
  
/*base64加密*/
std::string base64_encode(unsigned char const* , unsigned int len);  
std::string base64_decode(std::string const& s);  

#endif //__BASE64_H__
