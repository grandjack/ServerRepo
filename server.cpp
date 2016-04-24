#include "mainthread.h"
#include "thread.h"
#include "usersession.h"
#include "debug.h"
#include <signal.h>
#include "utils.h"
#include "md5.h"

//#define LOG_FILE    "/var/log/chessServer.log"
#define LOG_FILE    "/tmp/chessServer.log"

int main() 
{   
    struct event signal_int, signal_term;
    MainThread *mainThread = NULL;
    
    log_init(LOG_FILE, 102400, DEBUG);

    sig_ignore(SIGPIPE);
    
    LOG_DEBUG(MODULE_COMMON, "Begain create main thread...");
    
    mainThread = MainThread::GetMainThreadObj();
    mainThread->CreateWorkTheads(1);
    mainThread->IniServerEnv(8888);

    AddEventForBase(mainThread->mainEventBase,&signal_int,SIGINT, EV_SIGNAL|EV_PERSIST, 
        signal_handle, (void*)mainThread->mainEventBase);

    AddEventForBase(mainThread->mainEventBase,&signal_term,SIGTERM, EV_SIGNAL|EV_PERSIST, 
        signal_handle, (void*)mainThread->mainEventBase);

    mainThread->Run();
    
    MainThread::DestructMainThreadObj();
    
    log_close();
   
    return 0; 
}

