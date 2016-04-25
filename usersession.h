#ifndef __USER_SESSION_HEAD__
#define __USER_SESSION_HEAD__

#include <event.h>
#include <string>
#include <time.h>
#include "utils.h"
#include <stdio.h>

using namespace std;

class WorkThread;
class ChessBoard;
class State;

#ifndef DATA_HEAD_LENGTH
#define DATA_HEAD_LENGTH (2*sizeof(u_int32))
#endif

#ifndef MAX_DATA_LENGTH
#define MAX_DATA_LENGTH (DATA_HEAD_LENGTH + (0x01<<13))
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
    STATUS_BEGAN,
    STATUS_PLAYING,
    STATUS_ENDED
}UserGameStatus;

struct UsersInfo
{
public:
    string account;
    string password;
    u_int32 score;
    string email;
    string head_photo;
    string user_name;
    string phone;

    UsersInfo(){Reset();}

    void Reset()
    {
        account=password=email=head_photo=user_name=phone="";
        score=0;
    }
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

struct ReadBufInfo
{
    u_int32 total_size;
    u_int32 msg_type;
    bool got_head;

    ReadBufInfo(){Reset();}

    void Reset()
    {
        total_size=msg_type=0;
        got_head=false;
    }
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

    u_int8 *GetBuffer(const size_t size=MAX_DATA_LENGTH);
    size_t GetBufferSize(void) const;

public:
    int clifd;
    struct event timer_ev;
    struct timeval tv;
    WorkThread *thread;

    Location locate;
    ChessBoard *currChessBoard;

    State *stateMachine;
    State *nextState;

    UsersInfo user_info;

    UserGameStatus status;

    FILE *ad_img_fp;

    bufferevent *bufev;
    ReadBufInfo buf_info;
    struct timeval send_tv;

    u_int8 *buffer;
    size_t buf_size;
    
};
#endif/*__USER_SESSION_HEAD__*/
