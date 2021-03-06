#ifndef _COMM_H_
#define _COMM_H_
#include <net/if.h>
#include <linux/if_tun.h>
#include <unistd.h>

int tun_create(char *dev, int flags);
int sock_create(char *ip, unsigned short port);
int check_pack(char *buf, int n);
void make_pack(char *buf, int n);

#endif /* _COMM_H_ */
