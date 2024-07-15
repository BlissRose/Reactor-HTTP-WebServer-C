#include "ChannelMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
这个文件是做一个映射
*/

// 初始化
struct ChannelMap *channelMapInit(int size)
{
    struct ChannelMap *map = (struct ChannelMap *)malloc(sizeof(struct ChannelMap));
    map->size = size;
    map->list = (struct Channel **)malloc(size * sizeof(struct Channel *));
    return map;
}
// 清空map
void ChannelMapClear(struct ChannelMap *map)
{
    if (map != NULL)
    {
        for (int i = 0; i < map->size; ++i)
        {
            if (map->list[i] != NULL)
            {
                free(map->list[i]);
            }
        }
        free(map->list);
        map->list = NULL;
    }
    map->size = 0;
}
// 重新分配内存空间
bool makeMapRoom(struct ChannelMap *map, int newSize, int unitSize)
{
    if (map->size < newSize)
    {
        int curSize = map->size;
        // 容量每次扩大原来的一倍
        while (curSize < newSize)
        {
            curSize *= 2;
        }
        // 扩容 realloc
        struct Channel **temp = realloc(map->list, curSize * unitSize);
        if (temp == NULL)
        {
            return false;
        }
        map->list = temp;
        memset(&map->list[map->size], 0, (curSize - map->size) * unitSize);
        map->size = curSize;
    }
    return true;
}