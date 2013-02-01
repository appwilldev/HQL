/* -*- c++ -*-
 *
 * file: hql_xpc.hpp
 * author: KDr2
 *
 */

#ifndef HQL_XPC_H
#define HQL_XPC_H

#define PROJ_ID      0x70
#define SHM_PAGE_NUM 1024

#include <set>
#include <inttypes.h>
#include <pthread.h>

using std::set;

struct HQLXPCDelta
{
    enum Action{
        ADD = 0,
        DEL = 1,
        CLR = 2
    };

    int64_t      id;
    uint32_t     nprocess;
    uint32_t     ns;
    Action       action;
    char         data[512];
};

struct HQLXPCDeltaInfo
{
    int             flags;
    pthread_mutex_t lock;
    uint32_t        num;
    long            head;
    long            tail;
};

struct HQLXPCElement
{
    int ns;
    char data[512];
};

struct HQLXPCElementInfo
{
    uint32_t   num;
    long       head;
    long       tail;
};

class HQLXPController
{
public:

    static void setup();
    static void add_delta(HQLXPCDelta::Action, int64_t, int, int, const char*);
    static void check_delta();
    static int64_t get_id(){return ++cur_delta_id;};

    static bool enabled;
    static int64_t cur_delta_id;
    static set<int64_t> handled_ids;

    static int shm_id;
    static void *shm_start;
    static void *ele_start;

    static HQLXPCDeltaInfo *delta_info;
    static HQLXPCElementInfo *element_info;
};

#endif
