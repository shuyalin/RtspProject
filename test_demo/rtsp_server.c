
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include "rtp.h"
#include <sys/epoll.h>
#include "rtsp_malloc.h"
#include <errno.h>
#include "rtsp_server.h"
#include <pthread.h>
#include "rtsp_frame.h"
#include "log.h"
#define H264_FILE_NAME   "./test.h264"
#define H265_FILE_NAME   "./test.h265"
#define SERVER_PORT      8554
#define SERVER_RTP_PORT  55532
#define SERVER_RTCP_PORT 55533

int rtsp_epoll_fd = 0;
static int createTcpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR/* | SO_BROADCAST*/, (const char*)&on, sizeof(on));
	/*if( fcntl( sockfd, F_SETFL, fcntl( sockfd, F_GETFD, 0 )|O_NONBLOCK ) == -1 ) {  
        return -1;  
    }*/
    return sockfd;
}

static int createUdpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, (const char*)&on, sizeof(on));
	/*if( fcntl( sockfd, F_SETFL, fcntl( sockfd, F_GETFD, 0 )|O_NONBLOCK ) == -1 ) {  
        return -1;  
    }*/
    return sockfd;
}

static int bindSocketAddr(int sockfd, const char* ip, int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        return -1;

    return 0;
}

static int acceptClient(int sockfd, char* ip, int* port)
{
    int clientfd;
    socklen_t len = 0;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);

    clientfd = accept(sockfd, (struct sockaddr *)&addr, &len);
    if(clientfd < 0)
        return -1;

	if( fcntl( clientfd, F_SETFL, fcntl( clientfd, F_GETFD, 0 )|O_NONBLOCK ) == -1 ) {  
        return -1;  
    }  
    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);

    return clientfd;
}

static char* getLineFromBuf(char* buf, char* line)
{
	int counts = 0;
    while(*buf != '\n')
    {
        *line = *buf;
        line++;
        buf++;
		counts++;
		if(counts > 395)
		{
			break;
		}
    }

    *line = '\n';
    ++line;
    *line = '\0';

    ++buf;
    return buf; 
}
static char *getLineAccStrFromBuf(char *buf,char *line,const char *str)
{
	char speLine[400] = {0};
	char *bufPos = buf;
	while(1)
	{
		bufPos = getLineFromBuf(bufPos,speLine);
		if(strstr(speLine,str))
		{
			memcpy(line,speLine,strlen(speLine));
			return line;
		}
	}
}
static int handleCmd_OPTIONS(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n"
                    "\r\n",
                    cseq);
                
    return 0;
}
static int handleCmd_DESCRIBE(rtsp_msg_info *rtsp_msg)
{
    char sdp[500];
    char localIp[100];
	char type[16] = {0};
	char channel[16] = {0};
	char port[16] = {0};
	sscanf(rtsp_msg->url, "rtsp://%[^:]:%[^\"/\"]/%[^\"/\"]/%[^\"/\"]/", localIp, port, type,channel);
    /*sscanf(rtsp_msg->url, "rtsp://%[^:]:", localIp);
	
	//sscanf(rtsp_msg->url, "rtsp://%s:%[^/]/%[^/]/", localIp, port, type);
	char *p = strstr(rtsp_msg->url, "//");
	if(p)
	{
		char *t = strstr(p+2, "/");
		if(t)
		{
			printf("t:%s\n", t);
			strncpy(type, t+1, strlen(t+1));
			//sscanf(t+1, "%[^/]/", type);
		}
	}*/
	//printf("localIp :%s,type:%s\n", localIp,type);
	if(!strcmp(type, "H265"))
	{
		rtsp_msg->stream_type = STREAM_H265;
	}
	else if(!strcmp(type, "H264"))
	{
		rtsp_msg->stream_type = STREAM_H264;
	}
	else
	{
		memset(type, 0, 16);
		sprintf(type, "H264");
		rtsp_msg->stream_type = STREAM_H264;
	}
	//printf("rtsp_msg->stream_type:%d\n", rtsp_msg->stream_type);
    sprintf(sdp, "v=0\r\n"
                 "o=- 9%ld 1 IN IP4 %s\r\n"
                 "t=0 0\r\n"
                 "a=control:*\r\n"
                 "m=video 0 RTP/AVP 96\r\n"
                 "a=rtpmap:96 %s/90000\r\n"
                 "a=control:track0\r\n\r\n",
                 time(NULL), localIp, type);
    
    sprintf(rtsp_msg->responses, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
                    "Content-Base: %s\r\n"
                    "Content-type: application/sdp\r\n"
                    "Content-length: %d\r\n\r\n"
                    "%s",
                    rtsp_msg->cseq,
                    rtsp_msg->url,
                    strlen(sdp),
                    sdp);
    return 0;
}

