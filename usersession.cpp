#include "usersession.h"
#include "state.h"
#include "utils.h"
#include "debug.h"
#include "thread.h"
#include "chessboard.h"

UserSession::UserSession():clifd(0),thread(NULL),locate(LOCATION_UNKNOWN),
    currChessBoard(NULL),stateMachine(NULL),nextState(NULL),status(STATUS_NOT_START),ad_img_fp(NULL),bufev(NULL),send_status(true)
{
    tv.tv_sec = 60;//every 10 seconds trigger the timer
    tv.tv_usec = 0;
    stateMachine = new StateAuth(this);
    nextState = stateMachine;
}

UserSession::~UserSession()
{
}

void UserSession::SetNextState(State * state)
{
    this->nextState = state;
}

bool UserSession::MessageHandle(const u_int32 msg_type, const string &msg)
{
    if (stateMachine->MsgHandle(msg_type, msg) != true) {
        LOG_ERROR(MODULE_COMMON, "MsgHandle failed. current state[%d]", stateMachine->GetType());
        return false;
    } else {
        if ((nextState != NULL) && (nextState->GetType() != stateMachine->GetType())) {
            State *tmp = stateMachine;
            stateMachine = nextState;
            delete tmp;
        }
    }

    return true;
}

bool UserSession::MessageReply(const u_int32 msg_type, const string &msg)
{
     return thread->OnWrite(clifd, msg_type, msg, this);
}

void UserSession::DestructResource()
{
    if (currChessBoard != NULL) {
        currChessBoard->LeaveRoomHandle(this);
        currChessBoard = NULL;
    }

    if (stateMachine != NULL) {
        if (stateMachine == nextState) {
            delete stateMachine;
            stateMachine = NULL;
            nextState = NULL;
        } else {
        
            delete stateMachine;
            stateMachine = NULL;
        
            if (nextState != NULL) {
                delete nextState;
                nextState = NULL;
            }
        }
    }
    
    thread = NULL;

}

void UserSession::IncreaseScore(u_int32 score)
{
    user_info.score += score;
    thread->UpdateUserScoreToDB(this->user_info.account, this->user_info.score);
}
void UserSession::ReduceScore(u_int32 score)
{
    user_info.score -= score;
    thread->UpdateUserScoreToDB(this->user_info.account, this->user_info.score);
}

bool UserSession::GetHandleResult() const
{
    return send_status;
}

void UserSession::SetHandleResult(bool result)
{
    send_status = result;
}

