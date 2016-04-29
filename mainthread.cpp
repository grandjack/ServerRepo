#include "mainthread.h"
#include "thread.h"
#include "usersession.h"
#include "debug.h"
#include <netinet/tcp.h>
#include <errno.h>
#include "chessboard.h"
#include "findpwd.h"

MainThread* MainThread::mainThread = NULL;

MainThread::~MainThread()
{
    pthread_mutex_destroy(&init_lock);
    pthread_cond_destroy(&init_cond);

    std::map<u_int32, GameHall *>::iterator iter;
    for ( iter = GameHallMap.begin(); iter != GameHallMap.end(); iter++ ) {
        GameHall *pGameHall = iter->second;
        if (pGameHall) {
            delete pGameHall;
        }
    }

    GameHallMap.clear();

    FindPwdViaEmail::Destrory();

    DestrorySessionsFromPool(0, true);

    event_base_free(mainEventBase);
    ::close(listenFd);
}
MainThread::MainThread() : lastSelectIndex(0),threadNum(0)
{
    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);
    
    thread_id = pthread_self();
    mainEventBase = event_base_new();

    for (u_int32 i=1; i<= gameHallMaxNum; i++) {
        GameHall *pGameHall = new GameHall(i, chessBoardMaxNum);
        GameHallMap.insert(pair<u_int32, GameHall*>(i, pGameHall));
    }
}

GameHall* MainThread::GetGameHall(const u_int32 id)
{
    GameHall *gameHall = NULL;
    
    if (id < 1 || id > gameHallMaxNum) {
        LOG_ERROR(MODULE_COMMON, "Invalid Gamehall ID[%d]", id);
        return gameHall;
    }

    try {
        gameHall = static_cast<GameHall *>(GameHallMap.find(id)->second);
    } catch(exception &e)
    {
        LOG_ERROR(MODULE_COMMON, "Can NOT get the game hall.");
        return NULL;
    }

    return gameHall;
}

MainThread* MainThread::GetMainThreadObj()
{
    if (mainThread == NULL)
    {
        mainThread = new MainThread();
    }

    return mainThread;
}

void MainThread::DestructMainThreadObj()
{
    if (mainThread != NULL)
    {
        for(vector<WorkThread *>::iterator it = mainThread->threadList.begin(); 
            it != mainThread->threadList.end(); it++) {
            try {
                WorkThread *thread = *it;
                thread->NotifyThread(COMMAND_NOTIFY_QUIT_LOOP);
                thread->join();
                delete thread;
            } catch (const exception & e) {
                LOG_ERROR(MODULE_COMMON, "catch a exception!");
            }
        }
        
        delete mainThread;
    }
}
void MainThread::CreateWorkTheads(int num)
{
    for(int i=0; i < num; ++i) {
        WorkThread *thread = new WorkThread();
        if (!thread->start()) {
            LOG_ERROR(MODULE_COMMON, "Can NOT start thread!");
        }

        threadList.push_back(thread);
    }

    threadNum = num;
} 

