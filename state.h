
#ifndef __STATE_MACHINE_HEAD__
#define __STATE_MACHINE_HEAD__

#include <string>
#include "utils.h"
#include "message.pb.h"

using namespace std;
using namespace MessageStruct;

class UserSession;
class ChessBoard;
class GameHall;


typedef UserSession StateMachine;


typedef enum
{
    STATE_AUTH = 0,
    STATE_DOWN_AD,
    STATE_GAME_READY,
    STATE_GAME_PLAY,
    STATE_INVALID
}StateType;

typedef enum
{
    MSG_UNKNOWN = 0,
    MSG_ECHO,
    MSG_LOGIN,
    MSG_LOGIN_REPLY,
    MSG_LOGOUT,
    MSG_LOGOUT_REPLY,
    MSG_REGISTER,
    MSG_REGISTER_REPLY,
    MSG_CHESS_BOARD_USER,
    MSG_CHESS_BOARD_REQ,
    MSG_CHESS_BOARD,
    MSG_HALL_INFO_REQ,
    MSG_HALL_INFO,
    MSG_GAME_HALL_SUMARY_REQ,
    MSG_GAME_HALL_SUMARY,
    MSG_REQUEST_PLAY,
    MSG_REQUEST_PLAY_REPLY,
    MSG_MOVE_CHESS,
    MSG_ANNOUNCE_MOVE,
    MSG_USER_MSG,
    MSG_SYSTEM_MSG,
    MSG_RECONCILED_REQ,
    MSG_RECONCILED_RESP,
    MSG_GIVE_UP,
    MSG_UNDO_REQ,
    MSG_UNDO_REPS,
    MSG_GAME_READY_REQ,
    MSG_GAME_STATUS,
    MSG_FIND_PASSWORD,
    MSG_UPDATE_USER_INFO,
    MSG_AD_IMAGE_INFO,
    MSG_AD_IMAGE_REQ,
    MSG_AD_IMAGE_CONTENT,
    MSG_TYPE_MAX
}MessageType;

class State
{
public:
    State(StateMachine *machine);
    virtual ~State();
    virtual bool MsgHandle(const u_int32 msg_type, const string &msg) = 0;
    StateType GetType() const;

    bool EchoMsgHandle(const string &msg);
    bool GameHallSumaryHandle(const string &msg);
    bool HallInfoReqHandle(const string &msg);
    bool WrapHallInfo(const u_int32 hall_id, string &data);
    bool UpdateUserInfos(const string &msg);
    
protected:
    StateMachine *stateMachine;
    StateType type;
};

class StateAuth : public State
{
public:
    StateAuth(StateMachine *machine);
    ~StateAuth();
    
    bool MsgHandle(const u_int32 msg_type, const string &msg);

private:
    bool HandleRegister(const string &msg);
    bool HandleLogin(const string &msg);
    bool HandleLogout(const string &msg);
    bool HandleFindPwd(const string &msg);

};

class StateGameReady : public State
{
public:
    StateGameReady(StateMachine *machine);
    ~StateGameReady();

    bool MsgHandle(const u_int32 msg_type, const string &msg);
    bool GameRequestHandle(const string &msg);

};

class StateGamePlay : public State
{
public:
    StateGamePlay(StateMachine *machine);
    ~StateGamePlay();

    bool MsgHandle(const u_int32 msg_type, const string &msg);
    bool ChessBoardReq(const string &msg);

};

struct AdPicturesInfo;
class StateAdPictureDownload : public State
{
public:
    StateAdPictureDownload(StateMachine *machine);
    ~StateAdPictureDownload();
    
    bool MsgHandle(const u_int32 msg_type, const string &msg);
    bool AdPictureItemHandle(const string &msg);

private:
    //bool DownloadImage(const string &file_path);
    bool DownloadImageInfo(const AdPicturesInfo &ad_info);
    bool SendImageContent();
};
#endif /*__STATE_MACHINE_HEAD__*/

