/* -*- c++ -*-
 *
 * file: type_config.cpp
 * author: KDr2
 *
 */

#include <iostream>
#include <algorithm>

#define NDEBUG
#include "vendor/libjson/libjson.h"
#undef  NDEBUG

#include "trollers.hpp"
#include "type_config.hpp"

TypeModel::TypeModel(uint8_t _id, string _name):
    id(_id), name(_name)
{}

TypeModel::~TypeModel()
{}



TypeEntity::TypeEntity(uint8_t _id, string _name):
    TypeModel(_id, _name)
{}

TypeEntity::~TypeEntity()
{}




TypeRelation::TypeRelation(uint8_t _id, string _name,
                           TypeModel *l, TypeModel *r):
    TypeModel(_id, _name), left(l), right(r)
{}

TypeRelation::~TypeRelation()
{}



TypeCategory::TypeCategory(string _name):
    TypeModel(0, _name)
{}

TypeCategory::~TypeCategory()
{}


map<string, TypeModel*> TypeConfig::all_types;
map<uint8_t, TypeModel*> TypeConfig::rall_types;
map<string, TypeCategory*> TypeConfig::categories;


void TypeConfig::register_type(TypeModel *t)
{
    all_types[t->name] = t;
    rall_types[t->id] = t;
}


/*
 * see type.json
 */
void TypeConfig::setup_from_json(const string &json)
{
    JSONNode n;
    try{
        n = libjson::parse(json);
        if(n.type()!=JSON_NODE){
            TrollersHolder::set_max_ns_num(4);
            goto error;
        }
        JSONNode::const_iterator it = n.find("hql_namespace_num");
        if(it == n.end() || it->type()!=JSON_NUMBER){
            TrollersHolder::set_max_ns_num(4);
        }else{
            TrollersHolder::set_max_ns_num(static_cast<uint8_t>(it->as_int()));
        }
        it = n.find("type");
        if(it == n.end() || it->type()!=JSON_NODE){
            goto error;
        }

        // entities
        JSONNode::const_iterator entity = it->find("entity");
        if(entity != it->end() && entity->type()==JSON_NODE){
            JSONNode::const_iterator eit = entity->begin();
            while(eit!=entity->end()){
                register_type(new TypeEntity(static_cast<uint8_t>(eit->as_int()),
                                             eit->name()));
                ++eit;
            }
        }

        // relations
        JSONNode::const_iterator relation = it->find("relation");
        if(relation != it->end() && relation->type()==JSON_NODE){
            JSONNode::const_iterator rit = relation->begin();
            while(rit!=relation->end()){
                string rel_name = rit->name();
                JSONNode rel_info = rit->as_array();
                uint8_t rid = 128 + static_cast<uint8_t>(rel_info[0].as_int());
                string tmp_lname = rel_info[1].as_string();
                string tmp_rname = rel_info[2].as_string();
                if(all_types.find(tmp_lname)==all_types.end()){
                    std::cerr<<"Warning(0): Bad Left Type: "<< tmp_lname<<std::endl;
                    ++rit;
                    continue;
                }
                if(all_types.find(tmp_rname)==all_types.end()){
                    std::cerr<<"Warning(0): Bad Right Type: "<< tmp_rname<<std::endl;
                    ++rit;
                    continue;
                }

                TypeModel *l = all_types[tmp_lname];
                TypeModel *r = all_types[tmp_rname];
                register_type(new TypeRelation(rid, rel_name, l, r));
                ++rit;
            }
            //do it again
            rit = relation->begin();
            while(rit!=relation->end()){
                string rel_name = rit->name();
                if(all_types.find(rel_name)!=all_types.end()){
                    ++rit;
                    continue;
                }
                JSONNode rel_info = rit->as_array();
                uint8_t rid = 128 + static_cast<uint8_t>(rel_info[0].as_int());
                string tmp_lname = rel_info[1].as_string();
                string tmp_rname = rel_info[2].as_string();
                if(all_types.find(tmp_lname)==all_types.end()){
                    std::cerr<<"Warning(1): Bad Left Type: "<< tmp_lname<<std::endl;
                    ++rit;
                    continue;
                }
                if(all_types.find(tmp_rname)==all_types.end()){
                    std::cerr<<"Warning(1): Bad Right Type: "<< tmp_rname<<std::endl;
                    ++rit;
                    continue;
                }

                TypeModel *l = all_types[tmp_lname];
                TypeModel *r = all_types[tmp_rname];
                register_type(new TypeRelation(rid, rel_name, l, r));
                ++rit;
            }
        }

        // categories
        JSONNode::const_iterator category = it->find("category");
        if(category != it->end() && category->type()==JSON_NODE){
            JSONNode::const_iterator cit = category->begin();
            while(cit!=category->end()){
                string cat_name = cit->name();
                TypeCategory *cat = new TypeCategory(cat_name);
                categories[cat_name] = cat;
                JSONNode cat_info = cit->as_array();
                if(cat_info.size()>0){
                    JSONNode::const_iterator stit = cat_info.begin();
                    while(stit!=cat_info.end()){
                        string tmp_name = stit->as_string();
                        if(all_types.find(tmp_name)!=all_types.end()){
                            cat->subtypes.push_back(all_types[tmp_name]);
                        }else{
                            std::cerr<<"Warning: Bad Sub Type: "<< tmp_name<<std::endl;
                        }
                        ++stit;
                    }
                }
                ++cit;
            }
        }
    }catch(std::invalid_argument e){
        goto error;
    }
    return;
 error:
    std::cerr<<"Warning: Types Setup Error"<<std::endl;
    return;
}

