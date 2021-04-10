#ifndef __PROTOCOAL_H__
#define __PROTOCOAL_H__

#include "site_type.h"

#define DEFAULT_MGROUP      "224.2.2.2"                         // 多播地址
#define DEFUULT_RCVPORT     "2020"                              // 多播端口

#define CHANUM              100                                 // 频道数
#define LISTCHNID           0                                   // 节目单专频
#define MINCHNID            1
#define MAXCHNID            (MINCHNID+CHANUM-1)

#define MSG_CHANNEL_MAX     (65536-20-8)                        // 最大数据包长度(最大长度-ip报头-udp报头)
#define MAX_DATA            (MSG_CHANNEL_MAX-sizeof(chnid_t))   // 数据字段最大值

#define MSG_LIST_MAX        (65536-20-8)
#define MAX_ENTRY           (MSG_LIST_MAX-sizeof(chind_t))

struct msg_channel_st
{       
    chnid_t chnid;                                              // must between [MINCHID,MAXCHNID]
    uint8_t data[1];
}__attribute__((packed));                                       // 不需要内存对齐（每个平台对齐方式不同）

/**
 * 含义：单个频道信息
 * 格式：1 music:xxxx
*/
struct msg_listentry_st
{
    chnid_t chnid;
    uint16_t len;                                               // 变长结构体，标明其长度
    uint8_t desc[1];                                            // 频道内容描述

}__attribute__((packed));

struct msg_list_st
{   
    chnid_t chnid;                                              // must LISTCHNID
    struct msg_listentry_st entry[1];                           // 一个
    
}__attribute__((packed));



#endif 