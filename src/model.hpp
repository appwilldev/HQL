/* -*- c++ -*-
 *
 * file: new_model.hpp
 * author: KDr2
 *
 */



#ifndef _MODEL_HPP
#define _MODEL_HPP

#include <inttypes.h>

#include <set>
#include <string>

#define NDEBUG
#include "../vendor/libjson/libjson.h"
#undef  NDEBUG

using std::set;
using std::string;

template<typename T>
struct ModelDefaultAttr{
    T value(){return T();};
};

/*
 * class ExtraMatchDataInfo
 *
 */

class ExtraMatchDataInfo{
public:
    ExtraMatchDataInfo():relation_info(0){};
    ExtraMatchDataInfo(const ExtraMatchDataInfo &o):
        keys(o.keys), relation_info(o.relation_info)
    {};
    virtual ~ExtraMatchDataInfo(){};

    const ExtraMatchDataInfo operator=(const ExtraMatchDataInfo &o){
        if(this == &o) return *this;
        keys = o.keys;
        relation_info = o.relation_info;
        return *this;
    };

    const ExtraMatchDataInfo operator+=(const ExtraMatchDataInfo &o){
        keys.insert(o.keys.begin(), o.keys.end());
        relation_info = (relation_info >= o.relation_info) ? relation_info : o.relation_info;
        return *this;
    };

    void add_key(const string &k){keys.insert(k);};
    void set_relation_info(const uint8_t t){relation_info = t;};

    //private:
    set<string> keys;

    /*
     * 0=none
     * 1=fullnames
     * 2=[fullnames, reverse_fullnames, a_reversed_relation
     */
    uint8_t relation_info;
};

/*
 * class Model
 *
 */
class Model{
public:
    Model(const JSONNode&);

    const string type() const;
    uint8_t type_id() const;
    uint64_t fullname() const;
    uint64_t left() const;
    uint64_t right() const;
    bool deleted() const;
    const string rel_cache_key() const;

    template<typename T>
    T attr(const string& name) const;

    bool invalid;

private:
    uint64_t get_uinit64(const string&) const;
    JSONNode data;
};



#endif
