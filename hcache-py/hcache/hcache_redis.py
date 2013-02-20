#-*- coding:utf-8 -*-
# author : ldmiao

# HCache Engine: Redis
# easy_install redis

import re
import redis

import hcache_utils as utils
from hcache_capi import ORDER_ASC, ORDER_DESC


class CacheRedis(object):
    def get_client(self, host, port, db):
        pool = redis.ConnectionPool(host=host, port=port, db=db)
        return redis.Redis(connection_pool=pool)


    def __init__(self, host='localhost:6379/0'):
        host=host.split("?")
        pararmeters=""
        if len(host)>1:
            pararmeters=host[1]
        host=host[0]
        host=host.split(":")
        port=host[1].split("/")
        db = int(port[1]) if len(port)>1 else 0
        self.client = self.get_client(host=host[0], port=int(port[0]), db=db)
        self.batch_size=0
        bre=re.compile(".*?batch=(\d+).*")
        match=bre.match(pararmeters)
        if match:
            self.queued_size=0
            self.batch_size=int(match.group(1))
            self.client=self.client.pipeline()

    def close(self):
        if self.batch_size:
            self.client.execute()
        self.client.connection_pool.disconnect()


    #////////////////////////////////////////////////////////////////////////////////////////////
    # key - sorted list

    def slist_range(self, key, order_by=ORDER_ASC, count=25, start_index=-1, start_value=None):
        if not isinstance(key, (str, unicode, list)):
            return []

        count = utils.force_int(count, 25)
        if count<=0:
            return []

        start_index = utils.force_int(start_index, -1)

        if isinstance(key, list):
            key_count = len(key)
            if key_count<=0:
                return []
            elif key_count>1:
                return self.slist_merge_range(key, order_by, count, start_index, start_value)
            else:
                key = key[0]
            #endif
        #endif

        zrange_func = None
        if order_by==ORDER_ASC:
            zrange_func = self.client.zrange
        else:
            zrange_func = self.client.zrevrange
        #endif

        fn_list = None

        if start_index>=0:
            if start_value is None:
                fn_list = zrange_func(key, start_index, start_index+count-1)
            elif start_value is not None:
                offset = 10
                offset_start_index = start_index-offset if start_index-offset>0 else 0
                start_candidate_list = zrange_func(key, offset_start_index, offset_start_index + offset + count + offset)
                #endif
                if start_candidate_list and len(start_candidate_list)>0:
                    i = None
                    try:
                        start_value_36 = utils.to36(start_value)
                        i = start_candidate_list.index(start_value_36)
                    except:
                        i = None
                    #endtry

                    if i is not None:
                        if i+count<=len(start_candidate_list):
                            fn_list = start_candidate_list[i:i+count]
                        else:
                            new_start_index = offset_start_index + i
                            fn_list = zrange_func(key, new_start_index, new_start_index + count-1)
                        #endif
                    else:
                        fn_list = zrange_func(key, start_index, start_index + count-1)
                    #endif
                #endif
            #endif
        elif start_index<0:
            if start_value is None:
                fn_list = zrange_func(key, 0, count-1)
            elif start_value is not None:
                #///////////////////////////////////////////////////////////////////////////////////////
                # Find from start to end
                fn_list = None

                new_start_index = None
                i = 0
                step_count = 500
                start_value_36 = utils.to36(start_value)
                while True:
                    start_candidate_list = zrange_func(key, i, i + step_count-1)
                    if isinstance(start_candidate_list, list):
                        list_len = len(start_candidate_list)
                        if list_len>0:
                            try:
                                idx = start_candidate_list.index(start_value_36)
                                if count <= list_len-idx:
                                    fn_list = start_candidate_list[idx:idx+count]
                                    break
                                #endif

                                new_start_index = i + idx
                            except:
                                new_start_index = None

                            if new_start_index is not None:
                                break
                            #endif
                        else:
                            #empty list
                            break
                        #endif
                    else:
                        #not a list
                        break
                    #endif

                    i = i + step_count
                #endwhile

                if fn_list is None and new_start_index is not None:
                    fn_list = zrange_func(key, new_start_index, new_start_index + count-1)
                #endif
                #///////////////////////////////////////////////////////////////////////////////////////

            #endif
        #endif

        if fn_list is not None:
            new_list = [int(x,36) for x in fn_list]
            #print "\n", "*"*100
            #print "start_index:", start_index, "start_value:", start_value, "count:", count
            #print new_list
            #print "$"*100
            return new_list
        else:
            return []

    def slist_merge_range(self, keys, order_by=ORDER_ASC, count=25, start_index=-1, start_value=None):
        fn_score_dict = {}

        sub_count = 1000/len(keys)
        if sub_count<500: sub_count=50

        fn_score_list = None
        for key in keys:
            if order_by==ORDER_ASC:
                fn_score_list = self.client.zrange(key, 0, sub_count-1, withscores=True)
            else:
                fn_score_list = self.client.zrevrange(key, 0, sub_count-1, withscores=True)
            #endif

            if fn_score_list is None or len(fn_score_list)<=0: continue

            #print fn_score_list
            for fn_score in fn_score_list:
                fn, score = fn_score
                fn_score_dict[fn] = fn_score
            #endfor
        #endfor

        fn_score_list = [fn_score for fn_score in fn_score_dict.itervalues()]
        reverse = False if order_by==ORDER_ASC else True
        fn_score_list.sort(key=lambda x:x[1], reverse=reverse)
        fn_list = [x[0] for x in fn_score_list]

        result_list = []

        start_value_36 = utils.to36(start_value)
        if start_index>=0:
            if start_value is None:
                result_list = fn_list[start_index:start_index+count]
            else:
                try:
                    idx = fn_list.index(start_value_36)
                except:
                    idx = start_index
                result_list = fn_list[idx:idx+count]
            #endif
        else:
            if start_value is None:
                result_list = fn_list[:count]
            else:
                try:
                    idx = fn_list.index(start_value_36)
                except:
                    return []
                result_list = fn_list[idx:idx+count]
            #endif
        #endif

        new_list = [int(x,36) for x in result_list]
        return new_list

    def slist_add(self, key, value, score, limit=0, order_by=None):
        if not isinstance(score, (int, long, float)):
            return False

        value_36 = utils.to36(value)
        ret = self.client.zadd(key, value_36, score)

        # remove elements that exceeds the limit
        if limit>0:
            overflow = self.slist_len(key) - limit
            if overflow>0:
                if order_by==ORDER_ASC:
                    self.client.zremrangebyrank(key, -1*overflow, -1)
                else:
                    self.client.zremrangebyrank(key, 0, overflow)
                #endif
            #endif
        #endif
        #deal pipeline
        if self.batch_size:
            self.queued_size+=1
            if self.queued_size>self.batch_size:
                self.client.execute()
                self.queued_size=0
        return ret

    def slist_update(self, key, value, score, limit=0, order_by=None):
        return self.slist_add(key, value, score, limit, order_by)

    def slist_del_val(self, key, value):
        try:
            value_36 = utils.to36(value)
            ret = self.client.zrem(key, value_36)
            # deal pipeline
            if self.batch_size:
                self.queued_size+=1
                if self.queued_size>self.batch_size:
                    self.client.execute()
                    self.queued_size=0
            return ret
        except:
            return False

    def slist_len(self, key):
        try:
            ret = self.client.zcard(key)
            return ret
        except:
            return 0

    def slist_stat(self, key):
        try:
            ret = self.client.zcard(key)
            return ret
        except:
            return 0

    #///////////////////////////////////////////////////////////////////////////////
    # key - list

    def list_range(self, key, count=25, start_index=-1, start_value=None):
        if not isinstance(key, (str, unicode, list)):
            return []

        count = utils.force_int(count, 25)
        if count<=0:
            return []

        start_index = utils.force_int(start_index, -1)

        if isinstance(key, list):
            key_count = len(key)
            if key_count<=0:
                return []
            elif key_count>1:
                return self.list_merge_range(key, count, start_index, start_value)
            else:
                key = key[0]
            #endif
        #endif

        fn_list = None

        if start_index>=0:
            if start_value is None:
                fn_list = self.client.lrange(key, start_index, start_index+count-1)

            elif start_value is not None:
                offset = 10
                offset_start_index = start_index-offset if start_index-offset>0 else 0
                start_candidate_list = self.client.lrange(key, offset_start_index, offset_start_index + offset + count + offset)
                if start_candidate_list and len(start_candidate_list)>0:
                    i = None
                    try:
                        start_value_36 = utils.to36(start_value)
                        i = start_candidate_list.index(start_value_36)
                    except:
                        i = None
                    #endtry

                    if i is not None:
                        if i+count<=len(start_candidate_list):
                            fn_list = start_candidate_list[i:i+count]
                        else:
                            new_start_index = offset_start_index + i
                            fn_list = self.client.lrange(key, new_start_index, new_start_index + count-1)
                        #endif
                    else:
                        fn_list = self.client.lrange(key, start_index, start_index + count-1)
                    #endif
                #endif
            #endif
        elif start_index<0:
            if start_value is None:
                fn_list = self.client.lrange(key, 0, count-1)

            elif start_value is not None:
                #///////////////////////////////////////////////////////////////////////////////////////
                # Find from start to end
                new_start_index = None
                i = 0
                step_count = 500
                start_value_36 = utils.to36(start_value)
                while True:
                    start_candidate_list = self.client.lrange(key, i, i + step_count-1)
                    if start_candidate_list and len(start_candidate_list)>0:
                        try:
                            new_start_index = start_candidate_list.index(start_value_36)
                            new_start_index = i + new_start_index
                        except:
                            new_start_index = None

                        if new_start_index is not None:
                            break
                        #endif
                    else:
                        break
                    #endif

                    i = i + step_count
                #endwhile

                if new_start_index is not None:
                    fn_list = self.client.lrange(key, new_start_index, new_start_index + count-1)
                #endif
                #///////////////////////////////////////////////////////////////////////////////////////



                #///////////////////////////////////////////////////////////////////////////////////////
                # Smart Find

                '''
                new_start_index = None
                i = 0

                step_count = count/2 + count%2
                if step_count>8:
                    step_count = 8
                #endif

                start_value_36 = utils.to36(start_value)
                while True:
                    i = i-step_count if i-step_count>0 else 0
                    start_candidate_list = self.client.lrange(key, i, i + step_count*2 - 1)
                    if start_candidate_list and len(start_candidate_list)>0:
                        try:
                            new_start_index = start_candidate_list.index(start_value_36)
                            new_start_index = i + new_start_index
                        except:
                            new_start_index = None

                        if new_start_index is not None:
                            break
                        #endif
                    else:
                        break
                    #endif

                    i = i + count
                #endwhile

                if new_start_index is not None:
                    fn_list = self.client.lrange(key, new_start_index, new_start_index + count-1)
                #endif
                '''
                #///////////////////////////////////////////////////////////////////////////////////////

            #endif
        #endif

        if fn_list is not None:
            new_list = [int(x,36) for x in fn_list]
            #print "\n", "*"*100
            #print "start_index:", start_index, "start_value:", start_value, "count:", count
            #print new_list
            #print "$"*100
            return new_list
        else:
            return []

    def list_merge_range(self, keys, count=25, start_index=-1, start_value=None):
        fn_set = set()

        key_count = len(keys)
        sub_count = 1000/key_count + 50

        fn_list = None
        for key in keys:
            fn_list = self.client.lrange(key, 0, sub_count-1)
            if fn_list:
                for fn in fn_list:
                    fn_set.add(int(fn, 36))
                #endfor
            else:
                pass
        #endfor

        fn_list = list(fn_set)
        fn_list.sort(key=lambda x:x, reverse=True)

        result_list = []

        if start_index>=0:
            if start_value is None:
                result_list = fn_list[start_index:start_index+count]
            else:
                try:
                    idx = fn_list.index(start_value)
                except:
                    idx = start_index
                result_list = fn_list[idx:idx+count]
            #endif
        else:
            if start_value is None:
                result_list = fn_list[:count]
            else:
                try:
                    idx = fn_list.index(start_value)
                except:
                    return []

                result_list = fn_list[idx:idx+count]
            #endif
        #endif

        return result_list

    def list_pop(self, key):
        try:
            ret = self.client.lpop(key)
            # deal pipeline
            if self.batch_size:
                self.queued_size+=1
                if self.queued_size>self.batch_size:
                    self.client.execute()
                    self.queued_size=0
            if ret is not None:
                return int(ret, 36)
        except:
            return None

    def list_push(self, key, value, limit=0):
        new_value = utils.to36(value)
        ret = self.client.lpush(key, new_value)

        # remove elements that exceeds the limit
        if limit>0 and self.list_len(key)-limit>0:
            self.client.ltrim(key, 0, limit)
        #endif
        # deal pipeline
        if self.batch_size:
            self.queued_size+=1
            if self.queued_size>self.batch_size:
                self.client.execute()
                self.queued_size=0
        return ret

    def list_push_multi(self, key, values, limit=0):
        if limit>0 and len(values)>limit:
            values = values[:limit]
        #endif

        new_values = [utils.to36(x) for x in values]
        ret = self.client.lpush(key, *new_values)

        # remove elements that exceeds the limit
        if limit>0 and self.list_len(key)-limit>0:
            self.client.ltrim(key, 0, limit)
        #endif
        # deal pipeline
        if self.batch_size:
            self.queued_size+=1
            if self.queued_size>self.batch_size:
                self.client.execute()
                self.queued_size=0
        return ret

    def list_rpop(self, key):
        try:
            ret = self.client.rpop(key)
            # deal pipeline
            if self.batch_size:
                self.queued_size+=1
                if self.queued_size>self.batch_size:
                    self.client.execute()
                    self.queued_size=0
            if ret is not None:
                return int(ret, 36)
        except:
            return None

    def list_rpush(self, key, value, limit=0):
        new_value = utils.to36(value)
        ret = self.client.rpush(key, new_value)

        # remove elements that exceeds the limit
        if limit>0 and self.list_len(key)-limit>0:
            self.client.ltrim(key, -1*limit, -1)
        #endif
        # deal pipeline
        if self.batch_size:
            self.queued_size+=1
            if self.queued_size>self.batch_size:
                self.client.execute()
                self.queued_size=0
        return ret

    def list_rpush_multi(self, key, values, limit=0):
        if limit>0 and len(values)>limit:
            values = values[:limit]
        #endif

        new_values = [utils.to36(x) for x in values]
        ret = self.client.rpush(key, *new_values)

        # remove elements that exceeds the limit
        if limit>0 and self.list_len(key)-limit>0:
            self.client.ltrim(key, -1*limit, -1)
        #endif
        # deal pipeline
        if self.batch_size:
            self.queued_size+=1
            if self.queued_size>self.batch_size:
                self.client.execute()
                self.queued_size=0
        return ret

    def list_insert(self, key, index, value_to_insert, limit=0):
        index = utils.force_int(index, 0)

        value_36 = self.client.lindex(key, index)
        if value_36:
            try:
                new_value_36_to_insert = utils.to36(value_to_insert)
                ret = self.client.linsert(key, 'before', value_36, new_value_36_to_insert)
                # deal pipeline
                if self.batch_size:
                    self.queued_size+=1
                    if self.queued_size>self.batch_size:
                        self.client.execute()
                        self.queued_size=0
                return ret
            except:
                return False
        else:
            return False

    def list_insert_before(self, key, value, value_to_insert, limit=0):
        try:
            new_value = utils.to36(value)
            new_value_to_insert = utils.to36(value_to_insert)
            ret = self.client.linsert(key, 'before', new_value, new_value_to_insert)
            # deal pipeline
            if self.batch_size:
                self.queued_size+=1
                if self.queued_size>self.batch_size:
                    self.client.execute()
                    self.queued_size=0
            return ret
        except:
            return False

    def list_insert_after(self, key, value, value_to_insert, limit=0):
        try:
            new_value = utils.to36(value)
            new_value_to_insert = utils.to36(value_to_insert)
            ret = self.client.linsert(key, 'after', new_value, new_value_to_insert)
            # deal pipeline
            if self.batch_size:
                self.queued_size+=1
                if self.queued_size>self.batch_size:
                    self.client.execute()
                    self.queued_size=0
            return ret
        except:
            return False

    def list_set(self, key, index, value):
        index = utils.force_int(index, None)
        if index is None:
            return False

        try:
            new_value = utils.to36(value)
            ret = self.client.lset(key, index, new_value)
            # deal pipeline
            if self.batch_size:
                self.queued_size+=1
                if self.queued_size>self.batch_size:
                    self.client.execute()
                    self.queued_size=0
            return ret
        except:
            return False

    def list_del_val(self, key, value):
        try:
            new_value = utils.to36(value)
            ret = self.client.lrem(key, new_value, 0)
            # deal pipeline
            if self.batch_size:
                self.queued_size+=1
                if self.queued_size>self.batch_size:
                    self.client.execute()
                    self.queued_size=0
            return ret
        except:
            return False

    def list_index(self, key, index=None):
        index = utils.force_int(index, None)
        if index is None:
            return None

        try:
            value = self.client.lindex(key, index)
            if value:
                return int(value, 36)
            #endif
            return None
        except:
            return None

    def list_clear(self, key):
        return self.client.delete(key)

    def list_destroy(self, key):
        return self.client.delete(key)

    def list_len(self, key):
        try:
            ret = self.client.llen(key)
            return ret
        except:
            return 0

    def list_stat(self, key):
        try:
            ret = self.client.llen(key)
            return ret
        except:
            return 0

    #////////////////////////////////////////////////////////////////////////////////////////
    # key - value

    def kv_set(self, key, value, *args, **kwargs):
        return self.client.set(key, value)

    def kv_get(self, key,  *args, **kwargs):
        return self.client.get(key)

    def kv_del(self, key,  *args, **kwargs):
        return self.client.delete(key)

    def kv_set_multi(self, kv_dict, prefix='', *args, **kwargs):
        new_kv_dict = {}
        for k,v in kv_dict.iteritems():
            new_kv_dict[str(prefix)+str(k)] = v
        return self.client.mset(new_kv_dict)

    def kv_get_multi(self, keys, prefix='', *args, **kwargs):
        new_keys = [str(prefix)+str(x) for x in keys]
        return self.client.mget(new_keys)

    def kv_del_multi(self, keys, prefix='', *args, **kwargs):
        new_keys = [str(prefix)+str(x) for x in keys]
        return self.client.delete(*new_keys)


    def bgsave(self):
        return self.client.bgsave()

