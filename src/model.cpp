/* -*- c++ -*-
 *
 * file: model.cpp
 * author: KDr2
 *
 */

#include <iostream>
#include <algorithm>

#include <stdlib.h>

#include "cmn_util.hpp"
#include "type_config.hpp"
#include "model.hpp"



Model::Model(const JSONNode &n):
    data(n)
{}

const string Model::type() const{
    JSONNode::const_iterator it = data.find("type_name");
    if(it==data.end()) {
        it = data.find("type_id");
        if(it==data.end()){
            return "BAD-TYPE";
        }
    }
    if(it->type()==JSON_STRING){
        return it->as_string();
    }else if(it->type()==JSON_NUMBER){
        return TypeConfig::type_name((uint8_t)it->as_int());
    }
    return "BAD-TYPE";
}

uint8_t Model::type_id() const{
    return TypeConfig::type_id(type());
}


uint64_t Model::get_uinit64(const string &key) const{
    JSONNode::const_iterator it = data.find(key);
    if(it==data.end()) return 0;
    if(it->type()==JSON_NUMBER){
        return it->as_int();
    }else if(it->type()==JSON_STRING){
        string fs = it->as_string();
        return strtoul(fs.c_str(), NULL, 10);
    }
    return 0;
}

uint64_t Model::fullname() const{
    return get_uinit64("fullname");
}

uint64_t Model::left() const{
    return get_uinit64("left");
}

uint64_t Model::right() const{
    return get_uinit64("right");
}


const string Model::rel_cache_key() const
{
    uint64_t fn = this->fullname();
    uint8_t tid = fn & 0xFF;
    if(tid<=128) return "";
    return "R" + num2str(tid) + "_" + num2str(left()) + "_" + num2str(right());
}

template<typename T>
T Model::attr(const string& name) const
{
    return T();
}

template<>
JSONNode Model::attr<JSONNode>(const string& name) const
{
    if(name == "fullname" ||
       name == "left" ||
       name == "right"){
        JSONNode::const_iterator t = data.find(name);
        if(t==data.end()) return JSONNode();
        return *t;
    }
    
    JSONNode::const_iterator attrs = data.find("attributes");
    if(attrs==data.end()) return JSONNode();
    JSONNode::const_iterator ait = attrs->find(name);
    if(ait!=attrs->end()){
        return *ait;
    }
    return JSONNode();
}


template<>
bool Model::attr<bool>(const string& name) const
{
    JSONNode::const_iterator attrs = data.find("attributes");
    if(attrs==data.end()) return false;
    if(attrs->type()!=JSON_NODE){
        return false;
    }
    JSONNode::const_iterator ait = attrs->find(name);
    if(ait!=attrs->end() && ait->type()==JSON_BOOL){
        return ait->as_bool();
    }
    return false;
}

template<>
uint64_t Model::attr<uint64_t>(const string& name) const
{
    if(name == "fullname"){
        return this->fullname();
    }else if(name == "left"){
        return this->left();
    }else if(name == "right"){
        return this->right();
    }
    
    JSONNode::const_iterator attrs = data.find("attributes");
    if(attrs==data.end()) return 0;
    if(attrs->type()!=JSON_NODE){
        return 0;
    }

    JSONNode::const_iterator it = attrs->find(name);
    if(it==attrs->end()) return 0;
    if(it->type()==JSON_NUMBER){
        return it->as_int();
    }else if(it->type()==JSON_STRING){
        string fs = it->as_string();
        return strtoul(fs.c_str(), NULL, 10);
    }
    return 0;
}

template<>
double Model::attr<double>(const string& name) const
{
    JSONNode::const_iterator attrs = data.find("attributes");
    if(attrs==data.end()) return 0.0;
    if(attrs->type()!=JSON_NODE){
        return 0.0;
    }
    JSONNode::const_iterator ait = attrs->find(name);
    if(ait!=attrs->end() && ait->type()==JSON_NUMBER){
        return ait->as_float();
    }
    return 0.0;
}


template<>
string Model::attr<string>(const string& name) const
{
    JSONNode::const_iterator attrs = data.find("attributes");
    if(attrs==data.end()) return "";
    if(attrs->type()!=JSON_NODE){
        return "";
    }
    JSONNode::const_iterator ait = attrs->find(name);
    if(ait!=attrs->end() && ait->type()==JSON_STRING){
        return ait->as_string();
    }
    return "";
}


bool Model::deleted() const
{
    return this->attr<bool>("deleted");
}


