#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "comm.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>

struct udp_conn
{
	int fd;
	time_t t;
};

struct udp_conn conn[64];
int conn_num;

struct sockaddr_in seraddr;

void beat_server()
{
	int i;
	time_t t;
	char buf[2] = {0xb, 0xb};
	t = time(NULL);
	for(i = 0; i < conn_num; i++)
	{
		if(t - conn[i].t > 0)
		{
			if(sendto(conn[i].fd, buf, 2, 0,
				  (struct sockaddr *)&seraddr,
				  sizeof(struct sockaddr_in)) < 0)
				perror("beat");
		}
	}
}

int main(int argc, char *argv[])
{
	int tunfd;
	int ret;
	ssize_t n;
	socklen_t len;
	int nfds;
	char tun_name[IFNAMSIZ];
	char buf[4096];
	fd_set rfds;
	struct timeval tv;
	int i, j;
	if(argc != 3)
		return -1;
	tun_name[0] = '\0';
	tunfd = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
	if (tunfd < 0) {
		perror("tun_create");
		return 1;
	}
	sprintf(buf, "ifconfig %s 192.168.0.2 dstaddr 192.168.0.1 mtu 1400 up", tun_name);
	system(buf);
	printf("TUN name is %s\n", tun_name);

	bzero(&seraddr, sizeof(seraddr));
	seraddr.sin_family = AF_INET;
	seraddr.sin_addr.s_addr = inet_addr(argv[1]);
	seraddr.sin_port = htons(atoi(argv[2]));

	nfds = 0;
	for(conn_num = 0;gets(buf) && conn_num < 64;conn_num++)
	{
		if((conn[conn_num].fd = sock_create(buf, 1900+conn_num)) < 0)
			exit(1);
		conn[conn_num].t = time(NULL);
		if(nfds < conn[conn_num].fd)
			nfds = conn[conn_num].fd;
	}
	nfds++;

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(tunfd, &rfds);
		for(i = 0; i < conn_num; i++)
		{
			FD_SET(conn[i].fd, &rfds);
		}
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(nfds, &rfds, NULL, NULL, &tv);
		if(ret == -1)
			perror("select");
		else if(ret)
		{
			for(i = 0; i < conn_num; i++)
			{
				if(FD_ISSET(conn[i].fd, &rfds))
				{
					n = recvfrom(conn[i].fd, buf, 4096, 0,
						     (struct sockaddr *)&seraddr, &len);
					switch(check_pack(buf, n))
					{
					case 0:
						write(tunfd, buf+2, n-2);
						/* no break */
					case 1:
						/*update timestamp*/
						conn[i].t = time(NULL);
						break;
					default:
						printf("error packet\n");
						for(j = 0; j < n; j++)
							printf("%02x ", buf[j]);
						printf("\n%zd\n", n);
						break;
					}
				}
			}
			if(FD_ISSET(tunfd, &rfds))
			{
				int ni = rand()%conn_num;
				n = read(tunfd, buf+2, 4096-2);
				make_pack(buf, n+2);
				sendto(conn[ni].fd,
				       buf,
				       n+2,
				       0,
				       (struct sockaddr *)&seraddr,
				       sizeof(struct sockaddr_in));
			}
		}
		beat_server();
	}
	return 0;
}
