#ifndef __MAIN_THREAD_HEAD__
#define __MAIN_THREAD_HEAD__

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <pthread.h>
#include <iostream>
#include <event.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include "utils.h"

using namespace std;

class WorkThread;
class UserSession;
class GameHall;

class MainThread
{
public:
    static MainThread *mainThread;
    static MainThread *GetMainThreadObj();
    static void DestructMainThreadObj();
    
    static void AcceptHandler(const int fd, const short which, void *arg);
    
    void CreateWorkTheads(int num = 1);
    void IniServerEnv(int port = 8888);
    void Run();

    WorkThread *GetOneThread();
    void PushSessionBack(UserSession *session);
    UserSession* PopFrontSession();
    const int GetUnhandledSessionsSize();
    GameHall* GetGameHall(const u_int32 id);
    int GetThreadsNum() const;
    WorkThread *GetOneThreadByIndex(u_int8 index);
    
private:
    MainThread();
    MainThread(const MainThread &);
    MainThread& operator = (const MainThread &);
    ~MainThread();
    
public:    
    struct event_base *mainEventBase;
private:
    vector<WorkThread*>threadList;
    u_int8 lastSelectIndex;
    list<UserSession*>sessionList;
    struct event listenEvent;
    pthread_t thread_id;
    u_int32 threadNum;
    u_int32 listenFd;
    struct sockaddr_in svrAddr;   
    pthread_mutex_t init_lock;
    pthread_cond_t init_cond;    
    std::map<u_int32, GameHall*> GameHallMap;

public:
    static const u_int32 gameHallMaxNum = 5;
    static const u_int32 chessBoardMaxNum = 60;
};

#endif /*__MAIN_THREAD_HEAD__*/
