/* -*- c++ -*-
 *
 * file: hql_xpc.cpp
 * author: KDr2
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "hql_xpc.hpp"
#include "trollers.hpp"
#include "ast_util.hpp"

bool HQLXPController::enabled;
int64_t HQLXPController::cur_delta_id;
set<int64_t> HQLXPController::handled_ids;

int HQLXPController::shm_id;
void *HQLXPController::shm_start;
void *HQLXPController::ele_start;
HQLXPCDeltaInfo *HQLXPController::delta_info;
HQLXPCElementInfo *HQLXPController::element_info;

#define SHM_SIZE (SHM_PAGE_NUM * 4096)

#define XPC_DELTA_HEAD ((HQLXPCDelta*)((char*)shm_start + sizeof(HQLXPCDeltaInfo) + delta_info->head))
#define XPC_DELTA_TAIL ((HQLXPCDelta*)((char*)shm_start + sizeof(HQLXPCDeltaInfo) + delta_info->tail))

#define XPC_ELE_HEAD  ((HQLXPCElement*)((char*)ele_start + sizeof(HQLXPCElementInfo) + element_info->head))
#define XPC_ELE_TAIL  ((HQLXPCElement*)((char*)ele_start + sizeof(HQLXPCElementInfo) + element_info->tail))

void HQLXPController::setup()
{
    char key_path[64];
    pid_t ppid = getppid();
    struct stat kf_stat;

    int klen = sprintf(key_path, "/tmp/HQL_SHMKEY_%d", ppid);
    key_path[klen] = 0;
    if(stat(key_path, &kf_stat)){
        if(errno == ENOENT){
            int fd = open(key_path, O_WRONLY|O_CREAT, 0755);
            if(fd<0){
                enabled = false;
                puts("can not touch a key file!");
                return;
            }else{
                close(fd);
            }
        }
    }

    key_t key = ftok(key_path, PROJ_ID);
    shm_id = shmget(key, SHM_SIZE, IPC_CREAT|0640);
    if(shm_id == -1){
        enabled = false;
        puts("can not create hql shm!");
        return;
    }
    fprintf(stderr, "HQL SHM INFO: KPATH=%s, KEY=0x%x, ID=%d\n",
        key_path, key, shm_id);
    shm_start = shmat(shm_id, NULL, 0);
    if(shm_start == (void*)-1){
        enabled = false;
        perror("can not attach hql shm!");
        return;
    }
    enabled = true;

    cur_delta_id = static_cast<int64_t>(getpid()) << 32;

    delta_info = static_cast<HQLXPCDeltaInfo*>(shm_start);
    ele_start = (char*)shm_start + SHM_SIZE/2;
    element_info = static_cast<HQLXPCElementInfo*>(ele_start);

    struct shmid_ds shm_stat;
    shmctl(shm_id, IPC_STAT, &shm_stat);

    if(shm_stat.shm_cpid == getpid()){
        //init memory
        element_info->num = 0;
        element_info->head = 0;
        element_info->tail = 0;

        pthread_mutex_init(&delta_info->lock, NULL);

        delta_info->num = 0;
        delta_info->head = 0;
        delta_info->tail = 0;
        delta_info->flags |= 0x01;

    }else if((delta_info->flags & 0x01) == 0x01){
        HQLXPCElement *e = XPC_ELE_HEAD;
        HQLXPCElement *stop = XPC_ELE_TAIL;
        pthread_mutex_lock(&delta_info->lock);
        while(e != stop){
            HQLNode *n = ASTUtil::parser_hql(e->data);
            if(n){
                TrollersHolder::register_troller(n, (uint8_t)(e->ns), false);
                delete n;
            }
            ++e;
        }
        pthread_mutex_unlock(&delta_info->lock);
    }

}

void HQLXPController::add_delta(
    HQLXPCDelta::Action act,
    int64_t id,
    int nprocess,
    int ns,
    const char *hql)
{
    HQLXPCDelta *delta;

    pthread_mutex_lock(&delta_info->lock);

    delta = XPC_DELTA_TAIL;

    if(XPC_DELTA_TAIL + 1 > ele_start){
        if(delta_info->head == 0){
            fprintf(stderr, "No more Delta Sapce\n");
            return;
        }
        long all_delta_size = delta_info->tail - delta_info->head;
        memmove(
            (char*)shm_start + sizeof(HQLXPCDeltaInfo),
            XPC_DELTA_HEAD, all_delta_size);
        delta_info->head = 0;
        delta_info->tail = all_delta_size;
        delta = XPC_DELTA_TAIL;
    }

    delta_info->tail += sizeof(HQLXPCDelta);

    delta->id = id;
    delta->nprocess = nprocess;
    delta->ns = ns;
    delta->action = act;
    if(act==HQLXPCDelta::ADD || act==HQLXPCDelta::DEL){
        memset(delta->data, 0, 512);
        memcpy(delta->data, hql, strlen(hql));
    }
    ++delta_info->num;
    handled_ids.insert(id);

    //fill the shm dynamic elements
    switch(delta->action){
    case HQLXPCDelta::ADD:
        {
            HQLXPCElement *e = XPC_ELE_HEAD;
            HQLXPCElement *stop = XPC_ELE_TAIL;
            while(e != stop){
                if(strcmp(e->data, delta->data)==0){ break; }
                ++e;
            }//end while
            if(e == stop){ // add a new element
                if(XPC_ELE_TAIL + 1 > (void*)((char*)shm_start + SHM_SIZE)){
                    fprintf(stderr, "No more Element Sapce\n");
                    return;
                }
                memset(e->data, 0, 512);
                memcpy(e->data, hql, strlen(hql));
                e->ns = ns;
                element_info->tail += sizeof(HQLXPCElement);
                ++element_info->num;
            }
            break;
        }
    case HQLXPCDelta::DEL:
        {
            HQLXPCElement *e = XPC_ELE_HEAD;
            HQLXPCElement *stop = XPC_ELE_TAIL;
            while(e != stop){
                if(strcmp(e->data, delta->data)==0){
                    memmove(
                        e,
                        e + sizeof(HQLXPCElement),
                        XPC_ELE_TAIL - e - sizeof(HQLXPCElement));
                    element_info->tail -= sizeof(HQLXPCElement);
                    stop = XPC_ELE_TAIL;
                }else{
                    ++e;
                }
            }//end while
            break;
        }
    case HQLXPCDelta::CLR:
        {
            element_info->num = 0;
            element_info->head = 0;
            element_info->tail = 0;
            break;
        }
    default:
        break;
    }

    pthread_mutex_unlock(&delta_info->lock);
}


void HQLXPController::check_delta()
{
    if(delta_info->num <= 0) return;
    HQLXPCDelta *delta = XPC_DELTA_HEAD;
    HQLXPCDelta *stop = XPC_DELTA_TAIL;
    struct shmid_ds shm_stat;

    while(delta != stop){
        if(handled_ids.find(delta->id) == handled_ids.end()) { // not in set
            switch(delta->action){
            case HQLXPCDelta::ADD:
                {
                    HQLNode *n = ASTUtil::parser_hql(delta->data);
                    if(n){
                        TrollersHolder::register_troller(n, (uint8_t)(delta->ns), false);
                        delete n;
                    }
                    break;
                }
            case HQLXPCDelta::DEL:
                {
                    HQLNode *n = ASTUtil::parser_hql(delta->data);
                    if(n){
                        TrollersHolder::unregister_troller(n, (uint8_t)(delta->ns), false);
                        delete n;
                    }
                    break;
                }
            case HQLXPCDelta::CLR:
                {
                    TrollersHolder::clear_trollers((uint8_t)(delta->ns), false);
                    break;
                }
            default:
                break;
            }
            handled_ids.insert(delta->id);
            ++(delta->nprocess);
            shmctl(shm_id, IPC_STAT, &shm_stat);
            if(delta->nprocess >= shm_stat.shm_nattch){
                delta_info->head += sizeof(HQLXPCDelta);
                --(delta_info->num);
            }
        }
        ++delta;
    }
}
