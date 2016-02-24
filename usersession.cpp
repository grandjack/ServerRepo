#include "usersession.h"
#include "state.h"
#include "utils.h"
#include "debug.h"
#include "thread.h"
#include "chessboard.h"

UserSession::UserSession():clifd(0),thread(NULL),locate(LOCATION_UNKNOWN),score(0),
    currChessBoard(NULL),stateMachine(NULL),nextState(NULL),gameReady(false),gameOver(false)
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

void UserSession::MessageReply(const u_int32 msg_type, const string &msg)
{
     thread->OnWrite(clifd, msg_type, msg, this);
}

void UserSession::DestructResource()
{
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

    if (currChessBoard != NULL) {
        currChessBoard->LeaveOutRoom(this);
        currChessBoard = NULL;
    }
    
    thread = NULL;

}

void UserSession::IncreaseScore()
{
    this->score += 30;
    user_info.score = this->score;
    thread->UpdateUserScoreToDB(this->account, this->score);
}
void UserSession::ReduceScore()
{
    this->score -= 30;
    user_info.score = this->score;
    thread->UpdateUserScoreToDB(this->account, this->score);
}

