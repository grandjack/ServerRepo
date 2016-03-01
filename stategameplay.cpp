#include "state.h"
#include "usersession.h"
#include <string>
#include <string.h>
#include "utils.h"
#include "debug.h"
#include "thread.h"
#include "chessboard.h"

using namespace std;
using namespace MessageStruct;


StateGamePlay::StateGamePlay(StateMachine *machine):State(machine)
{
    type = STATE_GAME_PLAY;
}
StateGamePlay::~StateGamePlay(){}

bool StateGamePlay::MsgHandle(const u_int32 msg_type, const string &msg)
{
    bool ret = false;

    /*handling for auth message*/
    switch(msg_type)
    {
        case MSG_GAME_HALL_SUMARY_REQ:
            ret = GameHallSumaryHandle(msg);
            break;

        case MSG_HALL_INFO_REQ:
            ret = HallInfoReqHandle(msg);
            break;
        
        case MSG_ECHO:
            ret = EchoMsgHandle(msg);
            break;            

        case MSG_CHESS_BOARD_REQ:
            ret = ChessBoardReq(msg);
            break;

        case MSG_UPDATE_USER_INFO:
            ret = UpdateUserInfos(msg);
            break;
            
        case MSG_MOVE_CHESS:
        case MSG_RECONCILED_REQ:
        case MSG_USER_MSG:
        case MSG_UNDO_REQ:
        case MSG_GIVE_UP:
        case MSG_GAME_READY_REQ:
        case MSG_SYSTEM_MSG:
            ret = stateMachine->currChessBoard->TranslateMsg(msg_type, msg);
            break;

        default:
            break;
    }


    /*if handle failed,then  return next state*/
    if (ret == false) {
        stateMachine->SetNextState(new StateGameReady(stateMachine));
    }

    return ret;
}

bool StateGamePlay::ChessBoardReq(const string &msg)
{
    ChessBoardInfoReq chessboard;
    
    if (chessboard.ParseFromString(msg)) {
        WrapChessBoardInfo();
    }

    return true;
}

