#ifndef __MEDIALIB_H__
#define __MEDIALIB_H__
#include <unistd.h>
#include "site_type.h"

struct mlib_listentry_st 
{
    chnid_t chnid;
    char *desc;
};

int mlib_getchnlist(struct mlib_listentry_st **, int *);

int mlib_freechnlist(struct mlib_listentry_st *);


ssize_t mlib_readchnl(chnid_t, void *, size_t );

#endif