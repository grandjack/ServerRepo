#ifndef __CHESS_BOARD_HEAD__
#define __CHESS_BOARD_HEAD__

#include <map>
#include <string>
#include "utils.h"
#include "usersession.h"
#include <pthread.h>
#include "state.h"
#include "message.pb.h"

using namespace std;
using namespace MessageStruct;

class ChessBoard;
class UserSession;

class GameHall
{
public:
    GameHall(u_int32 id, u_int32 num = 60);
    ~GameHall();

    u_int32 GetChessValidBoardNum() const;

    ChessBoard* GetChessBoardByID(u_int32 id);

    u_int32 GetExpectedTotalPeopleNum() const;

    u_int32 GetCurrentPeopleNum() const;

    u_int32 GetGameHallID() const;

private:
    bool CreateChessBoard(u_int32 id);

private:
    u_int32 gameHallID;
    u_int32 chessBoardMaxNum;
    map<u_int32, ChessBoard*> chessBoardInfo;
    pthread_mutex_t mutex;
};


class ChessBoard
{
public:
    ChessBoard(u_int32 id, GameHall *gameHall=NULL);
    ~ChessBoard();

    UserSession *GetUserByLocation(const Location &locate);
    inline bool HasFullUsers();
    void BroadCastMsg(const MessageType type, const string &str, Location locate_filter);
    bool AddUser(UserSession *user);
    u_int32 GetUserNum() const;
    bool LeaveOutRoom(const UserSession *user);
    bool LeaveOutRoom(const Location &locate);
    bool TranslateMsg(const u_int32 msg_type, const string &msg);
    bool MoveChessHandle(const string &msg);
    bool ReconciledHandle(const string &msg, MessageType type);
    bool UserMessageHandle(const string &msg);
    bool GiveUpHandle(const string &msg);
    bool UndoHandle(const string &msg, MessageType type);
    bool GameReadyHandle(const string &msg);
    u_int32 GetActiveUsersNum() const;
    u_int32 GetChessBoardID() const;
    void WrapChessBoardInfo(ChessBoardInfo &chessBoard);
    bool GameBegine() const;

private:
    UserSession *leftUser;
    UserSession *rightUser;
    UserSession *bottomUser;
    u_int32 chessBoardID;
    GameHall *currentHall;
    u_int32 currUserNum;
    pthread_mutex_t mutex;

public:
    u_int32 first_come_user_locate;
    u_int32 total_time;
    u_int32 single_step_time;
};

#endif /*__CHESS_BOARD_HEAD__*/