void MainThread::IniServerEnv(int port)
{
    int error = -1;
    struct linger ling = {0, 0};
    int flags =1;
    
    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_addr.s_addr = INADDR_ANY;
    svrAddr.sin_port = htons(port);

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0)
    {
        LOG_ERROR(MODULE_COMMON, "socket failed");
    }
    
    evutil_make_socket_nonblocking(listenFd);
    evutil_make_listen_socket_reuseable(listenFd);

    error = setsockopt(listenFd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    if (error != 0)
        LOG_ERROR(MODULE_COMMON, "setsockopt SO_KEEPALIVE failed.");

    error = setsockopt(listenFd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (error != 0)
        LOG_ERROR(MODULE_COMMON, "setsockopt SO_LINGER failed.");

    error = setsockopt(listenFd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    if (error != 0)
        LOG_ERROR(MODULE_COMMON, "setsockopt TCP_NODELAY failed.");

    if (bind(listenFd, (struct sockaddr*)&svrAddr, sizeof(svrAddr)) < 0)
    {
        if (errno != EADDRINUSE) {
            LOG_ERROR(MODULE_COMMON, "bind() failed");
        }
        ::close(listenFd);
        return;
    }
    
    if (listen(listenFd, 32) < 0)
    {
        LOG_ERROR(MODULE_COMMON, "listen failed");
        ::close(listenFd);
        return;
    }

    AddEventForBase(mainEventBase, &listenEvent, listenFd, EV_READ | EV_PERSIST, AcceptHandler, (void*)this);

}

void MainThread::AcceptHandler(const int fd, const short which, void *arg)
{
    WorkThread *work = NULL;
    MainThread * pThread = static_cast<MainThread *>(arg);
    int iCliFd = -1; 
    struct sockaddr_in sCliAddr;

    socklen_t iSinSize = sizeof(sCliAddr); 
    iCliFd = accept(fd, (struct sockaddr*)&sCliAddr, &iSinSize);
    if (iCliFd < 0)
    {
        LOG_ERROR(MODULE_COMMON, "accept failed");
        return;
    }

    LOG_DEBUG(MODULE_COMMON,"############### Got a new connection: %d [%s:%d] #################### ",iCliFd, inet_ntoa(sCliAddr.sin_addr), sCliAddr.sin_port);

    evutil_make_socket_nonblocking(iCliFd);

    work = pThread->GetOneThread();
    work->NotifyThread(COMMAND_NOTIFY_ADD_EVENT, iCliFd);
}

WorkThread *MainThread::GetOneThread()
{
    u_int8 index = lastSelectIndex;
    lastSelectIndex = (lastSelectIndex+1) % threadList.size();
    return threadList[index];
}

int MainThread::GetThreadsNum() const
{
    return threadList.size();
}

WorkThread *MainThread::GetOneThreadByIndex(u_int8 index)
{
    WorkThread *thread = NULL;
    
    if ((index >= 0) && (index < threadList.size())) {
        thread = threadList[index];
    }

    return thread;
}

const int MainThread::GetUnhandledSessionsSize()
{
    int size = 0;
    pthread_mutex_lock(&init_lock);
    size = sessionList.size();
    pthread_mutex_unlock(&init_lock);
    return size;
}

void MainThread::CreatSessionsPool(u_int16 num)
{
    while(num--) {
        UserSession *session = new UserSession();
        sessionList.push_back(session);
    }
}

void MainThread::DestrorySessionsFromPool(u_int16 num, bool del_all)
{
    UserSession* session = NULL;
    list<UserSession*>::iterator iter;

    if (del_all) {
        for ( iter = sessionList.begin(); iter != sessionList.end(); iter++ ) {
            session = static_cast<UserSession*>(*iter);
            if (session != NULL) {
                session->DestructResource();
                if (session->buffer != NULL) {
                    free(session->buffer);
                    session->buffer = NULL;
                    session->buf_size = 0;
                }
                delete session;
            }
        }
        sessionList.clear();

    } else if (num > 0) {
        num = (sessionList.size() < num) ? sessionList.size() : num;
        while(num--) {
            session = sessionList.back();
            if (session != NULL) {
                session->DestructResource();
                if (session->buffer != NULL) {
                    free(session->buffer);
                    session->buffer = NULL;
                    session->buf_size = 0;
                }
                delete session;
            }
            sessionList.pop_back();
        }
    }
}

void MainThread::RecycleSession(UserSession *session)
{
    if (session != NULL) {
        session->DestructResource();
        pthread_mutex_lock(&init_lock);
        sessionList.push_front(session);
        if (sessionList.size() >= 2*connectionMaxStep) {
            DestrorySessionsFromPool(connectionMaxStep);
        }
        pthread_mutex_unlock(&init_lock);
    }
}

UserSession* MainThread::GetOneSessionFromPool(void)
{
    UserSession* session = NULL;

    pthread_mutex_lock(&init_lock);
    if (sessionList.size() > 0) {
        session = sessionList.front();
    } else {
        CreatSessionsPool();
        session = sessionList.front();
    }
    sessionList.pop_front();
    pthread_mutex_unlock(&init_lock);

    return session;

}

void MainThread::Run()
{
    event_base_dispatch(mainEventBase);
}