static int handleCmd_SETUP(rtsp_msg_info *rtsp_msg)
{
    /*sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
                    "Session: 66334873\r\n"
                    "\r\n",
                    cseq,
                    clientRtpPort,
                    clientRtpPort+1,
                    SERVER_RTP_TCP_PORT,
                    SERVER_RTCP_TCP_PORT);*/
	sprintf(rtsp_msg->responses, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "%s"
                    "Session: 66334873\r\n"
                    "\r\n",
                    rtsp_msg->cseq, rtsp_msg->transport
                    );
    
    return 0;
}

static int handleCmd_PLAY(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=60\r\n\r\n",
                    cseq);
    
    return 0;
}

static int handleCmd_TEARDOWN(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n\r\n",cseq);
    return 0;
}

int rtsp_recv_data(int clientSockfd, rtsp_char_buf **buf)
{
	int recvLen;
	int ret = 1;
	
	rtsp_char_buf * buf_dat = rtsp_malloc_char_buf(BUF_MAX_SIZE);
	if(NULL == buf_dat)
	{
		printf("buf_dat is rtsp_malloc_char_buf error\n");
		return -1;
	}
	do
	{
	    recvLen = recv(clientSockfd, buf_dat->dat, BUF_MAX_SIZE, 0);
	    if(recvLen < 0)
	    {
	    	
	    	if(errno == EAGAIN || errno == EWOULDBLOCK)
			{				
				printf("there is no left data :%d\n", errno);
				break;
	    	}
			else
			{
				rtsp_free_char_buf(buf_dat);
				printf("recv is error\n");
	    		return -1;
			}
	    }
		else if(!recvLen)
		{
			printf("read size is null\n");
			ret = 0;
		}
		else
		{
			buf_dat->dat_size = recvLen;
			buf_dat->dat_free_size = buf_dat->dat_total_size - recvLen;
			rtsp_copy_char_buf(buf, buf_dat);
			rtsp_clean_char_buf(buf_dat);
			ret = 1;
		}
		//printf("recvLen :%d\n", recvLen);
	}while(recvLen);//由于采用的是ET模型，所以必须循环读完
	rtsp_free_char_buf(buf_dat);
	buf_dat = NULL;
	//printf("recv is end\n");
	return ret;
}



int rtsp_msg_parse(rtsp_msg_info *rtsp_msg, rtsp_char_buf *rBuf)
{
	char line[400];
	char *bufPtr = NULL;
	printf("rBuf->dat:%s\n", rBuf->dat);
	/* 解析方法 */
    bufPtr = getLineFromBuf(rBuf->dat, line);
    if(sscanf(line, "%s %s %s\r\n", rtsp_msg->method, rtsp_msg->url, rtsp_msg->version) != 3)
    {
        printf("parse err1\n");
		return -1;
    }
	//url格式:  rtsp://192.168.4.180:8554/H264/sub
	/* 解析序列号 */
    //bufPtr = getLineFromBuf(bufPtr, line);
	getLineAccStrFromBuf(bufPtr,line,"CSeq");
    if(sscanf(line, "CSeq: %d\r\n", &rtsp_msg->cseq) != 1)
    {
        printf("parse err2\n");
		return -1;
    }
    /* 如果是SETUP，那么就再解析client_port */
    if(!strcmp(rtsp_msg->method, "SETUP"))
    {
        while(1)
        {
            getLineAccStrFromBuf(bufPtr,line,"Transport:");
            if(!strncmp(line, "Transport:", strlen("Transport:")))
            {
            	char *p_tcp = strstr(line, "RTP/AVP/TCP");
				if(p_tcp)
				{
					rtsp_msg->tcp_flag = 1;
					strncpy(rtsp_msg->transport, line, strlen(line));
				}
				else
				{
					rtsp_msg->tcp_flag = 0;
					sscanf(line, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",
                                &rtsp_msg->clientRtpPort, &rtsp_msg->clientRtcpPort);
					sprintf(rtsp_msg->transport, "%s", line);
					sprintf(rtsp_msg->transport+strlen(rtsp_msg->transport)-2, ";server_port=%d-%d\r\n", SERVER_RTP_PORT,
                    SERVER_RTCP_PORT);
				}
                
                break;
            }
        }
    }

    if(!strcmp(rtsp_msg->method, "OPTIONS"))
    {
        if(handleCmd_OPTIONS(rtsp_msg->responses, rtsp_msg->cseq))
        {
            printf("failed to handle options\n");
            return -1;
        }
    }
    else if(!strcmp(rtsp_msg->method, "DESCRIBE"))
    {
        if(handleCmd_DESCRIBE(rtsp_msg))
        {
            printf("failed to handle describe\n");
            return -1;
        }
    }
    else if(!strcmp(rtsp_msg->method, "SETUP"))
    {
        if(handleCmd_SETUP(rtsp_msg))
        {
           printf("failed to handle setup\n");
           return -1;
        }
    }
    else if(!strcmp(rtsp_msg->method, "PLAY"))
    {
        if(handleCmd_PLAY(rtsp_msg->responses, rtsp_msg->cseq))
        {
            printf("failed to handle play\n");
            return -1;
        }
    }
	else if(!strcmp(rtsp_msg->method, "TEARDOWN"))
	{
		if(handleCmd_TEARDOWN(rtsp_msg->responses, rtsp_msg->cseq))
		{
			printf("failed to handle teardown\n");
            return -1;
		}
	}
    else
    {
        return -1;
    }
	return 0;
}
static int client_read(rtsp_epoll_proc_info *epoll_info, rtsp_server_socket_info *socket_info)

