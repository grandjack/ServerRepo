#include <unistd.h>
#include "thread.h"
#include "usersession.h"
#include "mainthread.h"
#include "debug.h"
#include <iostream>
#include <errno.h>
#include <string.h>
#include "mysqldb.h"
#include "state.h"

using namespace std;

Thread::Thread(): tid(0),threadStatus(THREAD_STATUS_NEW),beInitialed(false)
{
    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);
}
Thread::~Thread()
{
    pthread_mutex_destroy(&init_lock);
    pthread_cond_destroy(&init_cond);
}

  
bool Thread::start()
{
    int ret = -1;
    ret = pthread_create(&tid, NULL, thread_proxy_func, this);


    pthread_mutex_lock(&init_lock);
    while(beInitialed == false) {
        pthread_cond_wait(&init_cond, &init_lock);
    }    
    pthread_mutex_unlock(&init_lock);

    return (ret == 0);
}  
  
pthread_t Thread::getThreadID()  
{  
    return tid;  
}  
  
int Thread::getState()  
{  
    return threadStatus;  
}  
  
void Thread::join()
{  
    if (tid > 0)  
    {  
        pthread_join(tid, NULL);
    }  
}  
void * Thread::thread_proxy_func(void * args)
{
    Thread * pThread = static_cast<Thread *>(args);
    pThread->threadStatus = THREAD_STATUS_RUNNING;
    pThread->tid = pthread_self();
    
    pthread_mutex_lock(&pThread->init_lock);
    pThread->beInitialed = true;
    pthread_cond_signal(&pThread->init_cond);
    pthread_mutex_unlock(&pThread->init_lock);

    pThread->Run();

    pThread->threadStatus = THREAD_STATUS_EXIT;
    pThread->tid = 0;
    
    pthread_exit(NULL);  

    return NULL;
}  
  
void Thread::join(unsigned long millisTime)  
{  
    if (tid == 0)  
    {  
        return;  
    }  
    if (millisTime == 0)  
    {  
        join();  
    }
    else
    {  
        unsigned long k = 0;  
        while ((threadStatus != THREAD_STATUS_EXIT) && (k <= millisTime))
        {  
            usleep(10);
            k++;
        }  
    }  
}
bool Thread::detach()
{
    return (pthread_detach(tid)==0);
}

WorkThread::~WorkThread(){}

WorkThread::WorkThread():base(NULL),pdb_con(NULL){}

bool WorkThread::SetupThread()
{
    bool ret = false;
    int fds[2];
    
    base = event_init();
    
    if (!base) {
        return ret;
    }
    
    if (pipe(fds)) {
        return ret;
    }

    notify_receive_fd = fds[0];
    notify_send_fd = fds[1];        

    /* Listen for notifications from other threads */
    
    AddEventForBase(base, &notify_event, notify_receive_fd, EV_READ | EV_PERSIST, NotifyReceivedProcess, (void*)this);

    return true;
}

bool WorkThread::IniSQLConnection()
{
    int ret = 0;

    mysql_db_init(&pdb_con);
    if (pdb_con == NULL) {
        LOG_ERROR(MODULE_DB, "mysql_db_init failed.");
        return false;
    }
        
    ret = mysql_db_connect(pdb_con, "123.57.180.67", "gbx386", "760226", "ThreePeoplePlayChessDB");
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_connect failed.");
        return false;
    }

    creat_users_info_table(pdb_con);

    creat_ad_pictures_table(pdb_con);

    return true;
}

void WorkThread::TimerOutHandle(int iCliFd, short iEvent, void *arg)
{
    UserSession * session = static_cast<UserSession *>(arg);
    WorkThread *thread = NULL;
    
    if (session == NULL) {
        return;
    }

    thread = session->thread;

    //Send the message to the client user

    LOG_DEBUG(MODULE_COMMON, "User[%s] timer out, we will get him rid off.", session->user_info.account.c_str());

    thread->ClosingClientCon(session->clifd);
}

void WorkThread::ResetTimer(UserSession *session)
{
    if (session == NULL) {
        return;
    }

    try {
        event_del(&session->timer_ev);

        LOG_DEBUG(MODULE_COMMON, "ResetTimer for user[%s]...", session->user_info.account.c_str());
        
        evtimer_set(&session->timer_ev, TimerOutHandle, (void*)session);
        event_base_set(session->thread->base, &session->timer_ev);
        evtimer_add(&session->timer_ev, &session->tv);
    } catch(exception &e) {
        LOG_ERROR(MODULE_COMMON, "Update timer failed.");
    }
}

