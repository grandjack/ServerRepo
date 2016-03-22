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

typedef enum 
{
    STATUS_NOT_START = 0,
    STATUS_EXITED = STATUS_NOT_START,
    STATUS_READY,
    STATUS_PLAYING,
    STATUS_ENDED    
}UserGameStatus;

struct UsersInfo
{
    string account;
    string password;
    u_int32 score;
    string email;
    string head_photo;
    string user_name;
    string phone;
};

struct AdPicturesInfo
{
    u_int32 image_id;
    bool  existed;
    string image_hashcode;
    string image_name;
    string image_type;
    string link_url;
    string locate_path;
    u_int32 image_size;    
};

class UserSession
{
public:
    UserSession();
    ~UserSession();

    void SetNextState(State *state);
    bool MessageHandle(const u_int32 msg_type, const string &msg);
    bool MessageReply(const u_int32 msg_type, const string &msg);
    void DestructResource();
    
    void IncreaseScore(u_int32 score=10);
    void ReduceScore(u_int32 score=10);

    bool GetHandleResult() const;
    void SetHandleResult(bool result);

public:
    int clifd;
    struct event event;
    struct event timer_ev;
    struct timeval tv;
    WorkThread *thread;

    Location locate;
    ChessBoard *currChessBoard;

    State *stateMachine;
    State *nextState;

    UsersInfo user_info;
    bool gameReady;
    bool gameOver;

    UserGameStatus status;

private:
    bool send_status;
    
};

#endif/*__USER_SESSION_HEAD__*/
