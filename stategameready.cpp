#include "state.h"
#include "usersession.h"
#include <string>
#include <string.h>
#include "utils.h"
#include "debug.h"
#include "thread.h"
#include "mainthread.h"
#include "chessboard.h"

StateGameReady::StateGameReady(StateMachine *machine):State(machine)
{
    type = STATE_GAME_READY;
}
StateGameReady::~StateGameReady(){}

bool StateGameReady::MsgHandle(const u_int32 msg_type, const string &msg)
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
            
        case MSG_REQUEST_PLAY:
            ret = GameRequestHandle(msg);
            break;

        case MSG_ECHO:
            ret = EchoMsgHandle(msg);
            break;
            
        case MSG_UPDATE_USER_INFO:
            ret = UpdateUserInfos(msg);
            break;

        default:
            break;
    }

    return ret;
}

bool StateGameReady::GameRequestHandle(const string &msg)
{
    GameHallSumary sumary;
    GameHall *gameHall = NULL;
    string data;
    
    RequestPlay requestPlay;
    ChessBoardInfo *chessBoardInfo = NULL;
    ChessBoard *chessBoard = NULL;
    RequestPlayReply requestReply;
    bool added_ok = false;
    
    if (requestPlay.ParseFromString(msg)) {
        gameHall = MainThread::GetMainThreadObj()->GetGameHall(requestPlay.game_hall_id());
        if (NULL != gameHall) {
            chessBoard = gameHall->GetChessBoardByID(requestPlay.chess_board_id());
            stateMachine->locate = (Location)requestPlay.locate();
            if (!chessBoard->AddUser(stateMachine)) {
                LOG_ERROR(MODULE_COMMON, "Add user failed.");
                requestReply.set_status(0);
                //reply status 0
            } else {
                //reply chessBoardInfo & set status 1
                requestReply.set_status(1);
                stateMachine->currChessBoard = chessBoard;

                if (chessBoard->GetUserNum() == 1) {
                    chessBoard->first_come_user_locate = requestPlay.locate();
                }

                requestReply.set_first_come_user_locate(chessBoard->first_come_user_locate);

                
                chessBoardInfo = requestReply.mutable_chessboard();

                chessBoard->WrapChessBoardInfo(*chessBoardInfo);

                /* handle successfully,then return next state*/
                State *state = new StateGamePlay(stateMachine);
                stateMachine->SetNextState(state);
                stateMachine->status = STATUS_READY;
                added_ok = true;
            }

            requestReply.SerializeToString(&data);
            stateMachine->MessageReply(MSG_REQUEST_PLAY_REPLY, data);

            if (added_ok) {
                //Notify the others that should update user's info
                requestReply.set_status(2);
                requestReply.SerializeToString(&data);
                stateMachine->currChessBoard->BroadCastMsg(MSG_REQUEST_PLAY_REPLY, data, (int)stateMachine->locate);
                stateMachine->currChessBoard->BroadCastHallInfo(stateMachine);
            }

        }
    }

    return true;
}

