#include "state.h"
#include "usersession.h"
#include <string>
#include <string.h>
#include "utils.h"
#include "debug.h"
#include "thread.h"
#include "mainthread.h"
#include "chessboard.h"

using namespace std;
using namespace MessageStruct;

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
                chessBoardInfo = requestReply.mutable_chessboard();

                if (chessBoard->GetUserNum() == 1) {
                    chessBoard->first_come_user_locate = requestPlay.locate();
                }

                requestReply.set_first_come_user_locate(chessBoard->first_come_user_locate);
                               
                chessBoardInfo->set_id(requestPlay.chess_board_id());
                chessBoardInfo->set_people_num(gameHall->GetChessBoardByID(chessBoardInfo->id())->GetUserNum());
                ChessBoardUser *left = chessBoardInfo->mutable_left_user();
                left->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_LEFT)==NULL) ? true : false);
                
                if (!left->chess_board_empty()) {
                    left->set_user_name(chessBoard->GetUserByLocation(LOCATION_LEFT)->account);
                    left->set_score(chessBoard->GetUserByLocation(LOCATION_LEFT)->score);
                    left->set_status(chessBoard->GetUserByLocation(LOCATION_LEFT)->gameReady);
                    left->set_head_image("Unknown head image");
                }
    
                ChessBoardUser *right = chessBoardInfo->mutable_right_user();
                right->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_RIGHT)==NULL) ? true : false);
                if (!right->chess_board_empty()) {
                    right->set_user_name(chessBoard->GetUserByLocation(LOCATION_RIGHT)->account);
                    right->set_score(chessBoard->GetUserByLocation(LOCATION_RIGHT)->score);
                    right->set_status(chessBoard->GetUserByLocation(LOCATION_RIGHT)->gameReady);
                    right->set_head_image("Unknown head image");
                }
    
                ChessBoardUser *bottom = chessBoardInfo->mutable_bottom_user();
                bottom->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_BOTTOM)==NULL) ? true : false);
                if (!bottom->chess_board_empty()) {
                    bottom->set_user_name(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->account);
                    bottom->set_score(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->score);
                    bottom->set_status(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->gameReady);
                    bottom->set_head_image("Unknown head image");
                }

                /* handle successfully,then return next state*/
                State *state = new StateGamePlay(stateMachine);
                stateMachine->SetNextState(state);
            }

            requestReply.SerializeToString(&data);
            stateMachine->MessageReply(MSG_REQUEST_PLAY_REPLY, data);
        }
    }

    return true;
}

