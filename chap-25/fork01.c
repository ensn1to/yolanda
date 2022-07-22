#include "lib/common.h"

#define MAX_LINE 4096

char
rot13_char(char c) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

void child_run(int fd) {
    char outbuf[MAX_LINE + 1];
    size_t outbuf_used = 0;
    ssize_t result;

    while (1) {
        char ch;
        result = recv(fd, &ch, 1, 0);
        if (result == 0) {
            break;
        } else if (result == -1) {
            perror("read");
            break;
        }

        /* We do this test to keep the user from overflowing the buffer. */
        if (outbuf_used < sizeof(outbuf)) {
            outbuf[outbuf_used++] = rot13_char(ch);
        }

        if (ch == '\n') {
            send(fd, outbuf, outbuf_used, 0);
            outbuf_used = 0;
            continue;
        }
    }
}


void sigchld_handler(int sig) {
    // waitpid回收已终止子进程
    // WNOHANG： 即使还有未终止的子进程也不要阻塞在waitpid上。注意这里不可以使用wait，因为wait函数在有未终止子进程的情况下，没有办法不阻塞
    while (waitpid(-1, 0, WNOHANG) > 0);
    return;
}

int main(int c, char **v) {
    int listener_fd = tcp_server_listen(SERV_PORT);
    // 信号处理函数，用来回收子进程资源
    signal(SIGCHLD, sigchld_handler);
    while (1) {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        int fd = accept(listener_fd, (struct sockaddr *) &ss, &slen);
        if (fd < 0) {
            error(1, errno, "accept failed");
            exit(1);
        }

        if (fork() == 0) { 
            close(listener_fd);  // 子进程关闭fork出来的监听套接字
            child_run(fd);
            exit(0);
        } else {
            close(fd); // 父进程关闭链接套接字
        }
    }

    return 0;
}