void TypeConfig::clear()
{

    map<string, TypeModel*>::iterator mit=all_types.begin();
    while(mit!=all_types.end()){
        delete mit->second;
        ++mit;
    }

    map<string, TypeCategory*>::iterator cit=categories.begin();
    while(cit!=categories.end()){
        delete cit->second;
        ++cit;
    }


    all_types.clear();
    rall_types.clear();
    categories.clear();
}

bool TypeConfig::has_type(const string &t)
{
    if(all_types.find(t) != all_types.end()){
        return true;
    }
    if(categories.find(t) != categories.end()){
        return true;
    }
    return false;
}

const uint8_t TypeConfig::type_id(const string &name)
{
    map<string, TypeModel*>::iterator it = all_types.find(name);
    if (it == all_types.end()) return 0;
    return it->second->id;
}


const string TypeConfig::type_name(uint8_t tid)
{
    map<uint8_t, TypeModel*>::iterator it = rall_types.find(tid);
    if (it == rall_types.end()) return "BAD-TYPE";
    return it->second->name;
}

bool TypeConfig::is_subtype(const string &type, const string &cat)
{
    if(type == cat) return true;
    map<string, TypeCategory*>::iterator it = categories.find(cat);
    if(it == categories.end()) return false;
    vector<TypeModel*>::iterator sit = it->second->subtypes.begin();
    while(sit!=it->second->subtypes.end()){
        if((*sit)->name==type) return true;
        ++sit;
    }
    return false;
}

set<string> TypeConfig::get_subtypes(const string &cat)
{
    set<string> ret;
    map<string, TypeCategory*>::iterator it = categories.find(cat);
    if(it == categories.end()) {
        ret.insert(cat);
        return ret;
    }
    vector<TypeModel*>::iterator sit = it->second->subtypes.begin();
    while(sit!=it->second->subtypes.end()){
        ret.insert((*sit)->name);
        ++sit;
    }
    return ret;
}

const string TypeConfig::left_type(const string &rtype)
{
    map<string, TypeModel*>::iterator it = all_types.find(rtype);
    if (it == all_types.end()) return "";
    TypeModel *rel = it->second;
    if(rel->id > 128){
        return (dynamic_cast<TypeRelation*>(rel))->left->name;
    }
    return "";
}


const string TypeConfig::right_type(const string &rtype)
{
    map<string, TypeModel*>::iterator it = all_types.find(rtype);
    if (it == all_types.end()) return "";
    TypeModel *rel = it->second;
    if(rel->id > 128){
        return (dynamic_cast<TypeRelation*>(rel))->right->name;
    }
    return "";
}