void WorkThread::NotifyReceivedProcess(int fd, short which, void *arg)
{
    WorkThread * pThread = static_cast<WorkThread *>(arg);
    char buf[1] ={0};
    UserSession *session = NULL;
    MainThread *mainThread = MainThread::GetMainThreadObj();
 
    if (::read(fd, buf, 1) != 1)
        return;
    
    switch(buf[0])
    {
        case COMMAND_NOTIFY_ADD_EVENT:
        session = mainThread->PopFrontSession();
        if (session != NULL)
        {
            session->thread = pThread;
            AddEventForBase(pThread->base, &session->event, session->clifd, EV_READ | EV_PERSIST, OnRead, arg);
            pThread->fdSessionMap.insert(pair<int, UserSession*>(session->clifd, session));

            //initialize the timer
            evtimer_set(&session->timer_ev, TimerOutHandle, (void*)session);
            event_base_set(pThread->base, &session->timer_ev);
            evtimer_add(&session->timer_ev, &session->tv);
    
        }
        break;

        case COMMAND_NOTIFY_QUIT_LOOP:
            event_base_loopexit(pThread->base, NULL);
            break;
            
        default:
            break;
    }

}

void WorkThread::NotifyThread(const char command)
{
    char buf[1] = {0};
    buf[0] = command;
    
    if (write(this->notify_send_fd, buf, 1) != 1) {
        LOG_ERROR(MODULE_COMMON, "Writing to thread notify pipe failed");
    }
}

void WorkThread::DestrotiedSessions()
{
    map <int, UserSession*>::iterator iter;
    UserSession *session = NULL;
    for ( iter = fdSessionMap.begin(); iter != fdSessionMap.end(); iter++ ) {
        //free the user sessions
        session = iter->second;
        if (session != NULL) {
            event_del(&session->event);
            close(session->clifd);
            delete session;
        }
    }
    fdSessionMap.clear();
}

bool WorkThread::OnWrite(int iCliFd, const u_int32 msg_type, const string &data, void *arg)
{
    UserSession *user = static_cast<UserSession *>(arg);
    u_int8 buffer[MAX_DATA_LENGTH] = { 0 };
    u_int8 *buf = &buffer[0];
    u_int8 *pBuf = NULL;
    u_int32 totalSize = data.size() + DATA_HEAD_LENGTH;
    u_int32 sendLen = 0;
    int iLen = 0;
    bool ret = true;

    if(!user->GetHandleResult()) {
        LOG_ERROR(MODULE_NET, "Can NOT send, this session will ouver!");
        return false;
    }

    if (user == NULL) {
        LOG_ERROR(MODULE_NET, "Got parameter failed.");
        return false;
    }

    if (totalSize > MAX_DATA_LENGTH) {
        try {
            pBuf = new u_int8[totalSize];
        } catch (exception &e) {
            LOG_ERROR(MODULE_NET, "New pBuf failed.");
            return false;
        }
        buf = pBuf;
    }
    
    memcpy(buf, &totalSize, DATA_HEAD_LENGTH/2);
    memcpy(buf + DATA_HEAD_LENGTH/2 , &msg_type, DATA_HEAD_LENGTH/2);
    memcpy(buf + DATA_HEAD_LENGTH, data.c_str(), data.size());

    LOG_DEBUG(MODULE_NET, "Send totalSize[%u] msg_type %u", totalSize, msg_type);

    while(sendLen < totalSize)
    {
        iLen = send(iCliFd, &buf[sendLen], totalSize-sendLen, 0);
        if (iLen <= 0) {
            if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
                LOG_INFO(MODULE_NET, "errno EINTR, will continue");
                continue;
            } else {
                LOG_ERROR(MODULE_NET, "send() return err: %d,[%s]\n", errno, strerror(errno));
                //ClosingClientCon(iCliFd);
                ret = false;
                user->SetHandleResult(ret);
                break;
            }
        } else {
            sendLen += iLen;
        }
    }

    if (pBuf != NULL) {
        delete []pBuf;
        pBuf = NULL;
    }

    return ret;
}

