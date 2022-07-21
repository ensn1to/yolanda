#include "lib/common.h"

#define INIT_SIZE 128

int main(int argc, char **argv) {
    int listen_fd, connected_fd;
    int ready_number;
    ssize_t n;
    char buf[MAXLINE];
    struct sockaddr_in client_addr;

    listen_fd = tcp_server_listen(SERV_PORT);

    //初始化pollfd数组，这个数组的第一个元素是listen_fd，其余的用来记录将要连接的connect_fd
    // INIT_SIZE指定待监听fd的大小
    struct pollfd event_set[INIT_SIZE];
    
    // 把监听套接字加入到数组，当listen_fd上有新连接时可被内核检测到
    event_set[0].fd = listen_fd;
    event_set[0].events = POLLRDNORM; //  Equivalent to POLLIN.

    // 用-1表示这个数组位置还没有被占用，poll函数会忽略这些fd
    int i;
    for (i = 1; i < INIT_SIZE; i++) {
        event_set[i].fd = -1;
    }

    for (;;) {
        // 事件监测
        // 传入INIT_SIZE是因为poll可以计算INIT_SIZE的负值fd
        if ((ready_number = poll(INIT_SIZE, INIT_SIZE, -1)) < 0) {
            error(1, errno, "poll failed ");
        }

        // 检测到listen_fd上有连接事件发生
        if (event_set[0].revents & POLLRDNORM) {
            socklen_t client_len = sizeof(client_addr);
            // 建立连接
            connected_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_len);

            //找到一个可以记录该连接套接字的位置
            for (i = 1; i < INIT_SIZE; i++) {
                if (event_set[i].fd < 0) {
                    event_set[i].fd = connected_fd;
                    event_set[i].events = POLLRDNORM;
                    break;
                }
            }

            if (i == INIT_SIZE) {
                error(1, errno, "can not hold so many clients");
            }

            if (--ready_number <= 0)
                continue;
        }

        for (i = 1; i < INIT_SIZE; i++) {
            int socket_fd;
            if ((socket_fd = event_set[i].fd) < 0)
                continue;
            
            if (event_set[i].revents & (POLLRDNORM | POLLERR)) { // 连接描述符有数据或发生错误
                if ((n = read(socket_fd, buf, MAXLINE)) > 0) {
                    if (write(socket_fd, buf, n) < 0) {
                        error(1, errno, "write error");
                    }
                } else if (n == 0 || errno == ECONNRESET) { // 收到EOF和连接重置，关闭连接
                    close(socket_fd);
                    event_set[i].fd = -1;
                } else {
                    error(1, errno, "read error");
                }

                if (--ready_number <= 0)
                    break;
            }
        }
    }
}
