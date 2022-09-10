#include <sys/types.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include "rtp.h"
#include "log.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
int mysend(int socket, void *dat, int dat_len)
{
	int len = 0;
	int ret = 0;
	do
	{
		ret = send(socket, (void*)dat+len, dat_len-len, 0);
		if(ret >= 0)
			len += ret;
	}while(ret < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) );
	//while((ret >= 0 || ret < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) ) && len < (dataSize+RTP_HEADER_SIZE+4));
	return dat_len-ret;
}
void rtpHeaderInit(struct RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
                    uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc)
{
    rtpPacket->rtpHeader.csrcLen = csrcLen;
    rtpPacket->rtpHeader.extension = extension;
    rtpPacket->rtpHeader.padding = padding;
    rtpPacket->rtpHeader.version = version;
    rtpPacket->rtpHeader.payloadType =  payloadType;
    rtpPacket->rtpHeader.marker = marker;
    rtpPacket->rtpHeader.seq = seq;
    rtpPacket->rtpHeader.timestamp = timestamp;
    rtpPacket->rtpHeader.ssrc = ssrc;
}
static int test_fd = -1;
extern int rtsp_epoll_fd;
int rtptcpSendPacket(rtp_net_info *net_info, tcp_RtpPacket* rtpPacket, uint32_t dataSize)
{
    int ret;
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);
	int len = 0;
	do
	{
		ret = send(net_info->fd, (void*)rtpPacket+len, dataSize+RTP_HEADER_SIZE+4-len, 0);
		len += ret;
	}while((ret >= 0 || ret < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) && len < (dataSize+RTP_HEADER_SIZE+4));
	//ret = mysend(net_info->fd, rtpPacket, dataSize+RTP_HEADER_SIZE+4);
	if(ret < 0)
	{
		if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
		{
			ret = 0;
		}
		else		
			printf(">>>>>>>>>>>>>>>>>>>>send error :%d\n",errno);
	}
    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);
    return ret;
}

int rtpudpSendPacket(rtp_net_info *net_info, struct RtpPacket* rtpPacket, uint32_t dataSize)
{
    struct sockaddr_in addr;
    int ret;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(net_info->port);
    addr.sin_addr.s_addr = inet_addr(net_info->ip);

    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

	char IPdotdec[20]; 
	inet_pton(AF_INET, IPdotdec, (void *)&(addr.sin_addr));
	inet_ntop(AF_INET, (void *)&(addr.sin_addr), IPdotdec, 16);// 反转换         
	printf("localip=%s\n",IPdotdec);
	printf("datesize %d %d\n",dataSize+RTP_HEADER_SIZE,sizeof(addr));
   	ret = sendto(net_info->fd, (void*)rtpPacket, dataSize+RTP_HEADER_SIZE, 0,
                    (struct sockaddr*)&addr, sizeof(addr));
	if(ret < 0)
	{
		printf(">>>>>>>>>>>>>>>>>>>>sendto udp error :%d %s\n",errno,strerror(errno));
	}
    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

    return ret;
}



