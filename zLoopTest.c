#include "zLoopTest.h"
 
int nFd = -1;
struct termios stNew;
struct termios stOld;
 
//Open Port & Set Port
int SerialInit()
{
    nFd = open(DEVICE, O_RDWR|O_NOCTTY|O_NDELAY);
    if(-1 == nFd)
    {
        perror("Open Serial Port Error!\n");
        return -1;
    }
    if( (fcntl(nFd, F_SETFL, 0)) < 0 )
    {
        perror("Fcntl F_SETFL Error!\n");
        return -1;
    }
    if(tcgetattr(nFd, &stOld) != 0)
    {
        perror("tcgetattr error!\n");
        return -1;
    }
 
    stNew = stOld;
    cfmakeraw(&stNew);//将终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
 
    //set speed
    cfsetispeed(&stNew, BAUDRATE);//115200
    cfsetospeed(&stNew, BAUDRATE);
 
    //set databits
    stNew.c_cflag |= (CLOCAL|CREAD);
    stNew.c_cflag &= ~CSIZE;
    stNew.c_cflag |= CS8;
 
    //set parity
    stNew.c_cflag &= ~PARENB;
    stNew.c_iflag &= ~INPCK;
 
    //set stopbits
    stNew.c_cflag &= ~CSTOPB;
    stNew.c_cc[VTIME]=0;    //指定所要读取字符的最小数量
    stNew.c_cc[VMIN]=1; //指定读取第一个字符的等待时间，时间的单位为n*100ms
                //如果设置VTIME=0，则无字符输入时read（）操作无限期的阻塞
    tcflush(nFd,TCIFLUSH);  //清空终端未完成的输入/输出请求及数据。
    if( tcsetattr(nFd,TCSANOW,&stNew) != 0 )
    {
        perror("tcsetattr Error!\n");
        return -1;
    }
 
    return nFd;
}

void SerialDestroy() 
{
    close(nFd);
    nFd = -1;
    return ;
}

void * readEndPoint(void *arg)
{
    int nRet = 0;
    char buf[SIZE] = {0,};
    fd_set  rfds;
    struct timeval tv;
    int ret = -1;

    while(strcmp("@bye@", buf)) {
        bzero(buf, SIZE);
        FD_ZERO(&rfds);
        FD_SET(nFd, &rfds);
        tv.tv_sec = 30;
        tv.tv_usec = 00;
        ret = select (nFd + 1, &rfds, NULL, NULL, &tv);
        if (FD_ISSET(nFd, &rfds)) {
            nRet = read(nFd, buf, SIZE);
            if (-1 == nRet) {
                perror("Read Data Error!\n");
                break;
            }
        
            if (0 < nRet) {
                buf[nRet] = 0;
                printf("\t[ZigBee] >>>>>: %s\n", buf);
            }
        }

        if (!ret) {
            printf("\n\tRecv timeout > 30s.\n");
        }
    }
    printf("Read EndPoint exit!\n");

    return NULL;
}

void createRandomString(char *buf, unsigned int len)
{
    if (!buf) {
        printf("Error: Param buf is NULL!");
        return ;
    }

    srand((int)time(NULL));
    for (unsigned int i = 0; i < len; ++i) {
        buf[i] = 'A'+ rand()%('Z'-'A');
    }
    buf[len] = '\0';
    return;
}

#define MAX_TRANSPORT_NUM (255)
void * writeEndPoint(void *arg)
{
    char buf[SIZE] = {0,};
    unsigned int len = 0;
    while (strcmp("@bye@", buf)) {
        bzero(buf, SIZE);
        scanf("%u", &len);
        if (len > SIZE - 1) {
            strcpy(buf, "@bye@");
        }
        else {
            createRandomString(buf, len);
        }
        write(nFd, buf, strlen(buf));
        printf("%s :>>>> [ZigBee], size: %d.\n", buf, strlen(buf));
    }
    
    printf("Write EndPoint exit!\n");
    return NULL;
}

void mainLoop() 
{
    pthread_t rtid, wtid;
    pthread_create(&rtid, NULL, readEndPoint, NULL);
    pthread_create(&wtid, NULL, writeEndPoint, NULL);
    pthread_join(wtid, NULL);
    pthread_join(rtid, NULL);

    printf("ZigBee test program exit...!\n");

    return ;
}

int main(int argc, char **argv)
{
    if( SerialInit() == -1 )
    {
        perror("SerialInit Error!\n");
        return -1;
    }
    mainLoop();

    SerialDestroy();
 
    return 0;
}
