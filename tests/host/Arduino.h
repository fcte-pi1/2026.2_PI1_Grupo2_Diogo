#pragma once
#include <cstdint>
#include <string>
class String { std::string s; public:
 String()=default;
 String(const char* c):s(c){}
 String(char c):s(1,c){}
 String(std::string c):s(c){}
 String(int i):s(std::to_string(i)){}
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
