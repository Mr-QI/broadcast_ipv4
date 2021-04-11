#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include <errno.h>
#include <getopt.h>
#include "client.h"
#include "protocol.h"
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

/**
 *  -M --mgroup 指定多播组
 *  -P --port   指定接收端口
 *  -p --player 指定播放器
 *  -H --help   显示帮助
 * 
 * 
*/

struct client_conf_st client_conf =  {\
    .rcvport = DEFUULT_RCVPORT,       \
    .mgroup = DEFAULT_MGROUP,         \
    .player_cmd = DEFAULT_PLAYERCMD
    };

static void printhelp(void)
{
    printf("-P  --port      指定接受端口\n");      
    printf("-M  --mgroup    指定多播组\n");  
    printf("-p  --player    指定播放器命令行\n");
    printf("-H  --help      显示帮助信息\n");

}

/** 往管道中持续写入固定字节数据*/
static ssize_t writen(int fd, const char* buf, size_t len)
{
    int ret = 0;
    int pos = 0;
    while (len > 0)
    {
        ret = write(fd, buf+pos, len);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;

            LOG("write error.");
            perror("write error.");
            return -1;
        }
        len -= ret;
        pos += ret; 
    }
    return pos; 
}

int main(int argc, char *argv[])
{

    // 初始化级别：默认值、配置文件、环境变量、命令参数

    int c = 0;
    int index = 0;

    struct sockaddr_in localaddr, serveraddr, remoteaddr;
    socklen_t serveraddr_len = 0, remoteaddr_len = 0;;
    struct ip_mreqn mreq ;
    struct option argarr[] = {{"port", 1, NULL, 'P'},{"help", 1, NULL, 'H'},\
                             {"mgroup", 1, NULL, 'M'},{"player", 1, NULL, 'p'},\
                             {NULL, 0, NULL, 0}};

    while (1)
    {   
        // 长格式命令行分析
        c = getopt_long(argc, argv, "P:M:p:H", argarr, &index);
        if (c < 0) break;
        
        switch (c)
        {
        case 'P': client_conf.rcvport = optarg;    break; 
        case 'M': client_conf.mgroup = optarg;     break;
        case 'p': client_conf.player_cmd = optarg; break;
        case 'H': printhelp(); exit(0);            break;
        default:  abort();                         break;
        }
    }

    int sd = 0;
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("socket error.");
        LOG("socket error.");
        exit(1);
    }
    inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    mreq.imr_ifindex = if_nametoindex("eth0");

    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt error.");
        LOG("setsockopt error.");
        exit(1); 
    }

    int val = 1;
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &val, sizeof(val)) < 0)
    {
        perror("setsockopt error.");
        LOG("setsockopt error.");
        exit(1); 
    }

    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET, "0.0.0.0" ,&localaddr.sin_addr.s_addr);

    if (bind(sd, (void*)&localaddr, sizeof(localaddr)) < 0)
    {
        perror("bind error.");
        LOG("bind error.");
        exit(1);
    }

    int pd[2] = {0};
    if (pipe(pd) < 0)
    {
        perror("pipe error.");
        LOG("pipe error.");
        exit(1);
    }

    int pid = 0;
    pid = fork();
    if (pid < 0)
    {
        perror("fork error.");
        LOG("fork error.");
        exit(1);
    }

    // 子进程， 调解码器
    if (pid == 0)
    {   
        // 子进程关闭socket和管道读端
        close(sd);
        close(pd[1]);
        // 将当前管道的读端作为当前子进程的标准输入：cat 1.mp3 | mpg123 -
        dup2(pd[0], 0);
        if (pd[0] > 0)
            close(pd[0]);
        
        execl("/bin/sh", "sh", "-c", client_conf.player_cmd, NULL);
        perror("execl error");
        LOG("execl error");
        exit(1);
    }
    // 父进程，从网络上收包，发送个子进程

    // 收节目单
    struct msg_list_st *msg_list;
    msg_list = (struct msg_list_st *)malloc(MSG_LIST_MAX);
    if (msg_list == NULL)
    {
        perror("malloc error.");
        LOG("malloc error.");
        exit(1);
    }

    int len = 0;
    while (1)
    {
        // udp协议接受数据api
        len = recvfrom(sd, msg_list, MSG_LIST_MAX, 0, (void*)&serveraddr, &serveraddr_len);
        // 收到数据包内容过小，内容不完整
        if (len < sizeof(struct msg_list_st))
        {
            fprintf(stderr,"message is too small.\n");
            LOG("message is too small.");
            continue;
        }
        // 接收到的数据包不是节目单数据包
        if (msg_list->chnid != LISTCHNID)
        {
            fprintf(stderr,"chnid is not match.\n");
            LOG("chnid is not match.");
            continue;   
        }
        break;
    }

    // 打印节目单并选择频道
    struct msg_listentry_st *pos = NULL;
    for (pos = msg_list->entry; (char*)pos < (((char*)msg_list) + len); pos = (void*)(((char*)pos) + ntohs(pos->len)))
    {
        printf("%d:%s\n",pos->chnid, pos->desc);
    }

    int ret = 0;
    int chosenid = 0;
    
    // 用户选择频道
    while(1)
    {
        ret = scanf("%d", &chosenid);
        if (ret != 1)
            exit(1);
    } 
    free(msg_list);

    // 收频道包，发送给子进程
    struct msg_channel_st *msg_channel;
    msg_channel = malloc(MSG_CHANNEL_MAX);
    if (msg_channel == NULL)
    {
        LOG("malloc error.");
        perror("malloc error");
        exit(1);
    }

    while (1)
    {
        len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX, 0,(void*)&remoteaddr,&remoteaddr_len);
        
        // 接受远程地址和端口和服务器地址及端口不一致
        if (remoteaddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr || remoteaddr.sin_port != serveraddr.sin_port)
        {
            LOG("Ignore: address is not match.");
            fprintf(stderr,"Ignore: address is not match.\n");
            continue;
        }

        // 接受数据包过小
        if (len < sizeof(struct msg_channel_st))
        {
            LOG("Ignore: message is too small.");
            fprintf(stderr,"Ignore: message is too small.\n");
            continue;
        }

        // 接受数据包为用户选择频道，通过管道播放 ，反之则重新接受数据
        if (msg_channel->chnid == chosenid)
        {
            fprintf(stdout, "accepted msg: %d recived.\n",msg_channel->chnid);
            if (writen(pd[1], msg_channel->data, len-sizeof(chnid_t)) < 0 )
                exit(1);
        }

    }
    free(msg_channel);
    close(sd);
   return 0;
}