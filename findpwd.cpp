#include "findpwd.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "md5.h"
#include "debug.h"

#define VERIFY_PATH         "/usr/share/web/data/"
#define SEND_MAIL_CMD       "python ./MailSend.py "
#define OUT_TIME            (5*60)

FindPwdViaEmail* FindPwdViaEmail::instance_ = NULL;

FindPwdViaEmail::FindPwdViaEmail()
{
    pthread_mutex_init(&mutex_, NULL);
}
FindPwdViaEmail::~FindPwdViaEmail()
{
    pthread_mutex_destroy(&mutex_);
}

bool FindPwdViaEmail::FindPwdViaAccount(const string &account)
{
    if (instance_ == NULL) {
        instance_ = new FindPwdViaEmail();
        if (instance_ != NULL) {
            pthread_mutex_lock(&instance_->mutex_);
            timer_init();
            pthread_mutex_unlock(&instance_->mutex_);
        }
    }

    return instance_->AddToTimerList(account);
}


bool FindPwdViaEmail::AddToTimerList(const string &account)
{

    pthread_mutex_lock(&instance_->mutex_);

    FpwUserInfo *user = new FpwUserInfo();
    char buffer[33] = { 0 };
    sprintf(buffer, "%u", (unsigned int)time(NULL));
    
    if (user != NULL) {
        user->account = account;
        user->token = md5(account + buffer);
        user->retry_count = 0;
        if (0 > timer_add(0,10, &FindPwdViaEmail::SendMail, user)) {
            LOG_ERROR(MODULE_COMMON, "timer_add failed");
            delete user;
        }
    }
    
    pthread_mutex_unlock(&instance_->mutex_);
    
    return true;
}


void FindPwdViaEmail::SendMail(void * arg)
{
    FpwUserInfo *user = static_cast<FpwUserInfo*>(arg);
    LOG_DEBUG(MODULE_COMMON, "Send mail to user[%s]  token[%s]", user->account.c_str(), user->token.c_str());

    if (!FindPwdViaEmail::WriteUserTokentoFile(user->account, user->token)) {
        user->retry_count++;
        if (user->retry_count <= 5) {
            timer_add(60,0, &FindPwdViaEmail::SendMail, user);
        } else {
            timer_add(0,500, &FindPwdViaEmail::TimeOutHandle, user);
        }
    } else {
        timer_add(OUT_TIME,0, &FindPwdViaEmail::TimeOutHandle, user);
    }
}

void FindPwdViaEmail::TimeOutHandle(void * arg)
{
    FpwUserInfo *user = static_cast<FpwUserInfo*>(arg);

    LOG_DEBUG(MODULE_COMMON, "Time out of user[%s]  token[%s]", user->account.c_str(), user->token.c_str());
    //TODO check whether the flag has been set by the user

    string file = string(VERIFY_PATH + user->account);
    unlink(file.c_str());

    delete user;
    user = NULL;
}

bool FindPwdViaEmail::WriteUserTokentoFile(const string &account, const string &token)
{
    FILE *fptr = NULL;
    char buf[34] = {0};
    string path = VERIFY_PATH + account;
    string cmd = SEND_MAIL_CMD + account + " " + token;
    int ret = -1;
    bool ret_var = true;

    strncpy(buf, token.c_str(), sizeof(buf) - 1);
    
    fptr = fopen(path.c_str(), "w+");
    if (fptr != NULL) {
        if (fwrite(buf, 1, token.size(), fptr) < 0 ) {
            LOG_ERROR(MODULE_COMMON, "fwrite token failed for user[%s]", account.c_str());
        }
        
        fclose(fptr);
    }

    ret = system(cmd.c_str());
    if (ret != 0) {
        ret_var = false;
        LOG_ERROR(MODULE_COMMON, "system comand failed [%s]", cmd.c_str());
    }

    return ret_var;
}