if __name__ == "__main__":
    cache = CacheRedis(host='localhost:6379/0')
    '''
    print cache.list_rpush("foo", 1)
    print cache.list_rpush("foo", 2)
    print cache.list_rpush("foo", 3)
    print cache.list_rpush("foo", 4)
    print cache.list_rpush("foo", 5)

    print cache.list_range("foo", 1, 0)
    print cache.list_range("foo", 2, 0)
    print cache.list_range("foo", 3, 0)
    print cache.list_range("foo", 4, 0)
    print cache.list_range("foo", 5, 0)
    print cache.list_range("foo", 6, 0)

    print cache.list_push_multi("foo", [6,7,8,9])
    print cache.list_rpush_multi("foo", [6,7,8,9])

    print cache.list_del_val("foo", 9)

    print "insert 3 lines:"
    print cache.list_insert("foo", 0, 111)
    print cache.list_insert_before("foo", 9, 0)
    print cache.list_insert_after("foo", 9, 10)

    print cache.list_push_multi("foo", [6,7,8,8,9])

    print cache.list_range("foo", cache.list_len("foo"), 0)

    print cache.list_set("foo", 0, 100)
    print cache.list_set("foo", 10, 110)

    print cache.list_range("foo", cache.list_len("foo"), 0)
    print cache.list_del_val("foo", 8)
    print cache.list_range("foo", cache.list_len("foo"), 0)

    print cache.list_pop("foo")
    print cache.list_range("foo", cache.list_len("foo"), 0)

    print cache.list_pop("foo")
    print cache.list_range("foo", cache.list_len("foo"), 0)

    print cache.list_pop("foo")
    print cache.list_range("foo", cache.list_len("foo"), 0)

    print cache.list_rpop("foo")
    print cache.list_range("foo", cache.list_len("foo"), 0)

    print cache.list_rpop("foo")
    print cache.list_range("foo", cache.list_len("foo"), 0)

    print cache.list_rpop("foo")
    print cache.list_range("foo", cache.list_len("foo"), 0)


    print cache.list_len("foo")

    print cache.kv_set("k1", "v1")
    print cache.kv_set("k2", "v2")
    print cache.kv_set("k3", "v3")

    print cache.kv_get("k1")
    print cache.kv_get("k2")
    print cache.kv_get("k3")

    print cache.kv_set_multi({"k4":"v4", "k5":"v5", "k6":"v6"})
    print cache.kv_get_multi(["k1", "k2", "k3", "k4", "k5", "k6"])

    print cache.kv_del("k1")
    print cache.kv_del_multi(["k1", "k2", "k3"])
    print cache.bgsave()
    print cache.kv_get_multi(["k1", "k2", "k3", "k4", "k5", "k6"])
    '''

    '''
    print cache.slist_add('sl1', 1, 1)
    print cache.slist_add('sl1', 2, 2)
    print cache.slist_add('sl1', 10, 10)
    print cache.slist_add('sl1', 9, 9)
    print cache.slist_add('sl1', 0, 0)
    print cache.slist_add('sl1', 20, 20)
    print cache.slist_add('sl1', 7, 7)

    print cache.slist_range('sl1', ORDER_DESC, 25, 0)
    print cache.slist_len('sl1')

    print cache.slist_del_val('sl1', 0)
    print cache.slist_range('sl1', ORDER_DESC, 25, 0)
    print cache.slist_len('sl1')

    print cache.slist_del_val('sl1', 7)
    print cache.slist_range('sl1', ORDER_DESC, 25, 0)
    print cache.slist_len('sl1')

    print cache.slist_del_val('sl1', 1)
    print cache.slist_range('sl1', ORDER_DESC, 25, 0)
    print cache.slist_len('sl1')

    '''

    print cache.slist_add('sl1', 1, 1)
    print cache.slist_add('sl1', 2, 2)
    print cache.slist_add('sl1', 10, 10)
    print cache.slist_add('sl1', 9, 9)
    print cache.slist_add('sl1', 0, 0)
    print cache.slist_add('sl1', 20, 20)
    print cache.slist_add('sl1', 7, 7)

    print cache.slist_merge_range(['sl1', 'sl2'], ORDER_DESC, 25, 0)
