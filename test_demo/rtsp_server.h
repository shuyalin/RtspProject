#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H
#include <string.h>


#define STREAM_H264 0
#define STREAM_H265 1

typedef struct
{
	char method[32];
	char url[128];
	char version[32];
	int cseq;
	char transport[128];
	int clientRtpPort;
	int clientRtcpPort;
	char *responses;
	int tcp_flag;//0:udp 1:tcp
	int stream_type;//0: H264 1:H265
}rtsp_msg_info;

typedef struct
{
	char client_ip[64];
	int client_rtp_port;
	int serverRtpSockfd;//UDP
	int clenitSockfd;//TCP
	int tcp_flag;//0:udp 1:tcp
	int stream_type;//0: H264 1:H265
	pthread_t play_id;
}rtsp_client_info;

typedef struct
{
	int serverSockfd;
    int serverRtpSockfd;//UDP
	int serverRtcpSockfd;//UDP
}rtsp_server_socket_info;

typedef int (* start_send_rtp_steam)(rtsp_client_info *client_info);

typedef struct
{
	int rtsp_epoll_fd;
	struct epoll_event event;
	char client_ip[64];
	int client_port;
	int client_fd;
	start_send_rtp_steam callback;
	rtsp_msg_info rtsp_msg;
}rtsp_epoll_proc_info;

typedef struct
{
	int rtsp_port;
	int rtp_port;
	int rtcp_port;
	start_send_rtp_steam callback;
	void *param;
}rtsp_init_info;




typedef int (* rtsp_callback)(char *stream_id);
#endif

