#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "comm.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>

struct udp_cli
{
	struct sockaddr_in cliaddr;
	time_t t;
};

struct udp_cli cli_array[64];
int cli_num;

int auto_client(struct sockaddr_in *paddr)
{
	int i;
	for(i = 0; i < cli_num; i++)
	{
		if(memcmp(paddr, &cli_array[i].cliaddr, sizeof(struct sockaddr_in)) == 0)
		{
			cli_array[i].t = time(NULL);
			return i;
		}
	}
	if(cli_num == 64)
	{
		printf("discard new client\n");
		return -1;
	}
	memcpy(&cli_array[i].cliaddr, paddr, sizeof(struct sockaddr_in));
	cli_array[i].t = time(NULL);
	printf("new client ip port %hu\n",
//	       inet_ntoa(paddr->sin_addr),
	       ntohs(paddr->sin_port));
	cli_num++;
	return i;
}

void clean_client()
{
	int flag = 1;
	int i, j;
	time_t t;
	t = time(NULL);
	while(flag)
	{
		flag = 0;
		for(i = 0; i < cli_num; i++)
			if(t - cli_array[i].t > 5)
			{
				printf("del client ip port %hu\n",
	//			       inet_ntoa(cli_array[i].cliaddr.sin_addr),
				       ntohs(cli_array[i].cliaddr.sin_port));
				flag = 1;
				for(j = i+1; j < cli_num; j++)
				{
					cli_array[j-1] = cli_array[j];
				}
				cli_num--;
				break;
			}
	}
}

int main(int argc, char *argv[])
{
	int tunfd, udpfd;
	int ret;
	ssize_t n;
	socklen_t len;
	int nfds;
	char tun_name[IFNAMSIZ];
	char buf[4096];
	struct sockaddr_in cliaddr;
	fd_set rfds;
	struct timeval tv;
	int i;
	nfds = 0;
	tun_name[0] = '\0';
	tunfd = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
	if (tunfd < 0) {
		perror("tun_create");
		return 1;
	}
	if(nfds < tunfd)
		nfds = tunfd;
	sprintf(buf, "ifconfig %s 192.168.0.1 dstaddr 192.168.0.2 mtu 1400 up", tun_name);
	system(buf);
	printf("TUN name is %s\n", tun_name);

	if((udpfd = sock_create("0.0.0.0", argc == 1?2222:atoi(argv[1]))) < 0)
		exit(1);
	if(nfds < udpfd)
		nfds = udpfd;
	nfds++;
	cli_num = 0;
	while (1) {
		FD_ZERO(&rfds);
		FD_SET(tunfd, &rfds);
		FD_SET(udpfd, &rfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(nfds, &rfds, NULL, NULL, &tv);
		if(ret == -1)
			perror("select");
		else if(ret)
		{
			if(FD_ISSET(udpfd, &rfds))
			{
				n = recvfrom(udpfd, buf, 4096, 0,
					     (struct sockaddr *)&cliaddr, &len);
				switch(check_pack(buf, n))
				{
				case 0:
					write(tunfd, buf+2, n-2);
					auto_client(&cliaddr);
					break;
				case 1:
//					printf("beat\n");
					auto_client(&cliaddr);
					break;
				default:
					printf("error packet\n");
					for(i = 0; i < n; i++)
						printf("%02x ", buf[i]);
					printf("\n%zd\n", n);
					break;
				}
			}
			if(FD_ISSET(tunfd, &rfds))
			{
				n = read(tunfd, buf+2, 4096-2);
				make_pack(buf, n+2);
				if(cli_num) {
					int ni = rand()%cli_num;
					sendto(udpfd,
					       buf,
					       n+2,
					       0,
					       (struct sockaddr *)&cli_array[ni].cliaddr,
					       sizeof(struct sockaddr_in));
				}
			}
		}
		clean_client();
	}
	return 0;
}
