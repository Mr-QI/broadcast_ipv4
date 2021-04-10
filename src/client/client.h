#ifndef __CLIENT_H__
#define __CLIENT_H__

#define DEFAULT_PLAYERCMD       "/usr/bin/mpg123 > /dev/null" 


/* 命令参数 */
struct client_conf_st
{
    char *rcvport;
    char *mgroup;
    char *player_cmd;
};

extern struct client_conf_st client_conf;

#endif 