#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/stat.h>
#include "medialib.h"
#include "protocol.h"
#include "thr_list.h"
#include "server_conf.h"
#include "thr_channel.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

struct server_conf_st server_conf = {\
    .rcvport = DEFUULT_RCVPORT,\
    .mgroup = DEFAULT_MGROUP,\
    .media_dir = DEFAULT_MEDIADIR,\
    .ifname = DEFULLT_IFNAME,\
    .runmode = RUN_DAEMON}; 


static void printhelp(void)
{
    printf("-M      指定多播组\n");
    printf("-P      指定端口\n");
    printf("-D      指定媒体库位置\n");
    printf("-F      前台运行\n");
    printf("-I      指定网络设\n");
    printf("-H      显示帮助\n");
}

// 处理释放句柄
static void daemon_exit(int s)
{

    closelog();

    exit(0);
}

static int daemonize(void)
{
    pid_t pid;
    pid = fork();

    // fork failed
    if (pid < 0)
    {
        //perror("fork error.");
        syslog(LOG_ERR, "fork():%s", strerror(errno));
        return -1;
    }

    // parent
    if (pid > 0)
        exit(0);
    
    // 重定向打开的标准文件描述符至空设备
    int fd = 0;
    fd = open("/dev/null", O_RDWR);
    if (fd < 0)
    {
        //perror("open error.");
        syslog(LOG_WARNING, "open():%s", strerror(errno));
        return -2;
    }else{

        dup2(fd, 0);    
        dup2(fd, 1);    
        dup2(fd, 2);
        if (fd > 2)
            close(fd);
    }
    setsid();
    chdir("/");
    umask(0);
    return 0;
}

static void socket_init(void)
{
    int serversd = 0; 
    struct ip_mreqn mreq;
    serversd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serversd < 0)
    {
        syslog(LOG_ERR, "socket():%s", strerror(errno));
        exit(1);
    }

    inet_pton(AF_INET, server_conf.mgroup, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    mreq.imr_ifindex = if_nametoindex(server_conf.ifname);

    if (setsockopt(serversd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0)
    {
        syslog(LOG_ERR,"setsocket(IP_MULTICAST_IF):%s",strerror(errno));
        exit(1);
    }
    //bind();
}

int main(int argc, char* argv[])
{
    
    int c = 0;
    struct sigaction sa;
    sa.sa_handler = daemon_exit;
    sigemptyset(sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTERM);

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    openlog("netradio", LOG_PID|LOG_PERROR, LOG_DAEMON);

    // 命令行分析
   while(1)
   {
        c = getopt(argc, argv, "M:P:FD:I:H:");
        if (c < 0) break;
        switch (c)
        {
        case 'M': server_conf.mgroup = optarg;          break;
        case 'P': server_conf.rcvport = optarg;         break;
        case 'F': server_conf.runmode = RUN_FOREGROUND; break;
        case 'D': server_conf.media_dir = optarg;       break;
        case 'I': server_conf.ifname = optarg;          break;
        case 'H': printhelp();                          break;
        default:  abort(); break;
        }
   }
    
    // 守护进程的实现
    if (server_conf.runmode == RUN_DAEMON)
    {
        if (daemonize() != 0)
            exit(1);
    }
    else if (server_conf.runmode == RUN_FOREGROUND)
    {
        // do nothing  
    }else{
         //fprintf(stderr, "EINAL\n");
        syslog(LOG_ERR, "EINVAL server_conf.runmode");
        exit(1);
    }

    // socket初始化
    socketinit();

    // 获取频道信息
    int err = 0;
    int list_size = 0;;
    struct mlib_listentry_st *list;
    err = mlib_getchnlist(&list, &list_size);
    if (err < 0)
    {

    }

    // 创建节目单线程
    
    err = thr_list_create(list, list_size);
    if (err < 0)
    {

    }

    // 创建频道线程
    int i = 0;
    fo(i = 0; i < list_size; i++)
    {
        thr_channel_create(list+i);
        // if error

    }
    syslog(LOG_DEBUG, "%d channel threads created.", i);

    while (1)
        pause();
    
    return 0;
}