#ifndef __WORKER_THREAD_HEAD__
#define __WORKER_THREAD_HEAD__

#include <pthread.h>
#include <event.h>
#include <string>
#include <map>
#include "mysqldb.h"
#include "utils.h"
#include "state.h"


#define COMMAND_NOTIFY_ADD_EVENT    0x10
#define COMMAND_NOTIFY_QUIT_LOOP    0x11

class Thread  
{  
private:
    pthread_t tid;
    int threadStatus;
    
    pthread_mutex_t init_lock;
    pthread_cond_t init_cond;
    bool beInitialed;
    
    static void* run0(void* pVoid);  
    static void * thread_proxy_func(void * args);
    void* run1();
    
public:
    static const unsigned char THREAD_STATUS_NEW = 0;
    static const unsigned char THREAD_STATUS_RUNNING = 1;
    static const unsigned char THREAD_STATUS_EXIT = -1;
    
    Thread();
    virtual ~Thread();
    virtual void Run() = 0;
    bool start();
    pthread_t getThreadID();
    int getState();
    void join();
    void join(unsigned long millisTime);
    bool detach();
}; 

struct UsersInfo;
class UserSession;
class WorkThread : public Thread
{
public:
    WorkThread();
    ~WorkThread();

    static void NotifyReceivedProcess(int fd, short which, void *arg);
    static void OnRead(int iCliFd, short iEvent, void *arg);
    static void TimerOutHandle(int iCliFd, short iEvent, void *arg);

    static void OnReadCb(struct bufferevent *bev, void *arg);

    static void OnEventCb(struct bufferevent *bev, short events, void *arg);

    void ResetTimer(UserSession *session);
    
    bool OnWrite(int iCliFd, const u_int32 msg_type, const std::string &data, void *arg);
    
    bool SetupThread();

    bool IniSQLConnection();

    void NotifyThread(const char command);

    void Run();

    void DestrotiedSessions();

    int GetSessionsNum() const;

    bool ClosingClientCon(int fd);

    UserSession* GetUserByClientFD(const int fd);

    void MessageHandle(int fd, const u_int32 msg_type, const std::string &msg);

    bool GetUsersInfoFromDB(const string account, UsersInfo &user_info);

    bool InsertUsersInfoToDB(const UsersInfo &user_info);

    bool UpdateUserScoreToDB(const std::string &account, u_int32 score);
    
    bool UpdateUserPasswordToDB(const std::string &account, const std::string &pswd);
    
    bool UpdateUserNameToDB(const std::string &account, const std::string &name);
    
    bool UpdateHeadImageToDB(const std::string &account, const std::string &data);

    bool GetHeadImageFromDB(const std::string &account, std::string &data);    

    bool GetIndividualUser(const std::string &account);
    
    bool UpdateUserPhoneToDB(const std::string &account, std::string &phoneNo);

    bool UpdateUserEmailToDB(const std::string &account, std::string &email);

    bool GetAdPicturesInfoFromDB(const u_int32 id, AdPicturesInfo &ad_info);
    
    bool GetImageVersionInfoFromDB(ImageVersions &image_info);
    
public:
    struct event_base *base;    /* libevent handle this thread uses */
    struct event notify_event;  /* listen event for notify pipe */
    int notify_receive_fd;      /* receiving end of notify pipe */
    int notify_send_fd;         /* sending end of notify pipe */
    struct event sig_event;
    std::map<int,UserSession*> fdSessionMap;
    MYSQL *pdb_con;
};

#endif /*__WORKER_THREAD_HEAD__*/