void WorkThread::OnRead(int iCliFd, short iEvent, void *arg)
{
    WorkThread * pThread = static_cast<WorkThread *>(arg);
    int iLen = 0; 
    u_int8 buffer[MAX_DATA_LENGTH] = { 0 };
    u_int8 *buf = &buffer[0];
    u_int8 *pBuf = NULL;
    u_int32 recvLen = 0;
    u_int32 leftLen = DATA_HEAD_LENGTH;
    u_int32 totalSize = leftLen;
    bool gotHead = false;
    u_int32 msg_type = 0;

    while(recvLen < totalSize) {
        iLen = recv(iCliFd, &buf[recvLen], leftLen, 0);
        if (iLen > 0) {
            recvLen += iLen;
            
            if (!gotHead) {
                if (recvLen >= DATA_HEAD_LENGTH) {
                    totalSize = *((u_int32 *)(buf));
                    msg_type = *(((u_int32 *)(buf))+1);
                    
                    LOG_DEBUG(MODULE_COMMON, "Got message total size : %u msg_type %u", totalSize, msg_type);
                    if ( msg_type > MSG_TYPE_MAX || totalSize > 500*MAX_DATA_LENGTH) {
                        LOG_ERROR(MODULE_NET, "Received invalid message, ignore it!");
                        return;
                    }
                    
                    gotHead = true;

                    if (totalSize > MAX_DATA_LENGTH) {
                        try {
                            pBuf = new u_int8[totalSize];
                        } catch (exception &e) {
                            LOG_ERROR(MODULE_NET, "New pBuf failed.");
                            return;
                        }
                        memcpy(pBuf, buf, recvLen);
                        buf = pBuf;
                    }
                }
            }

            LOG_DEBUG(MODULE_NET, "Current recv Len %u", iLen);
            
            leftLen = totalSize - recvLen;
            
        } else if (iLen <= 0) {
            if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
                LOG_INFO(MODULE_NET, "errno EINTR, will continue");
                continue;
            } else {
                LOG_ERROR(MODULE_NET, "recv() return err: %d,[%s]\n", errno, strerror(errno));
                pThread->ClosingClientCon(iCliFd);
                break;
            }
        }
    }

    const string data((char *)&buf[DATA_HEAD_LENGTH], totalSize - DATA_HEAD_LENGTH);
    
    if (pBuf != NULL) {
        delete []pBuf;
        pBuf = NULL;
    }

    pThread->MessageHandle(iCliFd, msg_type, data);
} 

int WorkThread::GetSessionsNum() const
{
    return fdSessionMap.size();
}

void WorkThread::MessageHandle(int fd, const u_int32 msg_type, const string &msg)
{
    map<int ,UserSession* >::iterator l_it;
    UserSession *session = NULL;
    
    try {
        l_it = fdSessionMap.find(fd);
    } catch (const exception &e) {
        LOG_ERROR(MODULE_COMMON, "Get session failed.");
        return ;
    }
      
    if(l_it != fdSessionMap.end()) {
        session = static_cast<UserSession *>(l_it->second);
        if (session != NULL) {
            if ((!session->MessageHandle(msg_type, msg)) || (!session->GetHandleResult())) {
                LOG_ERROR(MODULE_COMMON, "MessageHandle failed,close the connection for [%s].",session->user_info.account.c_str());
                ClosingClientCon(fd);
            } else {
                ResetTimer(session);
            }
        }
    }

}

bool WorkThread::ClosingClientCon(int fd)
{
    map<int ,UserSession* >::iterator l_it;
    UserSession *session = NULL;
    
    try {
        l_it = fdSessionMap.find(fd);
    }
    catch (const exception &e) {
        LOG_ERROR(MODULE_COMMON, "Get session failed.");
        close(fd);
        return false;
    }

    if(l_it != fdSessionMap.end()) {
        session = static_cast<UserSession *>(l_it->second);
        if (session != NULL) {
            event_del(&session->event);
            event_del(&session->timer_ev);
            close(fd);
            session->DestructResource();
            delete session;
        }
        fdSessionMap.erase(l_it);
    }

    return true;
}

UserSession* WorkThread::GetUserByClientFD(const int fd)
{
    map<int ,UserSession* >::iterator l_it;
    UserSession *session = NULL;
    
    try {
        l_it = fdSessionMap.find(fd);
    }
    catch (const exception &e) {
        LOG_ERROR(MODULE_COMMON, "Get session failed.");
        close(fd);
        return session;
    }

    if(l_it != fdSessionMap.end()) {
        session = static_cast<UserSession *>(l_it->second);
    }

    return session;
}

bool WorkThread::GetIndividualUser(const string &account)
{
    map<int ,UserSession* >::iterator l_it;
    UserSession *session = NULL;
    bool found = false;

    for (l_it = fdSessionMap.begin(); l_it != fdSessionMap.end(); l_it++) {
        session = static_cast<UserSession *>(l_it->second);
        if (session->user_info.account == account) {
            found = true;
            break;
        }
    }

    return (!found);
}

bool WorkThread::GetUsersInfoFromDB(const string account, UsersInfo &user_info)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    if (account.empty()) {
        LOG_ERROR(MODULE_DB, "Account is empty.");
        return false;
    }
    
    snprintf(select_cmd, sizeof(select_cmd)-1, "SELECT * FROM users_info WHERE account='%s'",account.c_str());
    
    ret = mysql_db_query(pdb_con, select_cmd, user_info);
    if (ret <= 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_query failed for user[%s].", user_info.account.c_str());
        return false;
    }

    return true;
}

