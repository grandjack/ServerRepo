#ifndef __USER_SESSION_HEAD__
#define __USER_SESSION_HEAD__

#include <event.h>
#include <string>
#include <time.h>
#include "utils.h"

using namespace std;

class WorkThread;
class ChessBoard;
class State;

#ifndef DATA_HEAD_LENGTH
#define DATA_HEAD_LENGTH (2*sizeof(u_int32))
#endif

#ifndef MAX_DATA_LENGTH
#define MAX_DATA_LENGTH (DATA_HEAD_LENGTH + 2048)
#endif


typedef enum
{
    LOCATION_UNKNOWN = 0,
    LOCATION_LEFT = LOCATION_UNKNOWN,
    LOCATION_RIGHT,
    LOCATION_BOTTOM,
    LOCATION_MAX
}Location;

struct UsersInfo
{
    string account;
    string password;
    u_int32 score;
    string email;
    string head_photo;
    string user_name;
};

class UserSession
{
public:
    UserSession();
    ~UserSession();

    void SetNextState(State *state);
    bool MessageHandle(const u_int32 msg_type, const string &msg);
    void MessageReply(const u_int32 msg_type, const string &msg);
    void DestructResource();
    
    void IncreaseScore();
    void ReduceScore();

public:
    int clifd;
    struct event event;
    struct event timer_ev;
    struct timeval tv;
    WorkThread *thread;


    Location locate;
    u_int32 score;
    string account;
    string password;
    string email;
    string user_name;
    ChessBoard *currChessBoard;

    State *stateMachine;
    State *nextState;

    UsersInfo user_info;
    bool gameReady;
    bool gameOver;
    
};

#endif/*__USER_SESSION_HEAD__*/
