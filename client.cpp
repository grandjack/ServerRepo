/******* 客户端程序  client.c ************/  
#include <stdlib.h>  
#include <stdio.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>  
#include <netdb.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/types.h>  
#include <arpa/inet.h>  
#include<sys/time.h>  
#include<event.h> 
#include <iostream>
#include <string>
#include "message.pb.h"

using namespace std;
using namespace MessageStruct;


struct event_base *base = NULL;

#ifndef MAX_DATA_LENGTH
#define MAX_DATA_LENGTH 10240
#endif

typedef unsigned int    u_int32;
typedef unsigned short  u_int16;
typedef unsigned char   u_int8;

// 读事件回调函数 
void constructMsg(int sockfd, void *arg);

void OnWrite(int iCliFd, const u_int32 msg_type, const string &data, void *arg)
{
    
    u_int8 buf[MAX_DATA_LENGTH] = { 0 };
    u_int32 totalSize = data.size() + 2*sizeof(u_int32);
    u_int32 sendLen = 0;
    int iLen = 0;
    
    memcpy(buf, &totalSize, sizeof(u_int32));
    memcpy(buf + sizeof(u_int32), &msg_type, sizeof(u_int32));
    memcpy(buf + 2*sizeof(u_int32), data.c_str(), data.size());

    while(sendLen < totalSize)
    {
        iLen = send(iCliFd, &buf[sendLen], totalSize, 0);
        if (iLen <= 0) {
            if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
                //LOG_INFO(MODULE_COMMON, "errno EINTR, will continue");
                continue;
            } else {
                printf("the net has a error occured..");
    			struct event *pEvRead = (struct event*)arg; 
    	        event_del(pEvRead); 
    	        delete pEvRead; 
    	   
    	        close(iCliFd);
            }
        } else {
            sendLen += iLen;
        }
    }
}

void onRead(int iCliFd, short iEvent, void *arg)
{
    int iLen = 0; 
    u_int8 buf[MAX_DATA_LENGTH] = { 0 };
    u_int32 recvLen = 0;
    u_int32 leftLen = sizeof(buf);
    u_int32 totalSize = leftLen;
    bool gotHead = false;
    u_int32 msg_type = 0;

    while(recvLen < totalSize)
    {
        iLen = recv(iCliFd, &buf[recvLen], leftLen, 0);
        if (iLen > 0) {
            recvLen += iLen;
            
            if (!gotHead) {
                if (recvLen >= 2*sizeof(u_int32)) {
                    u_int32 *pTotal = (u_int32 *)(buf);
                    totalSize = *pTotal;
                    msg_type = *((u_int32 *)(buf)+1);
                    //LOG_DEBUG(MODULE_COMMON, "message total size : %u", totalSize);
                    gotHead = true;
                }
            }

            leftLen = totalSize - recvLen;
            
        } else if (iLen <= 0) {
            if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
                //LOG_INFO(MODULE_COMMON, "errno EINTR, will continue");
                continue;
            } else {
                cout << "Server Close" << endl; 
                struct event *pEvRead = (struct event*)arg; 
                event_del(pEvRead); 
                delete pEvRead; 
           
                close(iCliFd); 
                return;
            }
        }
    }
    
    buf[recvLen] = 0;   
    //cout << "Server Info:" << buf << endl;    

    cout<<"msg_type "<<msg_type<<" size "<<totalSize<<endl;
    
    string data((char *)&buf[2*sizeof(u_int32)], totalSize-2*sizeof(u_int32));

    User userInfo;
    /*handling for auth message*/

    if (userInfo.ParseFromString(data)) {
        for (int i =0; i < userInfo.user_size(); i++) {
            const user_info& person = userInfo.user(i);
            cout<<"account "<<person.account()<<endl;
            cout<<"name "<<person.name()<<endl;
            cout<<"password "<<person.password()<<endl;
            cout<<"score "<<person.score()<<endl;
            cout<<"email "<<person.email()<<endl;
        }
    }
    
    sleep(2);
    constructMsg(iCliFd, arg);

    //pThread->MessageHandle(iCliFd, (void*)&data);

    //OnWrite(iCliFd, iEvent, arg);

} 
/*
void onRead(int iCliFd, short iEvent, void *arg) 
{ 
    int iLen; 
    char buf[1500]; 
   
    iLen = recv(iCliFd, buf, 1500, 0); 
   
    if (iLen <= 0) { 
        cout << "Server Close" << endl; 
   
        struct event *pEvRead = (struct event*)arg; 
        event_del(pEvRead); 
        delete pEvRead; 
   
        close(iCliFd); 
        return; 
    } 
   
    buf[iLen] = 0; 
    cout << "Server Info:" << buf << endl; 
    sleep(2);
	onWrite(iCliFd, iEvent, NULL);

} 
void onWrite(int iCliFd, short iEvent, void *arg) 
{
	char MESSAGE[]="hello server...";

	if(-1 == (::send(iCliFd,MESSAGE,strlen(MESSAGE),0)))  
	{  
			printf("the net has a error occured..");
			struct event *pEvRead = (struct event*)arg; 
	        event_del(pEvRead); 
	        delete pEvRead; 
	   
	        close(iCliFd); 
	}  

}
*/

void constructMsg(int sockfd, void *arg)
{
    User userInfo;
    int num = 2;
    while(num--) {
        user_info  *user = userInfo.add_user();
        user->set_account("517564967@qq.com");
        user->set_name("Stern");
        user->set_password("111111");
        user->set_score(100);
        user->set_email("wei.yan1@motorolasolutions.com");

    }

    string data;
    userInfo.SerializeToString(&data);
    OnWrite(sockfd, 1, data, arg);
}

int main(int argc, char *argv[])
{  
        int sockfd;  
        struct sockaddr_in server_addr; 
        struct hostent *host;  
        int portnumber;  
  
        if((host=gethostbyname("127.0.0.1"))==NULL)  
        {  
                fprintf(stderr,"Gethostname error\n");  
                exit(1);  
        }  
  
        if((portnumber=atoi("8888"))<0)  
        {  
                fprintf(stderr,"Usage:%s hostname portnumber\a\n",argv[0]);  
                exit(1);  
        }  
  
        /* 客户程序开始建立 sockfd描述符  */  
        if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)  
        {  
                fprintf(stderr,"Socket Error:%s\a\n",strerror(errno));  
                exit(1);  
        }  
  
        /* 客户程序填充服务端的资料       */  
        bzero(&server_addr,sizeof(server_addr));  
        server_addr.sin_family=AF_INET;  
        server_addr.sin_port=htons(portnumber);  
        server_addr.sin_addr=*((struct in_addr *)host->h_addr);  
  
        /* 客户程序发起连接请求         */  
        if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)  
        {  
                fprintf(stderr,"Connect Error:%s\a\n",strerror(errno));  
                exit(1);  
        }

		base = event_base_new(); 
       
    	struct event *ev_sockfd = new event; 	
	    // 设置事件 
	    event_set(ev_sockfd, sockfd, EV_READ|EV_PERSIST, onRead, ev_sockfd); 
	    // 设置为base事件 
	    event_base_set(base, ev_sockfd);
	    // 添加事件 
	    event_add(ev_sockfd, NULL); 
		
		constructMsg(sockfd, ev_sockfd);
         
    	// 事件循环 
	    event_base_dispatch(base); 
          
        /* 结束通讯     */  
        close(sockfd); 

        //event_free(ev_sockfd);
		//delete ev_sockfd;
        exit(0);  
}  

