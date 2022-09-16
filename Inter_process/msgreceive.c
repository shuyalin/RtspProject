/*
	int msgget(key_t key, int msgflg); 
	创建消息队列 ， 返回值为该队列号
	int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
	发送消息
	ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,int msgflg);
	接受信息，msgtyp需要指明接受的数据type
	key_t ftok(const char *pathname, int proj_id);
	为队列随机附加key，pathename为路径，id号可随意（1-255）
	int msgctl(int msqid, int cmd, struct msqid_ds *buf);//可用于消除内内核中的队列
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <errno.h>

struct msg_st {
    long int msg_type;
    char text[BUFSIZ];
};

int main ( int argc, char **argv ) {
    int msgid = -1;
    struct msg_st data;
    long int msgtype = 0;
    msgid = msgget ( ( key_t ) 1234, 0666 | IPC_CREAT );

    if ( msgid == -1 ) {
        fprintf ( stderr, "msgget failed width error: %d\n", errno );
        exit ( EXIT_FAILURE );
    }

    while ( 1 ) { /* 从队列中获取消息，直至遇到“end” */
        if ( msgrcv ( msgid, ( void * ) &data, BUFSIZ, msgtype, 0 ) == -1 ) {
            fprintf ( stderr, "msgrcv failed width erro: %d", errno );
        }

        printf ( "You wrote: %s", data.text );

        if ( strncmp ( data.text, "end", 3 ) == 0 ) {
            break;
        }
    }

    if ( msgctl ( msgid, IPC_RMID, 0 ) == -1 ) { /* 删除消息队列 */
        fprintf ( stderr, "msgctl(IPC_RMID) failed\n" );
    }

    exit ( EXIT_SUCCESS );
}
