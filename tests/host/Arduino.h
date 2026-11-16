#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
class String { std::string s; public:
 String()=default;
 String(const char* c):s(c){}
 String(char c):s(1,c){}
 String(std::string c):s(c){}
 String(int i):s(std::to_string(i)){}
 String(float f,int digits=2){std::ostringstream o;o<<std::fixed<<std::setprecision(digits)<<f;s=o.str();}
 String(unsigned int i):s(std::to_string(i)){}
 String(long i):s(std::to_string(i)){}
 void reserve(size_t n){s.reserve(n);}
 String &operator+=(const String &x){s+=x.s;return *this;}
 String &operator+=(const char* c){s+=c;return *this;}
 String &operator+=(char c){s+=c;return *this;}
 const char* c_str()const{return s.c_str();}
 friend String operator+(String a,const String &b){a+=b;return a;}
};
uint32_t millis();

struct FakeSerial { template<typename... Args> int printf(const char* ,Args...){return 0;} void println(const char*){} };
extern FakeSerial Serial;