int rtp_tcp_send_frame(struct RtpPacket* rtphead, rtp_net_info *net_info, char* frame_buf, int frameSize)
{
	int ret = 0;
	int sendBytes = 0;
	int naluType = frame_buf[0];
	tcp_RtpPacket *rtpPacket = malloc(sizeof(tcp_RtpPacket)+frameSize+2);
	if(NULL == rtpPacket)
	{
		logerror("rtpPacket is malloc error");
		return -1;
	}
	rtpPacket->tcp_head[0] = 0x24;
	rtpPacket->tcp_head[1] = 0;
	
	memcpy(&rtpPacket->rtpHeader, rtphead, sizeof(struct RtpHeader));
	if (frameSize <= RTP_MAX_PKT_SIZE)
	{
		memcpy(rtpPacket->payload, frame_buf, frameSize);
		rtpPacket->tcp_head[2] = (frameSize+sizeof(struct RtpHeader))>>8;
		rtpPacket->tcp_head[3] = frameSize+sizeof(struct RtpHeader);
		ret = rtptcpSendPacket(net_info, rtpPacket, frameSize);
        if(ret < 0)
        {
        	logerror("rtpSendPacket is error");
            goto err;
        }
		rtphead->rtpHeader.seq++;
		rtpPacket->rtpHeader.seq = rtphead->rtpHeader.seq;
		sendBytes+=ret;
	}
	else
	{	
		
		int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 1, head_len;
		if(net_info->stream_type == RTP_H264)
			head_len = 1;
		else if(net_info->stream_type == RTP_H265)
			head_len = 2;
		pos = head_len; 
		for(i = 0; i < pktNum; i++)
		{
			log1("net_info->stream_type:%d  pos %d", net_info->stream_type, pos);
			if(net_info->stream_type == RTP_H264)
			{
				rtpPacket->payload[0] = (naluType & 0x60) | 28;
	            rtpPacket->payload[1] = naluType & 0x1F;
	            if (i == 0) //第一包数据
	                rtpPacket->payload[1] |= 0x80; // start
	            else if (remainPktSize == 0 && i == (pktNum - 1)) //最后一包数据
	                rtpPacket->payload[1] |= 0x40; // end
			}
			else if(net_info->stream_type == RTP_H265)
			{
				rtpPacket->payload[0] = 49 << 1;
				rtpPacket->payload[1] = 1;
	            rtpPacket->payload[2] = (naluType & 0x7E)>>1;
	            if (i == 0) //第一包数据
	                rtpPacket->payload[2] |= 0x80; // start
	            else if (remainPktSize == 0 && i == (pktNum - 1)) //最后一包数据
	                rtpPacket->payload[2] |= 0x40; // end
			}
            memcpy(rtpPacket->payload+head_len+1, frame_buf+pos, RTP_MAX_PKT_SIZE);
			rtpPacket->tcp_head[2] = (RTP_MAX_PKT_SIZE+sizeof(struct RtpHeader)+head_len+1)>>8;
			rtpPacket->tcp_head[3] = RTP_MAX_PKT_SIZE+sizeof(struct RtpHeader)+head_len+1;
            ret = rtptcpSendPacket(net_info, rtpPacket, RTP_MAX_PKT_SIZE+head_len+1);
            if(ret < 0)
	        {
	        	logerror("rtpSendPacket is error");
	            goto err;
	        }
            rtphead->rtpHeader.seq++;
			rtpPacket->rtpHeader.seq = rtphead->rtpHeader.seq;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
		}
		if (remainPktSize > 0)
        {
        	if(net_info->stream_type == RTP_H264)
        	{
	            rtpPacket->payload[0] = (naluType & 0x60) | 28;
	            rtpPacket->payload[1] = naluType & 0x1F;
	            rtpPacket->payload[1] |= 0x40; //end
        	}
			else if(net_info->stream_type == RTP_H265)
			{
				rtpPacket->payload[0] = 49 << 1;
				rtpPacket->payload[1] = 1;
	            rtpPacket->payload[2] = (naluType & 0x7E)>>1;
	            rtpPacket->payload[2] |= 0x40; // end
			}
			rtpPacket->tcp_head[2] = (remainPktSize+sizeof(struct RtpHeader)+head_len+1)>>8;
			rtpPacket->tcp_head[3] = remainPktSize+sizeof(struct RtpHeader)+head_len+1;
            memcpy(rtpPacket->payload+head_len+1, frame_buf+pos, remainPktSize);
            ret = rtptcpSendPacket(net_info, rtpPacket, remainPktSize+head_len+1);
            if(ret < 0)
	        {
	        	logerror("rtpSendPacket is error");
	            goto err;
	        }

            rtphead->rtpHeader.seq++;
			rtpPacket->rtpHeader.seq = rtphead->rtpHeader.seq;
            sendBytes += ret;
        }
	}
	free(rtpPacket);
	rtpPacket = NULL;
	return sendBytes;
err:
	free(rtpPacket);
	rtpPacket = NULL;
	return -1;
}

