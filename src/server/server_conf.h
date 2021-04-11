#ifndef __SERVER_CONF_H__
#define __SERVER_CONF_H__

#define DEFAULT_MEDIADIR    "/var/media"
#define DEFULLT_IFNAME      "eth0"
    
enum 
{
    RUN_DAEMON = 1,
    RUN_FOREGROUND
};

struct server_conf_st
{
    char* rcvport;
    char* mgroup;
    char* media_dir;
    char* ifname;
    char runmode;
};
extern struct server_conf_st server_conf;


#endif