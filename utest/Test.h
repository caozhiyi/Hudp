#ifndef HEADER_UTEST_TEST
#define HEADER_UTEST_TEST

#include <iostream>

#define Expect_Char(a, b)  do{if(strcmp(a, b) != 0) abort();}while(0)
#define Expect_Value(a, b) do{if(a != b) abort();}while(0)
#define Expect_True(v)     do{if(!v) abort();}while(0)
#define Expect_False(v)    do{if(v) abort();}while(0)

#endif