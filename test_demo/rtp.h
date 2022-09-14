#ifndef _RTP_H_
#define _RTP_H_
#include <stdint.h>
#include <sys/epoll.h>
#define RTP_VESION              2

#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97

#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400


#define TCP 1
#define UDP 0

#define RTP_H264 0
#define RTP_H265 1
/*
 *
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   :                             ....                              :
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
struct RtpHeader
{
    /* byte 0 */
    uint8_t csrcLen:4;
    uint8_t extension:1;
    uint8_t padding:1;
    uint8_t version:2;

    /* byte 1 */
    uint8_t payloadType:7;
    uint8_t marker:1;
    
    /* bytes 2,3 */
    uint16_t seq;
    
    /* bytes 4-7 */
    uint32_t timestamp;
    
    /* bytes 8-11 */
    uint32_t ssrc;
};

struct RtpPacket
{
    struct RtpHeader rtpHeader;
    uint8_t payload[0];
};

typedef struct 
{
	char tcp_head[4];
    struct RtpHeader rtpHeader;
    uint8_t payload[0];
}tcp_RtpPacket;

typedef struct
{
	int fd;
	int port;
	char ip[64];
	int udp_or_tcp;// 0:udp 1:tcp
	int stream_type;//0: H264 1:H265
	int epoll_out_event_flag;
}rtp_net_info;

typedef struct
{
	int fd;
	struct epoll_event event;
	int *epoll_out_event_flag;
}rtp_epoll_out_send;

void rtpHeaderInit(struct RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
                    uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc);
int rtpudpSendPacket(rtp_net_info *net_info, struct RtpPacket* rtpPacket, uint32_t dataSize);
int rtptcpSendPacket(rtp_net_info *net_info, tcp_RtpPacket* rtpPacket, uint32_t dataSize);
int rtp_send_frame(struct RtpPacket* rtpPacket, rtp_net_info *net_info, char* frame_buf, int frameSize);
#endif 