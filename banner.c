
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "lcddriver.h"

int g_quit = 0;

void get_ip(char *ip_buf, int len)
{

    int fd;
    struct ifreq ifr;
    int ret;

    snprintf(ip_buf, len, "999.999.999.999");

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd<0) return;

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    ret = ioctl(fd, SIOCGIFADDR, &ifr);
    if(ret<0) {
        close(fd);
        return;
    }

    close(fd);

    /* display result */
    snprintf(ip_buf, len, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    return;
}

void get_fortune(char *buf, int len)
{
    static int counter = 0;

    counter++;
    snprintf(buf, len, "(null)");

    if(1==counter%3) {
        // return current date time
        time_t tt;
        struct tm ttm;

        tt = time(NULL); // get current time
        localtime_r(&tt, &ttm); // convert to localtime

        strftime(buf, len, "%m/%d %H:%M:%S", &ttm);
    }
    else {
        // return fortune cookie
        FILE *fp = NULL;
        fp = popen("/usr/games/fortune", "r");
        if(NULL==fp) {
            return;
        }

        fgets(buf, len, fp);
        fclose(fp);

        // remove ending new line
        if(buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = 0;
    }
    return;
}

void signal_handler(int signo)
{
    printf("got signal %d, quit\n", signo);
    g_quit = 1;
}

void msleep(int mili_sec)
{
    //TODO: use select
    usleep(mili_sec*1000);
}

int main(int argc, char **argv)
{
    char *dev = "/dev/i2c-2";
    int fd = open(dev, O_RDWR);
    int ret;
    uint8_t address = 0x27;
    char ip[32];
    char fortune[128];
    int scroll_delay = 200;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if(fd<0) {
        printf("open: %s\n", strerror(errno));
        exit(1);
    }

    ret = ioctl(fd, I2C_SLAVE, address);
    if(ret<0) {
        printf("set i2c address: %s\n", strerror(errno));
        exit(1);
    }

    lcd_init(fd);

    while(!g_quit) {
        char tmp_fortune[16+1];
        int i;

        get_ip(ip, sizeof(ip));
        get_fortune(fortune, sizeof(fortune));
        
        //strncpy(tmp_fortune, fortune, 16);
        snprintf(tmp_fortune, sizeof(tmp_fortune), "%s", fortune);

        printf("display fortune: %s\n", fortune);
        lcd_display_string(fd, tmp_fortune, 1);
        printf("display ip: %s\n", ip);
        lcd_display_string(fd, ip, 2);
        msleep(900); 

        // no scrolling if fit in one line
        if( strlen(fortune)<=16) {
            msleep(2000);
            continue;
        }

        // do scrolling text if unable to fit in one line
        printf("scroll start: %s\n", fortune);
        for(i=1; i<=strlen(fortune) && !g_quit ; ++i) {
            // shift the string to the left by 1 character
            snprintf(tmp_fortune, sizeof(tmp_fortune), "%s", fortune+i); 
       
            lcd_display_string(fd, tmp_fortune, 1);
            msleep(scroll_delay);
        }
        printf("scroll end: %s\n", fortune);

    }

    lcd_clear(fd);
    close(fd);
    return 0;
}

