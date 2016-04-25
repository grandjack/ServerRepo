#include "usersession.h"
#include "state.h"
#include "utils.h"
#include "debug.h"
#include "thread.h"
#include "chessboard.h"
#include "mainthread.h"

UserSession::UserSession():clifd(-1),thread(NULL),locate(LOCATION_UNKNOWN),
    currChessBoard(NULL),stateMachine(NULL),nextState(NULL),status(STATUS_NOT_START),ad_img_fp(NULL),bufev(NULL)
{
    tv.tv_sec = 60;//every 60 seconds trigger the timer
    tv.tv_usec = 0;

    buf_info.total_size = 0;
    buf_info.msg_type = 0;
    buf_info.got_head = false;

    send_tv.tv_sec = 50;
    send_tv.tv_usec = 0;

    buffer = NULL;
    buf_size = 0;
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
    if (stateMachine == NULL) {
        stateMachine = new StateAuth(this);
        nextState = stateMachine;
    }

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
        try {
            currChessBoard->LeaveRoomHandle(this);
        } catch(exception &e) {
            LOG_ERROR(MODULE_COMMON, "LeaveRoomHandle failed.");
        }
        
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

    if (ad_img_fp != NULL) {
        fclose(ad_img_fp);
        ad_img_fp = NULL;
    }

    if (bufev != NULL) {
        bufferevent_free(bufev);
        bufev = NULL;
    }

    if (clifd != -1) {
        event_del(&timer_ev);
        ::close(clifd);
        clifd = -1;

        LOG_INFO(MODULE_COMMON, "################### User[%s] Destroried ###################\r\n", user_info.account.empty() ? "Unknown" : user_info.account.c_str());

        thread = NULL;
        user_info.Initial();
    }

    //the following handling would let UserSession object crached when descontruct
    //memset(&user_info, 0, sizeof(UsersInfo));

}

u_int8 *UserSession::GetBuffer(const size_t size)
{
    if ((buf_size == 0) && (size < MAX_DATA_LENGTH)) {
        buffer = (u_int8*)calloc(1, MAX_DATA_LENGTH);
        buf_size = MAX_DATA_LENGTH;
    } else if (size > buf_size) {
        buffer = (u_int8*)realloc(buffer, size);
        buf_size = size;
    }

    if (buffer == NULL) {
        LOG_ERROR(MODULE_COMMON, "alloc memery failed!");
    }

    return buffer;
}

size_t UserSession::GetBufferSize(void) const
{
    return buf_size;
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

