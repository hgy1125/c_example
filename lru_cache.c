#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//LRU的cache，实现初始化，添加，查找，删除功能
//lru使用双链表实现，最近最常使用的节点放在节点头，为了实现查找效率O(1)使用hash表

/* 结构体定义 */
typedef struct tagLRU_CACHE_NODE_S
{
    int key;
    int value;
    
    struct tagLRU_CACHE_NODE_S *hash_list_prev;
    struct tagLRU_CACHE_NODE_S *hash_list_next;

    struct tagLRU_CACHE_NODE_S *lru_list_prev;
    struct tagLRU_CACHE_NODE_S *lru_list_next;

}LRU_CACHE_NODE_S;

typedef struct tagLRU_CACHE_S
{
    LRU_CACHE_NODE_S **hash_map;
    int capacity; 

    LRU_CACHE_NODE_S *lru_list_head;   
    LRU_CACHE_NODE_S *lru_list_tail;  
    int list_size; 

}LRU_CACHE_S;

/* 这里定义全局变量，方便VS调试 */
LRU_CACHE_S *g_lru_cache;

/* lru_cache初始化接口 */
int lru_cache_init(int num)
{
    LRU_CACHE_S *cache = NULL;

    cache = (LRU_CACHE_S *)malloc(sizeof(LRU_CACHE_S));
    if (NULL == cache)
    {
        return -1;
    }
    memset(cache, 0, sizeof(LRU_CACHE_S));

    cache->hash_map = (LRU_CACHE_NODE_S **)malloc(sizeof(LRU_CACHE_NODE_S) * num);
    if (NULL == cache->hash_map) 
    {
        free(cache);
        return -1;
    }
    memset(cache->hash_map, 0, sizeof(LRU_CACHE_NODE_S) * num);

    cache->capacity = num;
    g_lru_cache = cache;

    return 0;
}

LRU_CACHE_NODE_S* alloc_cache_node(int key, int value)
{
    LRU_CACHE_NODE_S* node = NULL;

    node = (LRU_CACHE_NODE_S*)malloc(sizeof(LRU_CACHE_NODE_S));
    if (NULL == node) 
    {
        return NULL;
    }

    memset(node, 0, sizeof(LRU_CACHE_NODE_S));
    node->key = key;
    node->value = value;

    return node;
}

void free_cache_node(LRU_CACHE_NODE_S *node)
{
    if (NULL != node)
    {
        free(node);
    }
}

void free_list(LRU_CACHE_S* cache)
{
    if (0 != cache->list_size) 
    {
        LRU_CACHE_NODE_S *node = cache->lru_list_head;
        while(node) 
        {
            LRU_CACHE_NODE_S *temp = node->lru_list_next;
            free_cache_node(node);
            node = temp;
        }

        cache->list_size = 0;
    }
}

void lru_cache_destory(LRU_CACHE_S *cache)
{
    if (NULL != cache) 
    {
        if (NULL != cache->hash_map)
        {
            free(cache->hash_map);
        }

        free_list(cache);
        free(cache);
    }
}

/* 删除cache链表中的节点 */
void del_node_from_list(LRU_CACHE_S *cache, LRU_CACHE_NODE_S *node)
{
    if (0 == cache->list_size) 
    {
        return;
    }

    if ((node == cache->lru_list_head) && (node == cache->lru_list_tail)) 
    {
        cache->lru_list_head = NULL;
        cache->lru_list_tail = NULL;
    } 
    else if (node == cache->lru_list_head) 
    {
        cache->lru_list_head = node->lru_list_next;
        cache->lru_list_head->lru_list_prev = NULL;
    } 
    else if (node == cache->lru_list_tail) 
    {
        cache->lru_list_tail = node->lru_list_prev;
        cache->lru_list_tail->lru_list_next = NULL;
    } 
    else 
    {
        node->lru_list_prev->lru_list_next = node->lru_list_next;
        node->lru_list_next->lru_list_prev = node->lru_list_prev;
    }

    cache->list_size--;
}

/* 将节点插入到cache链表表头 */
LRU_CACHE_NODE_S* add_node_to_list_head(LRU_CACHE_S *cache, LRU_CACHE_NODE_S *node)
{
    LRU_CACHE_NODE_S *removed_node = NULL;

    if (++cache->list_size > cache->capacity) 
    {
        removed_node = cache->lru_list_tail;
        del_node_from_list(cache, cache->lru_list_tail);
    }

    if ((NULL == cache->lru_list_head) && (NULL == cache->lru_list_tail)) 
    {
        cache->lru_list_head = cache->lru_list_tail = node;
    } 
    else 
    {
        node->lru_list_next = cache->lru_list_head;
        node->lru_list_prev = NULL;
        cache->lru_list_head->lru_list_prev = node;
        cache->lru_list_head = node;
    }

    return removed_node;
}

