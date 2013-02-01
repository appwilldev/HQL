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


void HQLXPController::setup()
{
    char key_path[64];
    pid_t ppid = getppid();
    struct stat kf_stat;

    int klen = sprintf(key_path, "/tmp/HQL_SHMKEY_%d", 0);//ppid);
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
    shm_id = shmget(key, SHM_PAGE_NUM * 4096, IPC_CREAT|0700);
    if(shm_id == -1){
        enabled = false;
        puts("can not create hql shm!");
        return;
    }
    printf("kf=%s,%x,%d\n",key_path,key,shm_id);
    shm_start = shmat(shm_id, NULL, 0);
    if(shm_start == (void*)-1){
        enabled = false;
        perror("can not attach hql shm!");
        return;
    }
    enabled = true;

    cur_delta_id = static_cast<int64_t>(getpid()) << 32;

    delta_info = static_cast<HQLXPCDeltaInfo*>(shm_start);
    ele_start = (char*)shm_start + SHM_PAGE_NUM*4096/2;
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
        HQLXPCElement *e = static_cast<HQLXPCElement*>(
            (void*)((char*)ele_start + sizeof(HQLXPCElementInfo) + element_info->head));
        HQLXPCElement *stop = static_cast<HQLXPCElement*>(
            (void*)((char*)ele_start + sizeof(HQLXPCElementInfo) + element_info->tail));
        pthread_mutex_lock(&delta_info->lock);
        while(e != stop){
            HQLNode *n = ASTUtil::parser_hql(e->data, true);
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

    if(delta_info->head == 0 && delta_info->tail == 0){
        delta = static_cast<HQLXPCDelta*>(
            static_cast<void*>((char*)shm_start + sizeof(HQLXPCDeltaInfo)));
        delta_info->head = 0;
        delta_info->tail = sizeof(HQLXPCDelta);
    }else{
        delta = static_cast<HQLXPCDelta*>(
            static_cast<void*>(
                (char*)(shm_start) + (delta_info->tail) + sizeof(HQLXPCDeltaInfo)));
        delta_info->tail += sizeof(HQLXPCDelta);
    }

    delta->id = id;
    delta->nprocess = nprocess;
    delta->ns = ns;
    delta->action = act;
    if(act==HQLXPCDelta::ADD || act==HQLXPCDelta::DEL){
        memset(delta->data, 0, 512);
        memcpy(delta->data, hql, strlen(hql));
        printf("ADD DELTA:%s\n", delta->data);
    }
    ++delta_info->num;
    handled_ids.insert(id);

    //fill the shm dynamic elements
    switch(delta->action){
    case HQLXPCDelta::ADD:
        {
            //TODO
            break;
         }
    case HQLXPCDelta::DEL:
        {
            //TODO
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
    HQLXPCDelta *delta = (HQLXPCDelta*)((char*)shm_start + sizeof(HQLXPCDeltaInfo) + delta_info->head);
    HQLXPCDelta *stop = (HQLXPCDelta*)((char*)shm_start + sizeof(HQLXPCDeltaInfo) + delta_info->tail);
    struct shmid_ds shm_stat;

    while(delta != stop){
        printf("check delta\n");
        if(handled_ids.find(delta->id) == handled_ids.end()) { // not in set
            printf("--->1:%p - %p - %p\n", shm_start, delta_info, delta);
            printf("!!!DLD:%s\n", delta->data);
            printf("----2\n");
            HQLNode *n = ASTUtil::parser_hql(delta->data, true);
            if(n){
                TrollersHolder::register_troller(n, (uint8_t)(delta->ns), false);
                delete n;
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
