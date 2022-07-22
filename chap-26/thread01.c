#include "lib/common.h"

extern void loop_echo(int);

void thread_run(void *arg) {
    // 分离线程
    pthread_detach(pthread_self());
    int fd = (int) arg;
    loop_echo(fd);
}

int main(int c, char **v) {
    int listener_fd = tcp_server_listen(SERV_PORT);
    pthread_t tid;

    while (1) {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        int fd = accept(listener_fd, (struct sockaddr *) &ss, &slen);
        if (fd < 0) {
            error(1, errno, "accept failed");
        } else {
            // 为每个链接创建一个线程
            pthread_create(&tid, NULL, &thread_run, (void *) fd);
        }
    }

    return 0;
}