void update_list_for_lru(LRU_CACHE_S *cache, LRU_CACHE_NODE_S *node)
{
    /* 先将节点从链表中摘除，再添加到链表头 */
    del_node_from_list(cache, node);
    add_node_to_list_head(cache, node);
}

/* 获取cache节点 */
LRU_CACHE_NODE_S *get_node_in_map(LRU_CACHE_S *cache, int key)
{
    LRU_CACHE_NODE_S *node = cache->hash_map[(key % cache->capacity)];

    for (; NULL != node; node = node->hash_list_next)
    {
        if (key == node->key)
        {
            return node;
        }
    }
    
    return NULL;
}

/* 插入cache节点 */
void put_node_in_map(LRU_CACHE_S *cache, LRU_CACHE_NODE_S *node)
{
    LRU_CACHE_NODE_S *temp_node = cache->hash_map[(node->key % cache->capacity)];
    if (NULL != temp_node)
    {
        node->hash_list_next = temp_node;
        temp_node->hash_list_prev = node;
    }

    cache->hash_map[(node->key % cache->capacity)] = node;
}

/* 删除cache节点 */
void del_node_in_map(LRU_CACHE_S *cache, LRU_CACHE_NODE_S *node)
{
    LRU_CACHE_NODE_S *temp_node = cache->hash_map[(node->key % cache->capacity)];

    for (; NULL != temp_node; temp_node = temp_node->hash_list_next)
    {
        if (temp_node->key == node->key)
        {
            if (temp_node->hash_list_prev)
            {
                temp_node->hash_list_prev->hash_list_next = temp_node->hash_list_next;
            } 
            else 
            {
                cache->hash_map[(node->key % cache->capacity)] = temp_node->hash_list_next;
            }

            if (temp_node->hash_list_next) 
            {
                temp_node->hash_list_next->hash_list_prev = temp_node->hash_list_prev;
            }

            return;
        }
    }
}

/* lru_cache添加数据和更新数据接口 */
int lru_cache_put(LRU_CACHE_S *cache, int key, int value)
{
    LRU_CACHE_NODE_S *node = NULL;

    node = get_node_in_map(cache, key);
    if (NULL != node)
    {   
        /* 节点存在，更新节点数据和lru */
        node->value = value;
        update_list_for_lru(cache, node);
    }
    else
    {
        node = alloc_cache_node(key, value);
        if (NULL == node)
        {
            return -1;
        }

        LRU_CACHE_NODE_S *temp_node = add_node_to_list_head(cache, node);
        if (NULL != temp_node) 
        {
            /* 删除淘汰节点 */
            del_node_in_map(cache, temp_node);
            free_cache_node(temp_node);
        }
   
        put_node_in_map(cache, node);
    }

    return 0;
}

/* lru_cache查询数据接口 */
int lru_cache_get(LRU_CACHE_S *cache, int key)
{
    LRU_CACHE_NODE_S* node = get_node_in_map(cache, key);
    if (NULL != node) 
    {
        update_list_for_lru(cache, node);
        return node->value;
    }
    
    return -1;
}

/* lru_cache删除数据接口 */
void lru_cache_del(LRU_CACHE_S *cache, int key)
{
    //从map中删除，从list中删除，再释放节点资源
    LRU_CACHE_NODE_S* node = get_node_in_map(cache, key);
    if (NULL != node) 
    {
        del_node_in_map(cache, node);
    }

    del_node_from_list(cache, node);

    free_cache_node(node);
    node = NULL;
}

void print_lru_cache_data(void)
{
    LRU_CACHE_NODE_S* node = g_lru_cache->lru_list_head;
    while (node) 
    {
        printf("(key:%d, value:%d)\n", node->key, node->value);
        node = node->lru_list_next;
    }

    printf("\n");
}

int main()
{
    int ret = lru_cache_init(5);

    //增加数据key:1,2,3,4,5
    lru_cache_put(g_lru_cache, 1, 6);
    lru_cache_put(g_lru_cache, 2, 7);
    lru_cache_put(g_lru_cache, 3, 8);
    lru_cache_put(g_lru_cache, 4, 9);
    lru_cache_put(g_lru_cache, 5, 10);
    
    print_lru_cache_data();

    //查询数据key:4
    int value = lru_cache_get(g_lru_cache, 4);

    print_lru_cache_data();

    //删除数据key:2
    lru_cache_del(g_lru_cache, 2);

    print_lru_cache_data();

    lru_cache_destory(g_lru_cache);

    getchar();

	return 0;
}

