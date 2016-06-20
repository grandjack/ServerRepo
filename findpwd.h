
#ifndef __FIND_PASSWORD_HEAD__
#define __FIND_PASSWORD_HEAD__

#include <iostream>
#include <string>
#include <list>
#include "thread.h"
#include "timer.h"

using namespace std;

typedef struct {
    string account;
    string token;
    unsigned char retry_count;
}FpwUserInfo;

class FindPwdViaEmail {
public:
    static bool FindPwdViaAccount(const string &account);
    static void SendMail(void * arg);
    static void TimeOutHandle(void * arg);
    static bool WriteUserTokentoFile(const string &account, const string &token);
    static void Destrory();

private:
    FindPwdViaEmail();
    ~FindPwdViaEmail();

    bool AddToTimerList(const string &account);

private:
    static FindPwdViaEmail *instance_;
};

#endif /*__FIND_PASSWORD_HEAD__*/