bool WorkThread::InsertUsersInfoToDB(const UsersInfo &user_info)
{
    char select_cmd[512] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "INSERT INTO users_info (account,password,email,score,head_photo,user_name,phone) VALUES ('%s','%s','%s',%d,'%s','%s','%s')",\
            user_info.account.c_str(), user_info.password.c_str(), user_info.email.c_str(),user_info.score, user_info.head_photo.c_str(), user_info.user_name.c_str(), user_info.phone.c_str());

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);
    ret = mysql_db_excute(pdb_con, select_cmd, strlen(select_cmd));
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        return false;
    }

    return true;
}

bool WorkThread::UpdateUserScoreToDB(const std::string &account, u_int32 score)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "UPDATE users_info SET score=%d where account='%s'",
             score, account.c_str());

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);

    ret = mysql_db_excute(pdb_con, select_cmd, strlen(select_cmd));
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        return false;
    }
    
    return true;
}

bool WorkThread::UpdateUserPasswordToDB(const std::string &account, const std::string &pswd)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "UPDATE users_info SET password='%s' WHERE account='%s'",
             pswd.c_str(), account.c_str());

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);

    ret = mysql_db_excute(pdb_con, select_cmd, strlen(select_cmd));
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        return false;
    }
    
    return true;
}

bool WorkThread::UpdateUserNameToDB(const std::string &account, const std::string &name)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "UPDATE users_info SET user_name='%s' WHERE account='%s'",
             name.c_str(), account.c_str());

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);

    ret = mysql_db_excute(pdb_con, select_cmd, strlen(select_cmd));
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        return false;
    }
    
    return true;
}

bool WorkThread::UpdateHeadImageToDB(const std::string &account, const std::string &data)
{
    char *select_cmd = NULL;
    int ret = -1;
    char *end = NULL;
    bool retV = true;

    select_cmd = new char[data.size() + data.size()*2/3 + 255];
    if (select_cmd != NULL) {
        end = strcpy(select_cmd, "UPDATE users_info SET head_photo=");
        end += strlen(select_cmd);
        *end++ = '\'';
        end += mysql_real_escape_string(pdb_con, end, data.c_str(), data.size());
        *end++ = '\'';
        end += sprintf(end, " WHERE account='%s'", account.c_str());
        
        LOG_DEBUG(MODULE_DB, "select_cmd length[%d]  data legth[%d]", (end - select_cmd), data.size());

        ret = mysql_db_excute(pdb_con, select_cmd, (end - select_cmd));
        if (ret != 0) {
            LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
            retV = false;
        }

        delete []select_cmd;
        select_cmd = NULL;
        end = NULL;
    }
    
    return retV;
}

bool WorkThread::UpdateUserPhoneToDB(const std::string &account, std::string &phoneNo)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "UPDATE users_info SET phone='%s' WHERE account='%s'",
             phoneNo.c_str(), account.c_str());

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);

    ret = mysql_db_excute(pdb_con, select_cmd, strlen(select_cmd));
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        return false;
    }
    
    return true;

}
bool WorkThread::UpdateUserEmailToDB(const std::string &account, std::string &email)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "UPDATE users_info SET email='%s' WHERE account='%s'",
             email.c_str(), account.c_str());

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);

    ret = mysql_db_excute(pdb_con, select_cmd, strlen(select_cmd));
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        return false;
    }
    
    return true;
}

bool WorkThread::GetHeadImageFromDB(const std::string &account, std::string &data)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "SELECT head_photo FROM users_info WHERE account='%s'", account.c_str());

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);

    ret = mysql_get_binary_data(pdb_con, select_cmd, strlen(select_cmd), data);
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_get_binary_data failed, ret[%d]", ret);
        return false;
    }
    
    return true;
}

bool WorkThread::GetAdPicturesInfoFromDB(const u_int32 id, AdPicturesInfo &ad_info)
{
    char select_cmd[255] = { 0 };
    int ret = -1;

    snprintf(select_cmd, sizeof(select_cmd)-1, "SELECT * FROM ad_pictures WHERE id=%d", id);

    LOG_DEBUG(MODULE_DB, "select_cmd[%s]", select_cmd);

    ret = mysql_db_query_ad_info(pdb_con, select_cmd, ad_info);
    if (ret <= 0) {
        ad_info.existed = false;
        LOG_ERROR(MODULE_DB, "mysql_db_query_ad_info failed, ret[%d]", ret);
        return false;
    }
    
    return true;
}

void WorkThread::Run()
{
    LOG_INFO(MODULE_COMMON, "Start run thread[%lu]", getThreadID());
    
    SetupThread();

    if (!IniSQLConnection()) {
        LOG_ERROR(MODULE_DB, "IniSQLConnection failed, thread exit!");
        return;
    }

    event_base_dispatch(base);
    
    event_base_free(base);

    this->DestrotiedSessions();

    mysql_db_close(pdb_con);
    
    LOG_INFO(MODULE_COMMON, "End run thread[%lu]", getThreadID());
}