int rtp_udp_send_frame(struct RtpPacket* rtphead, rtp_net_info *net_info, char* frame_buf, int frameSize)
{
	int ret = 0;
	int sendBytes = 0;
	int naluType = frame_buf[0];
	struct RtpPacket *rtpPacket = malloc(sizeof(struct RtpPacket)+frameSize+2);
	if(NULL == rtpPacket)
	{
		logerror("rtpPacket is malloc error");
		return -1;
	}
	memcpy(rtpPacket, rtphead, sizeof(struct RtpPacket));
	if (frameSize <= RTP_MAX_PKT_SIZE)
	{
		memcpy(rtpPacket->payload, frame_buf, frameSize);
		ret = rtpudpSendPacket(net_info, rtpPacket, frameSize);
        if(ret < 0)
        {
        	logerror("rtpSendPacket is error");
            goto err;
        }
		rtphead->rtpHeader.seq++;
		rtpPacket->rtpHeader.seq = rtphead->rtpHeader.seq;
		sendBytes+=ret;
	}
	else
	{	
		
		int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 1, head_len;
		if(net_info->stream_type == RTP_H264)
			head_len = 1;
		else if(net_info->stream_type == RTP_H265)
			head_len = 2;
		pos = head_len; 
		for(i = 0; i < pktNum; i++)
		{
			log1("net_info->stream_type:%d  pos %d", net_info->stream_type, pos);
			if(net_info->stream_type == RTP_H264)
			{
				rtpPacket->payload[0] = (naluType & 0x60) | 28;
	            rtpPacket->payload[1] = naluType & 0x1F;
	            if (i == 0) //第一包数据
	                rtpPacket->payload[1] |= 0x80; // start
	            else if (remainPktSize == 0 && i == (pktNum - 1)) //最后一包数据
	                rtpPacket->payload[1] |= 0x40; // end
			}
			else if(net_info->stream_type == RTP_H265)
			{
				rtpPacket->payload[0] = 49 << 1;
				rtpPacket->payload[1] = 1;
	            rtpPacket->payload[2] = (naluType & 0x7E)>>1;
	            if (i == 0) //第一包数据
	                rtpPacket->payload[2] |= 0x80; // start
	            else if (remainPktSize == 0 && i == (pktNum - 1)) //最后一包数据
	                rtpPacket->payload[2] |= 0x40; // end
			}
            memcpy(rtpPacket->payload+head_len+1, frame_buf+pos, RTP_MAX_PKT_SIZE);
            ret = rtpudpSendPacket(net_info, rtpPacket, RTP_MAX_PKT_SIZE+head_len+1);
            if(ret < 0)
	        {
	        	logerror("rtpSendPacket is error");
	            goto err;
	        }
            rtphead->rtpHeader.seq++;
			rtpPacket->rtpHeader.seq = rtphead->rtpHeader.seq;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
		}
		if (remainPktSize > 0)
        {
        	if(net_info->stream_type == RTP_H264)
        	{
	            rtpPacket->payload[0] = (naluType & 0x60) | 28;
	            rtpPacket->payload[1] = naluType & 0x1F;
	            rtpPacket->payload[1] |= 0x40; //end
        	}
			else if(net_info->stream_type == RTP_H265)
			{
				rtpPacket->payload[0] = 49 << 1;
				rtpPacket->payload[1] = 1;
	            rtpPacket->payload[2] = (naluType & 0x7E)>>1;
	            rtpPacket->payload[2] |= 0x40; // end
			}
            memcpy(rtpPacket->payload+head_len+1, frame_buf+pos, remainPktSize);
            ret = rtpudpSendPacket(net_info, rtpPacket, remainPktSize+head_len+1);
            if(ret < 0)
	        {
	        	logerror("rtpSendPacket is error");
	            goto err;
	        }

            rtphead->rtpHeader.seq++;
			rtpPacket->rtpHeader.seq = rtphead->rtpHeader.seq;
            sendBytes += ret;
        }
	}
	free(rtpPacket);
	rtpPacket = NULL;
	return sendBytes;
err:
	free(rtpPacket);
	rtpPacket = NULL;
	return -1;
}

int rtp_send_frame(struct RtpPacket* rtphead, rtp_net_info *net_info, char* frame_buf, int frameSize)
{
	int ret = 0;
	if(net_info->udp_or_tcp == TCP)
	{
		ret = rtp_tcp_send_frame(rtphead, net_info, frame_buf, frameSize);
	}
	else if(net_info->udp_or_tcp == UDP)
	{
		ret = rtp_udp_send_frame(rtphead, net_info, frame_buf, frameSize);
	}
	return ret;
}

	
