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


State::State(StateMachine *machine):stateMachine(machine){}
State::~State(){}

StateType State::GetType() const
{
    return this->type;
}

bool State::EchoMsgHandle(const string &msg)
{
    Echo echo_msg;
    string data;

    if (echo_msg.ParseFromString(msg)) {
        if (echo_msg.has_time_stamp()) {
            LOG_DEBUG(MODULE_COMMON, "echo[%s]", echo_msg.time_stamp().c_str());
        }
    }
    
    echo_msg.set_time_stamp("Echo Reply from Server");
    
    echo_msg.SerializeToString(&data);
    
    stateMachine->MessageReply(MSG_ECHO, data);

    return true;
}

bool State::GameHallSumaryHandle(const string &msg)
{
    GameHallSumary sumary;
    HallInfo *hallInfo = NULL;
    GameHall *gameHall = NULL;
    string data;
    
    GameHallSumaryReq sumaryReq;

    if (!sumaryReq.ParseFromString(msg)) {
        LOG_ERROR(MODULE_COMMON, "ParseFromString failed.");
        return false;
    }

    sumary.set_username(stateMachine->account);
    sumary.set_score(stateMachine->score);
    sumary.set_hall_num(MainThread::gameHallMaxNum);
    sumary.set_head_picture("Unknown head_picture");
    sumary.set_ad_picture1("Unknown ad_picture1");
           
    for (u_int32 i = 1; i <= sumary.hall_num(); ++i) {
        hallInfo = sumary.add_hall_info();
        hallInfo->set_game_hall_id(i);
        gameHall = MainThread::GetMainThreadObj()->GetGameHall(i);
        hallInfo->set_total_people(gameHall->GetExpectedTotalPeopleNum());
        hallInfo->set_curr_people(gameHall->GetCurrentPeopleNum());
        hallInfo->set_total_chessboard(MainThread::chessBoardMaxNum);
        /*ChessBoardInfo *chessBoard = NULL;
                    for (u_int32 j=1; j <= hallInfo->total_chessboard(); ++j) {
                               chessBoard = hallInfo->add_chess_board();
                               chessBoard->set_id(j);
                               chessBoard->set_people_num(gameHall->GetChessBoardByID(j)->GetUserNum());
                    }*/
    }

    sumary.SerializeToString(&data);
    stateMachine->MessageReply(MSG_GAME_HALL_SUMARY,data);

    return true;
}

bool State::HallInfoReqHandle(const string &msg)
{
    HallInfoReq req;
    HallInfo hallInfo;
    ChessBoardInfo *chessBoardInfo = NULL;
    ChessBoard *chessBoard = NULL;
    GameHall*gameHall = NULL;
    string data;

    if (req.ParseFromString(msg)) {
        gameHall = MainThread::GetMainThreadObj()->GetGameHall(req.game_hall_id());
        if (gameHall != NULL) {
            hallInfo.set_game_hall_id(req.game_hall_id());
            hallInfo.set_total_people(gameHall->GetExpectedTotalPeopleNum());
            hallInfo.set_curr_people(gameHall->GetCurrentPeopleNum());
            hallInfo.set_total_chessboard(MainThread::chessBoardMaxNum);
            for (u_int32 j=1; j <= hallInfo.total_chessboard(); ++j) {
                chessBoard = gameHall->GetChessBoardByID(j);
                
                chessBoardInfo = hallInfo.add_chess_board();
                chessBoardInfo->set_id(j);
                chessBoardInfo->set_people_num(gameHall->GetChessBoardByID(j)->GetUserNum());

                ChessBoardUser *left = chessBoardInfo->mutable_left_user();
                left->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_LEFT)==NULL) ? true : false);
                
                if (!left->chess_board_empty()) {
                    left->set_user_name(chessBoard->GetUserByLocation(LOCATION_LEFT)->account);
                    left->set_score(chessBoard->GetUserByLocation(LOCATION_LEFT)->score);
                    left->set_status(chessBoard->GetUserByLocation(LOCATION_LEFT)->stateMachine->GetType());
                    left->set_head_image("Unknown head image");
                }
    
                ChessBoardUser *right = chessBoardInfo->mutable_right_user();
                right->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_RIGHT)==NULL) ? true : false);
                if (!right->chess_board_empty()) {
                    right->set_user_name(chessBoard->GetUserByLocation(LOCATION_RIGHT)->account);
                    right->set_score(chessBoard->GetUserByLocation(LOCATION_RIGHT)->score);
                    right->set_status(chessBoard->GetUserByLocation(LOCATION_RIGHT)->stateMachine->GetType());
                    right->set_head_image("Unknown head image");
                }
    
                ChessBoardUser *bottom = chessBoardInfo->mutable_bottom_user();
                bottom->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_BOTTOM)==NULL) ? true : false);
                if (!bottom->chess_board_empty()) {
                    bottom->set_user_name(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->account);
                    bottom->set_score(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->score);
                    bottom->set_status(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->stateMachine->GetType());
                    bottom->set_head_image("Unknown head image");
                }
            }
        } else {
            LOG_ERROR(MODULE_COMMON, "Get gameHall failed.");
        }
    } else {
        LOG_ERROR(MODULE_COMMON, "ParseFromString failed.");
    }

    hallInfo.SerializeToString(&data);
    stateMachine->MessageReply(MSG_HALL_INFO, data);

    return true;
}