{
	rtsp_char_buf *rBuf = NULL;
    char* sBuf = malloc(BUF_MAX_SIZE);
    if(NULL == sBuf)
    {
		return -1;
	}
	memset(sBuf, 0, BUF_MAX_SIZE);
	epoll_info->rtsp_msg.responses= sBuf;
	int ret = rtsp_recv_data(epoll_info->client_fd, &rBuf);
	if(ret < 0 || rBuf == NULL)
	{
		printf("rtsp_recv_data err rBuf:%p\n", rBuf);
        goto out;
	}
    printf("---------------C->S--------------\n");
    printf("%s", rBuf->dat);
	if(rtsp_msg_parse(&epoll_info->rtsp_msg, rBuf) < 0)
	{
		printf("rtsp_msg_parse err\n");
        goto out;
	}
    printf("---------------S->C--------------\n");
    printf("%s", sBuf);
    send(epoll_info->client_fd, sBuf, strlen(sBuf), 0);

    /* 开始播放，发送RTP包 */
    if(!strcmp(epoll_info->rtsp_msg.method, "PLAY"))
    {
        if(epoll_info->callback)
        {
        	//printf("start play epoll_info->rtsp_msg.clientRtpPort:%d\n", epoll_info->rtsp_msg.clientRtpPort);
        	rtsp_client_info client_info;
			memset(&client_info, 0, sizeof(rtsp_client_info));
			strncpy(client_info.client_ip, epoll_info->client_ip, strlen(epoll_info->client_ip));
			client_info.serverRtpSockfd = socket_info->serverRtpSockfd;
			client_info.clenitSockfd = epoll_info->client_fd;
			client_info.client_rtp_port = epoll_info->rtsp_msg.clientRtpPort;
			client_info.stream_type = epoll_info->rtsp_msg.stream_type;
			client_info.tcp_flag = epoll_info->rtsp_msg.tcp_flag;
			logs("client_info.stream_type:%d", client_info.stream_type);
			epoll_info->callback(&client_info);
		}
    }

out:
    printf("finish\n");
    rtsp_free_char_buf(rBuf);
    free(epoll_info->rtsp_msg.responses);
	epoll_info->rtsp_msg.responses = NULL;
	if(!ret)
	{
		printf("del epoll\n");
		epoll_info->event.data.ptr = (void *)epoll_info;
		epoll_info->event.events = (EPOLLIN | EPOLLET);//采用ET模式
		epoll_ctl(epoll_info->rtsp_epoll_fd, EPOLL_CTL_DEL, epoll_info->client_fd, &epoll_info->event);
		free(epoll_info);
		epoll_info = NULL;
	}
	else
	{
		epoll_info->event.data.ptr = (void *)epoll_info;
		epoll_info->event.events = (EPOLLIN | EPOLLET);//采用ET模式
		epoll_ctl(epoll_info->rtsp_epoll_fd, EPOLL_CTL_MOD, epoll_info->client_fd, &epoll_info->event);
	}
	return 0;
}

