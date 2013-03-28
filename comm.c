#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "comm.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int tun_create(char *dev, int flags)
{
	struct ifreq ifr;
	int fd, err;

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
		return fd;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags |= flags;
	if (*dev != '\0')
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		close(fd);
		return err;
	}
	strcpy(dev, ifr.ifr_name);

	return fd;
}

int sock_create(char *ip, unsigned short port)
{
	int sockfd;
	struct sockaddr_in addr;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	perror("socket");
	/* init servaddr */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	/* bind address and port to socket */
	if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind error");
		return -1;
	}
	return sockfd;
}

int check_pack(char *buf, int n)
{
	int i;
	unsigned char x;
	if(n == 2 && buf[0] == 0xb && buf[1] == 0xb)
		return 1;
	if(n > 2 && buf[0] == 0xd)
	{
		x = 0;
		for(i = 0; i < n; i++)
			x ^= buf[i];
		if(x == 0)
			return 0;
	}
	return -1;
}

void make_pack(char *buf, int n)
{
	int i;
	unsigned char x;
	buf[0] = 0xd;
	buf[1] = 0;
	x = 0;
	for(i = 0; i < n; i++)
		x ^= buf[i];
	buf[1] = x;
}
