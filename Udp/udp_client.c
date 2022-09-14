#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
int main()
{
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    assert( sockfd != -1 );

	struct sockaddr_in saddr;
 	memset(&saddr,0,sizeof(saddr));
 	saddr.sin_family = AF_INET;
 	saddr.sin_port = htons(6000);
	saddr.sin_addr.s_addr = inet_addr("192.168.226.174");

	while(1)
	{
		char buff[128]={0};
		printf("input\n");
		fgets(buff,127,stdin);
		if(strncmp(buff,"end",3)==0)
		{
			break;
		}
		printf("port=%d\n",saddr.sin_port);
		sendto(sockfd,buff,strlen(buff),0,(struct sockaddr*)&saddr,sizeof(saddr));
		memset(&buff,128,0);
		int len=sizeof(saddr);
		recvfrom(sockfd,buff,127,0,(struct sockaddr*)&saddr,&len);
		printf("recv:%s\n",buff);
	}
	close(sockfd);
	exit(0);
}