int rtsp_server_socket_init(rtsp_server_socket_info *socket_info, rtsp_init_info *rtsp_info)
{
	if(socket_info == NULL || rtsp_info == NULL)
	{
		logerror("param is null\n");
		return -1;
	}
	socket_info->serverSockfd = createTcpSocket();
    if(socket_info->serverSockfd < 0)
    {
        logerror("failed to create tcp socket\n");
        return -1;
    }

    if(bindSocketAddr(socket_info->serverSockfd, "0.0.0.0", rtsp_info->rtsp_port) < 0)
    {
        logerror("failed to bind addr\n");
        return -1;
    }

    if(listen(socket_info->serverSockfd, 10) < 0)
    {
        logerror("failed to listen\n");
        return -1;
    }

    socket_info->serverRtpSockfd = createUdpSocket();
    socket_info->serverRtcpSockfd = createUdpSocket();
    if(socket_info->serverRtpSockfd < 0 || socket_info->serverRtcpSockfd < 0)
    {
        logerror("failed to create udp socket\n");
        return -1;
    }

    if(bindSocketAddr(socket_info->serverRtpSockfd, "0.0.0.0", rtsp_info->rtp_port) < 0 ||
        bindSocketAddr(socket_info->serverRtcpSockfd, "0.0.0.0", rtsp_info->rtcp_port) < 0)
    {
        logerror("failed to bind addr\n");
        return -1;
    }
	return 0;
}

void *rtsp_func(void *arg)
{
	rtsp_init_info *rtsp_info = (rtsp_init_info *)arg;
	rtsp_server_socket_info socket_info;
	rtsp_epoll_proc_info *t_epoll = NULL;
	rtp_epoll_out_send *w_epoll = NULL;
	struct epoll_event s_event;

	if(rtsp_server_socket_init(&socket_info, rtsp_info) < 0)//创建rtsp和rtp，rtcp的套接字
	{
		logerror("rtsp server create socket is error\n");
		return NULL;
	}
    printf("rtsp://127.0.0.1:%d\n", rtsp_info->rtsp_port);
		
    rtsp_epoll_fd = epoll_create(256);//创建epoll
	if(rtsp_epoll_fd < 0)
	{
		logerror("rtsp_epoll_fd is -1\n");
		return NULL;
	}
	rtsp_epoll_proc_info s_epoll_info;
	memset(&s_epoll_info, 0, sizeof(rtsp_epoll_proc_info));
	s_epoll_info.client_fd = socket_info.serverSockfd;//rtsp服务器的socket
	
	memset(&s_event, 0, sizeof(struct epoll_event));
	s_event.data.ptr = (void *)&s_epoll_info;
	s_event.events = EPOLLIN;
	epoll_ctl(rtsp_epoll_fd, EPOLL_CTL_ADD, socket_info.serverSockfd, &s_event);//先把服务器监听的客户端链接的accept加入epoll

	struct epoll_event event_list[1024];
	while(1)
    {
        int clientSockfd;
        char clientIp[40];
        int clientPort;
		int i;
		int number = epoll_wait(rtsp_epoll_fd,event_list,1024,500);
        for(i = 0;i < number;i++)
        {
        	if(event_list[i].events & EPOLLIN)//如果是读事件
        	{
        		t_epoll = (rtsp_epoll_proc_info *)event_list[i].data.ptr;
				if(socket_info.serverSockfd == t_epoll->client_fd)
				{
					clientSockfd = acceptClient(socket_info.serverSockfd, clientIp, &clientPort);
			        if(clientSockfd < 0)
			        {
			            logerror("failed to accept client\n");
			            continue;
			        }					
					rtsp_epoll_proc_info *epoll_info= malloc(sizeof(rtsp_epoll_proc_info));
					if(epoll_info)
					{
						memset(epoll_info, 0, sizeof(rtsp_epoll_proc_info));
						epoll_info->client_port = clientPort;
						epoll_info->client_fd = clientSockfd;
						logs("epoll_info->client_fd:%d", epoll_info->client_fd);
						strncpy(epoll_info->client_ip, clientIp, strlen(clientIp));
						epoll_info->callback = rtsp_info->callback;
						epoll_info->rtsp_epoll_fd = rtsp_epoll_fd;
						
						epoll_info->event.data.ptr = (void *)epoll_info;
						epoll_info->event.events = (EPOLLIN | EPOLLET);//采用ET模式
						epoll_ctl(rtsp_epoll_fd, EPOLL_CTL_ADD, clientSockfd, &epoll_info->event);
					}
				}
				else
				{
					//printf("accept client;client ip:%s,client port:%d\n", clientIp, clientPort);
					client_read(t_epoll, &socket_info);
				}
			}
			
			if (event_list[i].events & EPOLLOUT)
			{
				w_epoll = (rtp_epoll_out_send *)event_list[i].data.ptr;
				if(w_epoll)
				{
					*w_epoll->epoll_out_event_flag = 0;
					logs(">>>>>>>>epoll_out_event_flag:%d", *w_epoll->epoll_out_event_flag);
				}
			}
		}
        
    }
	return NULL;
}

