/* -*- c++ -*-
 *
 * file: concrete_ast.hpp
 * author: KDr2
 *
 */

#ifndef _CONCRETE_AST_HPP
#define _CONCRETE_AST_HPP

#include "ast.hpp"


class MatchedNode : public HQLNode{
public:
    MatchedNode(const string&, HQLNode::TARGET_TYPE tt=IMPLICIT);
    MatchedNode(const MatchedNode&);
    const MatchedNode& operator=(const MatchedNode&);
    virtual ~MatchedNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const {return set<string>();};
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const {return true;};
    virtual bool has_semantic_each() const {return false;};
    virtual vector<string> has_semantic_fk() const {return vector<string>();};
};


/*
 * Concrete(Derived) ASTNode
 * ERR, EACH, FULLNAME
 * SL:
 *    1. SL_ALL
 *    2. SL_COND
 * SL_FK:
 *    1. SL_FK_ALL
 *    2. SL_FK_COND
 * RL: NORMAL
 * LOGIC: AND/OR/NOT
 * MISC: ORDER_BY/LIMIT
 *
 */


class ErrorNode : public HQLNode{
public:
    ErrorNode(const string&);
    ErrorNode(const ErrorNode&);
    const ErrorNode& operator=(const ErrorNode&);
    virtual ~ErrorNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const {return set<string>();};
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const {return true;};
    virtual bool has_semantic_each() const {return false;};
    virtual vector<string> has_semantic_fk() const {return vector<string>();};
};

class EachNode : public HQLNode{
public:
    EachNode(const string&);
    EachNode(const EachNode&);
    const EachNode& operator=(const EachNode&);
    virtual ~EachNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const {return set<string>();};
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const {return true;};
    virtual vector<string> has_semantic_fk() const {return vector<string>();};
};

class FullnameNode : public HQLNode{
public:
    FullnameNode(const uint64_t);
    FullnameNode(const FullnameNode&);
    const FullnameNode& operator=(const FullnameNode&);
    virtual ~FullnameNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const {return set<string>();};
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const {return false;};
    virtual vector<string> has_semantic_fk() const {return vector<string>();};
};



class SLAllNode : public HQLNode{
public:
    SLAllNode(const string&, TARGET_TYPE=IMPLICIT);
    SLAllNode(const SLAllNode&);
    const SLAllNode& operator=(const SLAllNode&);
    virtual ~SLAllNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const {return false;};
    virtual vector<string> has_semantic_fk() const {return vector<string>();};
};


class SLCondNode : public HQLNode{
public:
    SLCondNode(const string&,
               const string&,
               const HQLOperand&,
               const HQLOperand&,
               TARGET_TYPE=IMPLICIT);
    SLCondNode(const SLCondNode&);
    const SLCondNode& operator=(const SLCondNode&);
    virtual ~SLCondNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual pair<string, uint64_t> time_in() const;
    virtual vector<string> has_semantic_fk() const {return vector<string>();};
};



class SLFKAllNode : public HQLNode{
public:
    SLFKAllNode(const string&,
                const string&,
                const string&,
                TARGET_TYPE=IMPLICIT);
    SLFKAllNode(const SLFKAllNode&);
    const SLFKAllNode& operator=(const SLFKAllNode&);
    virtual ~SLFKAllNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const {return false;};
    virtual vector<string> has_semantic_fk() const;
};



class SLFKCondNode : public HQLNode{
public:
    SLFKCondNode(const string&,
                 const string&,
                 const string&,
                 const string&,
                 const HQLOperand&,
                 const HQLOperand&,
                 TARGET_TYPE=IMPLICIT);
    SLFKCondNode(const SLFKCondNode&);
    const SLFKCondNode& operator=(const SLFKCondNode&);
    virtual ~SLFKCondNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const {return set<string>();};
    virtual ExtraMatchDataInfo get_xmdinfo() const {return ExtraMatchDataInfo();};
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual pair<string, uint64_t> time_in() const;
    virtual vector<string> has_semantic_fk() const;
};



class RLNode : public HQLNode{
public:
    RLNode(const HQLOperand&,
           const HQLOperand&,
           const HQLOperand&,
           TARGET_TYPE=IMPLICIT);
    RLNode(const RLNode&);
    const RLNode& operator=(const RLNode&);
    virtual ~RLNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const;
    virtual ExtraMatchDataInfo get_xmdinfo() const;
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual vector<string> has_semantic_fk() const {return vector<string>();};
};



class LogicAndNode : public HQLNode{
public:
    LogicAndNode(shared_ptr<HQLNode>,
                 shared_ptr<HQLNode>,
                 TARGET_TYPE=IMPLICIT);
    LogicAndNode(vector<HQLOperand>,
                 TARGET_TYPE=IMPLICIT);
    LogicAndNode(vector<shared_ptr<HQLNode> >,
                 TARGET_TYPE=IMPLICIT);
    LogicAndNode(const LogicAndNode&);
    const LogicAndNode& operator=(const LogicAndNode&);
    virtual ~LogicAndNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const;
    virtual ExtraMatchDataInfo get_xmdinfo() const;
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual pair<string, uint64_t> time_in() const;
    virtual vector<string> has_semantic_fk() const;
};



class LogicOrNode : public HQLNode{
public:
    LogicOrNode(shared_ptr<HQLNode>,
                 shared_ptr<HQLNode>,
                 TARGET_TYPE=IMPLICIT);
    LogicOrNode(const LogicOrNode&);
    const LogicOrNode& operator=(const LogicOrNode&);
    virtual ~LogicOrNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const;
    virtual ExtraMatchDataInfo get_xmdinfo() const;
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual pair<string, uint64_t> time_in() const;
    virtual vector<string> has_semantic_fk() const;
};


class LogicNotNode : public HQLNode{
public:
    LogicNotNode(shared_ptr<HQLNode>);
    LogicNotNode(const LogicNotNode&);
    const LogicNotNode& operator=(const LogicNotNode&);
    virtual ~LogicNotNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const;
    virtual ExtraMatchDataInfo get_xmdinfo() const;
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual pair<string, uint64_t> time_in() const;
    virtual vector<string> has_semantic_fk() const;
};


class MiscOrderByNode : public HQLNode{
public:
    MiscOrderByNode(shared_ptr<HQLNode>,
                    const string&,
                    const HQLOperand&);
    MiscOrderByNode(const MiscOrderByNode&);
    const MiscOrderByNode& operator=(const MiscOrderByNode&);
    virtual ~MiscOrderByNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const;
    virtual ExtraMatchDataInfo get_xmdinfo() const;
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual pair<string, uint64_t> time_in() const;
    virtual vector<string> has_semantic_fk() const;
};


class MiscLimitNode : public HQLNode{
public:
    MiscLimitNode(shared_ptr<HQLNode>,
                  const HQLOperand&);
    MiscLimitNode(const MiscLimitNode&);
    const MiscLimitNode& operator=(const MiscLimitNode&);
    virtual ~MiscLimitNode();
    virtual HQLNode* reduce();
    virtual HQLNode* copy();
    virtual const string to_hql() const;
    virtual const string cache_key(bool do_result_reduce=false) const;
    virtual const set<string> get_ctypes() const;
    virtual const set<string> get_rtypes() const;
    virtual ExtraMatchDataInfo get_xmdinfo() const;
    virtual const map<uint64_t, set<string> > match(const Model&, ModelGetter*);
    virtual bool validate() const;
    virtual bool has_semantic_each() const;
    virtual pair<string, uint64_t> time_in() const;
    virtual vector<string> has_semantic_fk() const;
};


#endif
