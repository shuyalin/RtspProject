#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>

int main()
{
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    assert( sockfd != -1 );

	struct sockaddr_in saddr,caddr;
 	memset(&saddr,0,sizeof(saddr));
 	saddr.sin_family = AF_INET;
 	saddr.sin_port = htons(6000);
	saddr.sin_addr.s_addr = inet_addr("192.168.226.174");

	int res = bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert( res != -1 );

	while(1)
	{
		int len=sizeof(caddr);
		char buff[128]={0};
		recvfrom(sockfd,buff,127,0,(struct sockaddr*)&caddr,&len);
		printf("buff=%s %d\n",buff,caddr.sin_port);
		if(strncmp(buff,"end",3)==0)
		{
			break;
		}
		sendto(sockfd,"ok",2,0,(struct sockaddr*)&caddr,sizeof(caddr));
	}
	close(sockfd);
	exit(0);
}
