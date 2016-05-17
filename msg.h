#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>

struct msgbuf{
    long mtype;
    char mtext[10];
};

int create_msg (int key){
    return msgget(key, 0666|IPC_CREAT);
}
 
int open_msg (int key){
    
    return msgget(key, 0666);
}

int msg_rcv(int id,struct msgbuf *message,int taillemsg){
    
    return msgrcv(id, message, taillemsg, 1, 0);
}

int msg_send(int id,struct msgbuf *message,int taillemsg){
    
    return msgsnd(id, message, taillemsg, 0);
}

int remove_msg(int id){
    
    return msgctl(id, IPC_RMID, NULL);
}
