/* -*- c++ -*-
 *
 * file: cmn_util.hpp
 * author: KDr2
 *
 */

#ifndef _COMMON_UTIL_H
#define _COMMON_UTIL_H

#include <inttypes.h>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>

using std::string;

inline string num2str(uint8_t num)
{
    char buf[4];
    sprintf(buf, "%u%c", num, '\0');
    return string(buf);
}


inline string num2str(uint64_t num)
{
    std::ostringstream convert;
    convert<<num;
    return convert.str();
}

inline string num2str(int64_t num)
{
    std::ostringstream convert;
    convert<<num;
    return convert.str();
}

inline string num2str(double num)
{
    std::ostringstream convert;
    int64_t v =  static_cast<int64_t>(num);
    if(abs(static_cast<int>((num-v)*1000))<1){
        convert<<v;
    }else{
        convert.setf(std::ostringstream::fixed);
        convert.precision(3);
        convert<<num;
    }
    return convert.str();
}

inline string& str_escape_quote(string &o)
{
    string::iterator it = o.begin();
    string::iterator tmp;
    while(it!=o.end()){
        if(*it=='\\'){
            tmp = it + 1;
            if(tmp==o.end()) break;
            switch(*tmp){
            case '"':
                *it = '"';
                it = o.erase(tmp);
                continue;
            default:
                ++it;
                continue;
            }
        }
        ++it;
    }
    return o;
}

inline string& str_escape(string &o)
{
    string::iterator it = o.begin();
    string::iterator tmp;
    while(it!=o.end()){
        if(*it=='\\'){
            tmp = it + 1;
            if(tmp==o.end()) break;
            switch(*tmp){
            case 'a':
                *it = '\a';
                it = o.erase(tmp);
                continue;
            case 'b':
                *it = '\b';
                it = o.erase(tmp);
                continue;
            case 'f':
                *it = '\f';
                it = o.erase(tmp);
                continue;
            case 'n':
                *it = '\n';
                it = o.erase(tmp);
                continue;
            case 'r':
                *it = '\r';
                it = o.erase(tmp);
                continue;
            case 't':
                *it = '\t';
                it = o.erase(tmp);
                continue;
            case 'v':
                *it = '\v';
                it = o.erase(tmp);
                continue;
            case '\'':
                *it = '\'';
                it = o.erase(tmp);
                continue;
            case '"':
                *it = '"';
                it = o.erase(tmp);
                continue;
            case '?':
                *it = '\?';
                it = o.erase(tmp);
                continue;
            default:
                ++it;
                continue;
            }
        }
        ++it;
    }
    return o;
}

#endif
