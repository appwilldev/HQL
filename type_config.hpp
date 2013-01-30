/* -*- c++ -*-
 *
 * file: type_config.hpp
 * author: KDr2
 *
 */


#ifndef _TYPE_CONFIG_HPP
#define _TYPE_CONFIG_HPP

#include <inttypes.h>

#include <set>
#include <map>
#include <vector>
#include <string>

using std::set;
using std::map;
using std::vector;
using std::string;

#define TYPE_CAT_BIT_OFFSET 63
#define FULLNAME_IS_CAT(fn) ((fn>>TYPE_CAT_BIT_OFFSET)==1U)
#define FULLNAME_SET_CAT(fn) (fn|(1ULL<<TYPE_CAT_BIT_OFFSET))
#define FULLNAME_RM_CAT(fn) (fn&~(1ULL<<TYPE_CAT_BIT_OFFSET))

class TypeModel{
public:
    
    TypeModel(uint8_t, string);
    virtual ~TypeModel();
    
    uint8_t id;
    string name;
};

class TypeEntity : public TypeModel{
public:
    TypeEntity(uint8_t, string);
    virtual ~TypeEntity();
};

class TypeRelation : public TypeModel{
public:
    TypeRelation(uint8_t, string, TypeModel*, TypeModel*);
    virtual ~TypeRelation();
    TypeModel *left;
    TypeModel *right;
};

class TypeCategory : public TypeModel{
public:
    
    TypeCategory(string);
    ~TypeCategory();
    vector<TypeModel*> subtypes;
};

class TypeConfig{
    
public:
    static void register_type(TypeModel*);
    static void setup_from_json(const string&);
    static void clear();
    static bool has_type(const string&);

    static const uint8_t type_id(const string&);
    static const string type_name(uint8_t);

    static bool is_subtype(const string&, const string&);
    static set<string> get_subtypes(const string&);
    
    static const string left_type(const string&);
    static const string right_type(const string&);
    
    static map<string, TypeModel*> all_types;
    static map<uint8_t, TypeModel*> rall_types;
    static map<string, TypeCategory*> categories;
    
};

#endif



