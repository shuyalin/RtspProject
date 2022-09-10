#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rtsp_malloc.h"
rtsp_char_buf *rtsp_malloc_char_buf(int size)
{
	rtsp_char_buf *p = malloc(sizeof(rtsp_char_buf));
	if(p)
	{
		memset(p, 0, sizeof(rtsp_char_buf));
		p->dat = malloc(size);
		if(p->dat)
		{
			p->dat_size = 0;
			p->dat_total_size = size;
			p->dat_free_size = size;
		}
		else
		{
			free(p);
			printf("rtsp_malloc_char_buf p->dat is malloc error\n");
			return NULL;
		}
	}
	else
	{
		printf("rtsp_malloc_char_buf p is malloc error\n");
		return NULL;
	}
	return p;
}

void rtsp_clean_char_buf(rtsp_char_buf *buf)
{
	if(buf)
	{
		memset(buf->dat, 0, buf->dat_total_size);
		buf->dat_free_size = buf->dat_total_size;
		buf->dat_size = 0;
	}
}

void rtsp_free_char_buf(rtsp_char_buf *buf)
{
	if(buf)
	{
		if(buf->dat)
		{
			free(buf->dat);
			buf->dat = NULL;
		}
		free(buf);
		buf = NULL;
	}
}

int rtsp_copy_char_buf(rtsp_char_buf **buf, rtsp_char_buf *data)
{
	if(data== NULL || data->dat == NULL)
	{
		printf("rtsp_copy_char_buf: data or data->dat is null\n");
		return -1;
	}
	if(NULL == (*buf))
	{
		(*buf) = rtsp_malloc_char_buf(BUF_MAX_SIZE);
		if(NULL == (*buf))
		{
			printf("rtsp_malloc_char_buf is error\n");
			return -1;
		}
	}
	//printf("data->dat_size:%d dat:%s\n", data->dat_size, data->dat);
	if((*buf)->dat_free_size>= data->dat_size)
	{
		memcpy((*buf)->dat+(*buf)->dat_size, data->dat, data->dat_size);
		(*buf)->dat_size += data->dat_size;
		(*buf)->dat_free_size -= data->dat_size;
	}
	else
	{
		rtsp_char_buf *new_buf = rtsp_malloc_char_buf((*buf)->dat_total_size+data->dat_size-(*buf)->dat_free_size+BUF_MAX_SIZE);
		if(new_buf)
		{
			memcpy(new_buf->dat, (*buf)->dat, (*buf)->dat_size);
			memcpy(new_buf->dat+(*buf)->dat_size, data->dat, data->dat_size);
			new_buf->dat_size = (*buf)->dat_size + data->dat_size;
			new_buf->dat_free_size -= new_buf->dat_size;
			rtsp_free_char_buf(*buf);
			(*buf) = new_buf;
		}
		else
		{
			printf("rtsp_malloc_char_buf is error\n");
			return -1;
		}
	}
	return 0;
}