int rtsp_server_init(rtsp_init_info *rtsp_info)
{
	pthread_t rtsp_id;
	if(pthread_create(&rtsp_id, NULL, rtsp_func, (void *)rtsp_info) != 0)
	{
	   printf("create pthread is faided\n");
	   return -1;
	}
	pthread_detach(rtsp_id);
	return 0;
	
}

void *play_func(void *arg)
{
	rtsp_client_info *client_info = (rtsp_client_info *)arg;
	//struct RtpPacket* rtpPacket = (struct RtpPacket*)malloc(500000);
	struct RtpPacket rtpPacket;
	rtpHeaderInit(&rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0,
	                0, 0, 0x88923423/*(uint32_t)time(NULL)*/);
	//printf("start play\n");
	//printf("client ip:%s\n", client_info->client_ip);
	//printf("client port:%d\n", client_info->client_rtp_port);
	pthread_t p_id =  pthread_self();
	rtp_net_info net_info;
	memset(&net_info, 0, sizeof(rtp_net_info));
	strncpy(net_info.ip, client_info->client_ip, strlen(client_info->client_ip));
	logs("client_info->tcp_flag:%d", client_info->tcp_flag);
	if(client_info->tcp_flag == TCP)
	{
		net_info.fd = client_info->clenitSockfd;
	}
	else if(client_info->tcp_flag == UDP)
	{
		net_info.fd = client_info->serverRtpSockfd;
	}
	net_info.port= client_info->client_rtp_port;
	net_info.udp_or_tcp = client_info->tcp_flag;
	net_info.stream_type = client_info->stream_type;
	if(net_info.stream_type == STREAM_H264)
		read_frame(H264_FILE_NAME, &rtpPacket, &net_info);
	else if(net_info.stream_type == STREAM_H265)
		read_frame(H265_FILE_NAME, &rtpPacket, &net_info);
	//free(rtpPacket);
	free(client_info);
	pthread_cancel(p_id);
	return NULL;
}

int rtsp_start_stream(rtsp_client_info *client_info)
{
	rtsp_client_info *param_info= malloc(sizeof(rtsp_client_info));
	if(param_info == NULL)
	{
		return -1;
	}
	memset(param_info, 0, sizeof(rtsp_client_info));
	param_info->client_rtp_port = client_info->client_rtp_port;
	param_info->serverRtpSockfd = client_info->serverRtpSockfd;
	param_info->clenitSockfd = client_info->clenitSockfd;
	param_info->stream_type = client_info->stream_type;
	param_info->tcp_flag = client_info->tcp_flag;
	strncpy(param_info->client_ip, client_info->client_ip, strlen(client_info->client_ip));
	if(pthread_create(&param_info->play_id, NULL, play_func, (void *)param_info) != 0)
	{
	   printf("create pthread is faided\n");
	   return -1;
	}
	pthread_detach(param_info->play_id);
   return 0;
}

int main(void)
{
	rtsp_init_info rtsp_info;
	rtsp_info.rtsp_port = SERVER_PORT;//rtsp端口
	rtsp_info.rtp_port = SERVER_RTP_PORT;//rtp的udp的端口
	rtsp_info.rtcp_port = SERVER_RTCP_PORT;//rtcp的udp端口
	rtsp_info.callback = rtsp_start_stream;
	rtsp_server_init(&rtsp_info);
    while(1)
	{
		sleep(1);
	}

    return 0;
}

