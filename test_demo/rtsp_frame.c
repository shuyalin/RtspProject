#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rtsp_frame.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define frame_slice_size 1024*3
int copy_frame_dat(frame_info_t *frame_info, char *buf, int buffer_size)
{
	char *new_buf = NULL;
	if(frame_info->buf_len < buffer_size)
	{
		new_buf = (char *)malloc(frame_slice_size+frame_info->frame_size+1);//(frame_info->frame_size + len)
		if(new_buf)
		{
			memcpy(new_buf, frame_info->frame_buff, frame_info->frame_size);
			memcpy(new_buf+frame_info->frame_size, buf, buffer_size);
			new_buf[frame_info->frame_size+buffer_size] = 0;
			if(frame_info->frame_buff)
			{
				free(frame_info->frame_buff);
				frame_info->frame_buff = NULL;
			}
			frame_info->frame_buff = new_buf;
			frame_info->buf_len = frame_slice_size  - buffer_size;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		memcpy(frame_info->frame_buff+frame_info->frame_size, buf, buffer_size);
		frame_info->frame_buff[frame_info->frame_size+buffer_size] = 0;
		frame_info->buf_len -= buffer_size;
	}
	frame_info->frame_size += buffer_size;
	return 0;
}

unsigned int h264_parse(char *buf, int buffer_size, char *frame_buff, int *s_size, frame_info_t *frame_info, int *code_count)
{
	char *tmp_mem;
	char tmp_0;
	int i = 0;
	int ret = 0;
	log1("i:%d  buffer_size:%d", i, buffer_size);
	while(i < buffer_size-3)
	{
		tmp_mem = buf + i;
 
		if (((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x00) && (*(tmp_mem+3)==0x01)) ||  //NAL前的起始码00000001||000001
			((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x01)))
 
		{			
			if((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x00) && (*(tmp_mem+3)==0x01))
			{
				tmp_0 = *(tmp_mem+4) & 0x1f ; //获取nalu
				*code_count = 4;
			}
 			else
 			{
				tmp_0 = *(tmp_mem+3) & 0x1f ; //获取nalu
				*code_count = 3;
			}
			
			
			if (tmp_0==0x06 || tmp_0==0x07 || tmp_0==0x08)  //=== SEI，SPS, PPS
			{
				if(frame_info->nalu_count == 1)
				{
					frame_info->nalu_count++;
					ret = 2;
					copy_frame_dat(frame_info, buf, i);
					*s_size = buffer_size - i;//剩余部分 尺寸
					log1("frame_buff:%p, buf:%p, size:%d", frame_buff, buf+i, *s_size);
					if(*s_size > 0)
					{
						memcpy(frame_buff, buf+i, *s_size);
						frame_buff[*s_size] = 0;
					}
					return ret;
				}
			}
			
			if (tmp_0==0x01 || tmp_0==0x05)//P 或者 I
			{	
				frame_info->nalu_count++;
				if(frame_info->nalu_count == 1)
					ret = 1;//已经找到起始nalu了
				else if(frame_info->nalu_count == 2)//如果第二次读到了P或者I
				{
					copy_frame_dat(frame_info, buf, i);
					
					*s_size = buffer_size - i;
					log1("frame_buff:%p, buf:%p, size:%d", frame_buff, buf+i, *s_size);
					if(*s_size > 0)
					{	
						memcpy(frame_buff, buf+i, *s_size);
						//frame_buff[*s_size] = 0;
					}
					ret = 2;//已经找到结束nalu了
					return ret;
				}
					
				if(tmp_0==0x01)
					frame_info->frame_type = 1;
				else
					frame_info->frame_type = 7;
				log1("frame_info->frame_type:%d", frame_info->frame_type);
			}
			i += 2;
		}
		else
		{
			i++;
		}		
		
	}
	copy_frame_dat(frame_info, buf, i);
	log1("buffer_size:%d i:%d", buffer_size, i);
	*s_size = buffer_size - i;
	//*s_size = buffer_size;
	if(*s_size > 0)
	{
		memcpy(frame_buff, buf+i, *s_size);
		//frame_buff[*s_size] = 0;
	}
	log1("frame_buff:%p, buf:%p, size:%d", frame_buff, buf+i, *s_size);
	
	return ret;
}

unsigned int h265_parse(char *buf, int buffer_size, char *frame_buff, int *s_size, frame_info_t *frame_info, int *code_count)
{
	char *tmp_mem;
	char tmp_0;
	int i = 0;
	int ret = 0;
	int aud_i = 0;
	log1("i:%d  buffer_size:%d", i, buffer_size);
	while(i < buffer_size - 3)
	{
		tmp_mem = buf + i;
 
		if (((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x00) && (*(tmp_mem+3)==0x01)) ||  //NAL前的起始码00000001||000001
			((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x01)))
 
		{			
			if((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x00) && (*(tmp_mem+3)==0x01))
			{
				tmp_0 = (*(tmp_mem+4) & 0x7E)>>1 ; //获取nalu
				*code_count = 4;
			}
 			else if((*(tmp_mem)==0x00) && (*(tmp_mem+1)==0x00) && (*(tmp_mem+2)==0x01))
 			{
				tmp_0 = (*(tmp_mem+3) & 0x7E)>>1 ; //获取nalu
				*code_count = 3;
			}
			
			log1("h265>>>>>>>nalu:%d  tmp_mem+4:%x", tmp_0, *(tmp_mem+4));
			if ((tmp_0 >= 32 && tmp_0 <= 39)/* && tmp_0 != 35 && tmp_0 != 39*/)
			{
				/*if(tmp_0==0x07)
				{		
					if(frame_info->sps_flag == 0)
					{
						frame_info->sps_flag = 1;
						if(frame_info->sps_dat == NULL)
							frame_info->sps_dat = (char *)malloc(frame_slice_size);
					}
				}
				else
				{
					if(frame_info->sps_flag == 1)
					{	
						copy_sps_frame_dat(frame_info, buf, i);
						frame_info->sps_flag = 0;
					}
				}*/
				if(frame_info->nalu_count == 1)
				{
					frame_info->nalu_count++;
					ret = 2;
					copy_frame_dat(frame_info, buf, i);
					*s_size = buffer_size - i;//剩余部分 尺寸
					logs("frame_buff:%p, buf:%p, size:%d", frame_buff, buf+i, *s_size);
					if(*s_size > 0)
					{
						memcpy(frame_buff, buf+i, *s_size);
						frame_buff[*s_size] = 0;
					}
					return ret;
				}
			}
			if (tmp_0 <= 31)//P 或者 I,B
			{	
				frame_info->nalu_count++;
				if(frame_info->nalu_count == 1)
					ret = 1;//已经找到起始nalu了
				else if(frame_info->nalu_count == 2)//如果第二次读到了P或者I
				{
					copy_frame_dat(frame_info, buf, i);
					
					*s_size = buffer_size - i;
					log1("frame_buff:%p, buf:%p, size:%d", frame_buff, buf+i, *s_size);
					if(*s_size > 0)
					{	
						memcpy(frame_buff, buf+i, *s_size);
						frame_buff[*s_size] = 0;
					}
					ret = 2;//已经找到结束nalu了
					return ret;
				}
					
				if((tmp_0 <= 9 && tmp_0> 0))
					frame_info->frame_type = 1;
				else if(tmp_0 <= 23 && tmp_0>= 16)
					frame_info->frame_type = 7;
				else if(tmp_0 == 0)
					frame_info->frame_type = 0;
				log1("frame_info->frame_type:%d", frame_info->frame_type);
			}
			i += 2;
			if(tmp_0 == 35 || tmp_0 == 39)
			{
				aud_i+=2;
			}
		}
		else
		{
			i++;
			if(tmp_0 == 35 || tmp_0 == 39)
			{
				aud_i++;
			}
		}		
		
	}
	//i-=aud_i;
	copy_frame_dat(frame_info, buf, i);
	log1("buffer_size:%d i:%d", buffer_size, i);
	*s_size = buffer_size - i;
	if(*s_size > 0)
	{
		memcpy(frame_buff, buf+i, *s_size);
		frame_buff[*s_size] = 0;
	}
	log1("frame_buff:%p, buf:%p, size:%d", frame_buff, buf+i, *s_size);
	
	return ret;
}

void read_frame(char *name, struct RtpPacket* rtpPacket, rtp_net_info *net_info)
{
	int fd = open(name, O_RDWR);
	if(fd < 0)
	{
		logerror("open %s is failed", name);
		return;
	}
	logs("open is success");
	int ret = 0;
	int len = 0;
	char buf[frame_slice_size] = {0};
	char frame_buff[frame_slice_size] = {0};
	//int frame_size = 0;
	frame_info_t frame_info;
	int s_size = 0;
	int play_flag = 1;
	int code_count = 0;//计算每一帧的最开始是001还是0001的字节数
	//int t = open("./haha", O_CREAT | O_RDWR);
	while(1)
	{
		memset(&frame_info, 0, sizeof(frame_info_t));
		frame_info.frame_buff = (char *)malloc(frame_slice_size);
		if(!frame_info.frame_buff)
		{
			logs("malloc is error");
			return;
		}
		frame_info.buf_len = frame_slice_size;
		do
		{
			memset(buf, 0, frame_slice_size);
			log1("s_size:%d", s_size);
			if(s_size > 0 )
			{
				memcpy(buf, frame_buff, s_size);
				//buf[s_size] = 0;
			}
			else
			{
				s_size = 0;
			}
			memset(frame_buff, 0, frame_slice_size);
			if(((frame_slice_size - s_size) <= 1024) || (!play_flag && s_size))
			{
				len = 0;
				if(net_info->stream_type == RTP_H264)
					ret = h264_parse(buf, len+s_size, frame_buff, &s_size, &frame_info, &code_count);
				else if(net_info->stream_type == RTP_H265)
					ret = h265_parse(buf, len+s_size, frame_buff, &s_size, &frame_info, &code_count);
			}
			else if(play_flag == 1)
			{
				len = read(fd, buf+s_size, 1024);
				if(len > 0)
				{				
					if(net_info->stream_type == RTP_H264)
						ret = h264_parse(buf, len+s_size, frame_buff, &s_size, &frame_info, &code_count);
					else if(net_info->stream_type == RTP_H265)
						ret = h265_parse(buf, len+s_size, frame_buff, &s_size, &frame_info, &code_count);
				}
				else if(!len)
				{
					logs("read frame end!!!\n");	
					play_flag = 0;
					if(net_info->stream_type == RTP_H264)
						ret = h264_parse(buf, len+s_size, frame_buff, &s_size, &frame_info, &code_count);
					else if(net_info->stream_type == RTP_H265)
						ret = h265_parse(buf, len+s_size, frame_buff, &s_size, &frame_info, &code_count);				
				}
				else 
				{
					logs("read frame error %d  errno:%d!!!\n", len , errno);
					close(fd);
					//sms_handle->login_status = 0;
					return;
				}
			}
			else
			{
				logs("send frame end!!!\n");
				//close(fd);
				//sms_handle->login_status = 0;
				//return;//文件读取完毕
				break;
			}
		}
		while(ret != 2);//读到完整一帧就结束
		
		
		log1("frame_info.frame_size :%d, code_count:%d", frame_info.frame_size, code_count);
		if(frame_info.frame_size > 0)
		{
			//write(t, frame_info.frame_buff, frame_info.frame_size);
			rtp_send_frame(rtpPacket, net_info, frame_info.frame_buff+code_count, frame_info.frame_size-code_count);
			rtpPacket->rtpHeader.timestamp += 90000/25;
		}
		
		if(frame_info.frame_buff)
		{
			free(frame_info.frame_buff);
			frame_info.frame_buff = NULL;
		}
		if(!play_flag && !s_size)
		{
			logs("play is end");
			break;
		}
		//sleep(1);
		//usleep(1000 *270);
		usleep(1000 *40);
	}
	//sms_handle->login_status = 0;
	close(fd);
}

