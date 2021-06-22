#include "common.h"

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#define MAX_IFS 64

int log_data(int is_server, int is_err,
             int file_line, const char *func, const char *fmt, ...) {
    int off;
    va_list ap;
    char buf[64];
    char msg[1024];
    struct timeval tv;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    gettimeofday(&tv, NULL);
    off = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S.", tm);
    snprintf(buf + off, sizeof(buf) - off, "%03d", (int)tv.tv_usec / 1000);

    // printf("%s[%s:%d] %s\n", buf, func, file_line, msg);
    // return 1;

    if (is_err) {
        printf("[%s][%s][%s:%d][errno: %d, errstr: %s] %s\n",
               is_server ? "s" : "c",
               buf, func, file_line, errno, strerror(errno), msg);
    } else {
        printf("[%s][%s][%s:%d] %s\n",
               is_server ? "s" : "c", buf, func, file_line, msg);
    }

    return 1;
}

int bring_up_net_interface(const char *ip) {
    int fd;
    struct ifreq ifreqlo;
    struct sockaddr_in sa;

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strncpy(ifreqlo.ifr_name, "lo", sizeof("lo"));
    memcpy((char *)&ifreqlo.ifr_addr, (char *)&sa, sizeof(struct sockaddr));
    ioctl(fd, SIOCSIFADDR, &ifreqlo);
    ioctl(fd, SIOCGIFFLAGS, &ifreqlo);
    ifreqlo.ifr_flags |= IFF_UP | IFF_LOOPBACK | IFF_RUNNING;
    ioctl(fd, SIOCSIFFLAGS, &ifreqlo);
    close(fd);

    LOG("bring up interface: eth0");

    if (ip != NULL && !strlen(ip)) {
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr(ip);
        fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
        strncpy(ifreqlo.ifr_name, "eth0", sizeof("eth0"));
        memcpy((char *)&ifreqlo.ifr_addr, (char *)&sa, sizeof(struct sockaddr));
        ioctl(fd, SIOCSIFADDR, &ifreqlo);
        ioctl(fd, SIOCGIFFLAGS, &ifreqlo);
        ifreqlo.ifr_flags |= IFF_UP | IFF_RUNNING;
        ioctl(fd, SIOCSIFFLAGS, &ifreqlo);
        close(fd);
    }

    LOG("list all interfaces:");

    int s;
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];

    s = socket(PF_INET, SOCK_DGRAM, 0);

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
        LOG_SYS_ERR("ioctl(SIOCGIFCONF) failed!");
        return 0;
    }

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
        LOG("interface: %s", ifr->ifr_name);

        if (ifr->ifr_addr.sa_family == AF_INET) {
            strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
            if (ioctl(s, SIOCGIFHWADDR, &ifreq) < 0) {
                LOG_SYS_ERR("SIOCGIFHWADDR(%s): %m", ifreq.ifr_name);
                return 0;
            }

            LOG("ip address: %s",
                inet_ntoa(((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr));
            LOG("device: %s -> Ethernet %02x:%02x:%02x:%02x:%02x:%02x", ifreq.ifr_name,
                (int)((unsigned char *)&ifreq.ifr_hwaddr.sa_data)[0],
                (int)((unsigned char *)&ifreq.ifr_hwaddr.sa_data)[1],
                (int)((unsigned char *)&ifreq.ifr_hwaddr.sa_data)[2],
                (int)((unsigned char *)&ifreq.ifr_hwaddr.sa_data)[3],
                (int)((unsigned char *)&ifreq.ifr_hwaddr.sa_data)[4],
                (int)((unsigned char *)&ifreq.ifr_hwaddr.sa_data)[5]);
        }
    }

    return 0;
}