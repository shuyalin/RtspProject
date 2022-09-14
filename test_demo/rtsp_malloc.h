#ifndef RTSP_MALLOC_H
#define RTSP_MALLOC_H
#define BUF_MAX_SIZE     (1024)
#include <string.h>

typedef struct
{
	char *dat;
	int dat_size;
	int dat_total_size;
	int dat_free_size;
}rtsp_char_buf;

rtsp_char_buf *rtsp_malloc_char_buf(int size);
void rtsp_clean_char_buf(rtsp_char_buf *buf);
void rtsp_free_char_buf(rtsp_char_buf *buf);
int rtsp_copy_char_buf(rtsp_char_buf **buf, rtsp_char_buf *data);

#endif
