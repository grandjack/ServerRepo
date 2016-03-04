#include "state.h"
#include "usersession.h"
#include <string>
#include <string.h>
#include "utils.h"
#include "debug.h"
#include "thread.h"

StateAuth::StateAuth(StateMachine *machine):State(machine)
{
    type = STATE_AUTH;
}
StateAuth::~StateAuth(){}

bool StateAuth::MsgHandle(const u_int32 msg_type, const string &msg)
{
    bool ret = false;

    switch(msg_type)
    {
        case MSG_REGISTER:
            ret = HandleRegister(msg);
            break;
        
        case MSG_LOGIN:
            ret = HandleLogin(msg);
            /*if compare the user account and password successfully,then go to next state*/
            if (ret == true) {
                stateMachine->SetNextState(new StateGameReady(stateMachine));
            }
            break;
        
        case MSG_LOGOUT:
            ret = HandleLogout(msg);
            if (ret == true) {
                ret = false;//just for close the connection
            }
            break;
            
        case MSG_ECHO:
            ret = EchoMsgHandle(msg);
            break;

        case MSG_FIND_PASSWORD:
            ret = HandleFindPwd(msg);
            break;
            
        default:
            break;
    }

    return ret;
}

bool StateAuth::HandleRegister(const string &msg)
{
    bool ret = true;
    Register register_info;
    UsersInfo  user_info;
    
    if (register_info.ParseFromString(msg)) {
        LOG_DEBUG(MODULE_COMMON, "email_account %s", register_info.email_account().c_str());
        LOG_DEBUG(MODULE_COMMON, "password %s", register_info.password().c_str());
        LOG_DEBUG(MODULE_COMMON, "username %s", register_info.username().c_str());

        user_info.account = register_info.email_account();
        user_info.password = register_info.password();
        user_info.score = 100;
        user_info.email = user_info.account;
        
        ReplyStatus status;
        string data;
        
        if (stateMachine->thread->InsertUsersInfoToDB(user_info)) {
            status.set_status(1);
        } else {
            LOG_ERROR(MODULE_COMMON, "User Account[%s] register failed.", register_info.email_account().c_str());
            status.set_status(0);
        }

        status.SerializeToString(&data);
        stateMachine->MessageReply(MSG_REGISTER_REPLY, data);
    }
    
    return ret;
}

bool StateAuth::HandleLogin(const string &msg)
{
    LogOnorOut login;
    ReplyStatus status;        
    string data;
    UsersInfo  *user_info = &stateMachine->user_info;
    bool ret = false;
        
    if (login.ParseFromString(msg)) {
        LOG_DEBUG(MODULE_COMMON, "account %s", login.account().c_str());
        LOG_DEBUG(MODULE_COMMON, "password %s", login.password().c_str());

        if (!(stateMachine->thread->GetIndividualUser(login.account()))) {
            LOG_ERROR(MODULE_COMMON, "This user has already online.");
            status.set_status(0);
            ret = false;
        } else if (stateMachine->thread->GetUsersInfoFromDB(login.account(), *user_info)) {
            if (user_info->password.compare(login.password()) == 0) {

                stateMachine->thread->GetHeadImageFromDB(user_info->account, user_info->head_photo);
                
                status.set_status(1);
                ChessBoardUser *user = status.mutable_user();
                user->set_chess_board_empty(false);
                user->set_account(user_info->account);
                user->set_user_name(user_info->user_name);
                user->set_score(user_info->score);
                user->set_ex_email(user_info->email);
                user->set_phone(user_info->phone);
                user->set_head_image(user_info->head_photo);
                
                ret = true;
            } else {
                status.set_status(0);
                LOG_ERROR(MODULE_COMMON, "User Account[%s] login failed for password NOT equal.", user_info->account.c_str());
                ret = false;
            }
        } else {
            LOG_ERROR(MODULE_COMMON, "User Account[%s] login failed for select SQL DB failed.", user_info->account.c_str());
            status.set_status(0);
            ret = false;
        }

        status.SerializeToString(&data);
        stateMachine->MessageReply(MSG_LOGIN_REPLY, data);
    }
    
    return ret;
}

bool StateAuth::HandleLogout(const string &msg)
{
    LogOnorOut logout;
    
    if (logout.ParseFromString(msg)) {
        LOG_DEBUG(MODULE_COMMON, "account %s", logout.account().c_str());
        LOG_DEBUG(MODULE_COMMON, "password %s", logout.password().c_str());
    }
    
    return true;
}


bool StateAuth::HandleFindPwd(const string &msg)
{
    FindPassword pwd;

    if (pwd.ParseFromString(msg)) {
        LOG_DEBUG(MODULE_COMMON, "To find the lost password for [%s]!!", pwd.email().c_str());
    }

    return true;
}

