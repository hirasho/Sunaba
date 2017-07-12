#ifndef INCLUDED_SUNABA_BASE_H
#define INCLUDED_SUNABA_BASE_H

namespace Sunaba{
//死ぬ(実装はOsPc.cpp)
void die(const char* filename, int line, const char* message);
void writeLog(const char* message);
void writeLog(const char* filename, int line, const char* message);
} //namespace Sunaba

//マクロ類
#define STRONG_ASSERT(exp) ((!!(exp)) || (Sunaba::die(__FILE__, __LINE__, #exp), 0))
#define STRONG_ASSERT_MSG(exp, msg) ((!!(exp)) || (Sunaba::die( __FILE__, __LINE__, #msg), 0))
#define WRITE_LOG(msg) {Sunaba::writeLog(__FILE__, __LINE__, msg);}
#define HALT(msg) {Sunaba::die(__FILE__, __LINE__, #msg);}
#define DELETE(x) {delete (x); (x) = 0;}
#define DELETE_ARRAY(x) {delete[] (x); (x) = 0;}
#define OPERATOR_DELETE(x) {operator delete((x)); (x) = 0;}

//デバグとリリースで分岐するもの
#ifndef NDEBUG
#define ASSERT(exp) ((!!(exp)) || (Sunaba::die(__FILE__, __LINE__, #exp), 0))
#define ASSERT_MSG(exp, msg) ((!!(exp)) || (Sunaba::die(__FILE__, __LINE__, #msg), 0))
#else //NDEBUG
#define ASSERT(exp)
#define ASSERT_MSG(exp, msg)
#endif //NDEBUG

#include <new> //placement new

#endif
