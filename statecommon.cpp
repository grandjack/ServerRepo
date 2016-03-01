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

    sumary.set_account(stateMachine->account);
    sumary.set_username(stateMachine->user_name);
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
    string data;

    if (req.ParseFromString(msg)) {
        if (!WrapHallInfo(req.game_hall_id(), data)) {
            LOG_ERROR(MODULE_COMMON, "WrapHallInfo failed.");
        } else {            
            stateMachine->MessageReply(MSG_HALL_INFO, data);
        }
    } else {
        LOG_ERROR(MODULE_COMMON, "ParseFromString failed.");
    }

    return true;
}

bool State::WrapHallInfo(const u_int32 hall_id, string &data)
{
    HallInfo hallInfo;
    ChessBoardInfo *chessBoardInfo = NULL;
    ChessBoard *chessBoard = NULL;
    GameHall*gameHall = NULL;
    bool ret = true;
    
    gameHall = MainThread::GetMainThreadObj()->GetGameHall(hall_id);
    if (gameHall != NULL)
    {
        hallInfo.set_game_hall_id(hall_id);
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
                left->set_account(chessBoard->GetUserByLocation(LOCATION_LEFT)->account);
                left->set_user_name(chessBoard->GetUserByLocation(LOCATION_LEFT)->user_name);
                left->set_score(chessBoard->GetUserByLocation(LOCATION_LEFT)->score);
                left->set_status(chessBoard->GetUserByLocation(LOCATION_LEFT)->stateMachine->GetType());
                left->set_head_image("Unknown head image");
            }
    
            ChessBoardUser *right = chessBoardInfo->mutable_right_user();
            right->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_RIGHT)==NULL) ? true : false);
            if (!right->chess_board_empty()) {
                right->set_account(chessBoard->GetUserByLocation(LOCATION_RIGHT)->account);
                right->set_user_name(chessBoard->GetUserByLocation(LOCATION_RIGHT)->user_name);
                right->set_score(chessBoard->GetUserByLocation(LOCATION_RIGHT)->score);
                right->set_status(chessBoard->GetUserByLocation(LOCATION_RIGHT)->stateMachine->GetType());
                right->set_head_image("Unknown head image");
            }
    
            ChessBoardUser *bottom = chessBoardInfo->mutable_bottom_user();
            bottom->set_chess_board_empty((chessBoard->GetUserByLocation(LOCATION_BOTTOM)==NULL) ? true : false);
            if (!bottom->chess_board_empty()) {
                bottom->set_account(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->account);
                bottom->set_user_name(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->user_name);
                bottom->set_score(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->score);
                bottom->set_status(chessBoard->GetUserByLocation(LOCATION_BOTTOM)->stateMachine->GetType());
                bottom->set_head_image("Unknown head image");
            }
        }

        hallInfo.SerializeToString(&data);
    } else {
        LOG_ERROR(MODULE_COMMON, "Get gameHall object failed!");
        ret = false;
    }

    return ret;
}

void State::WrapChessBoardInfo()
{
    ChessBoardInfo chessBoard;
    string data;
    ChessBoard *chess = stateMachine->currChessBoard;
        
    chessBoard.set_id(chess->GetChessBoardID());
    chessBoard.set_people_num(chess->GetUserNum());
    
    ChessBoardUser *left = chessBoard.mutable_left_user();
    left->set_chess_board_empty((chess->GetUserByLocation(LOCATION_LEFT)==NULL) ? true : false);
                    
    if (!left->chess_board_empty()) {
        left->set_account(chess->GetUserByLocation(LOCATION_LEFT)->account);
        left->set_user_name(chess->GetUserByLocation(LOCATION_LEFT)->user_name);
        left->set_score(chess->GetUserByLocation(LOCATION_LEFT)->score);
        left->set_status(chess->GetUserByLocation(LOCATION_LEFT)->stateMachine->GetType());
        left->set_head_image("Unknown head image");
    }
        
    ChessBoardUser *right = chessBoard.mutable_right_user();
    right->set_chess_board_empty((chess->GetUserByLocation(LOCATION_RIGHT)==NULL) ? true : false);
    if (!right->chess_board_empty()) {
        right->set_account(chess->GetUserByLocation(LOCATION_RIGHT)->account);
        right->set_user_name(chess->GetUserByLocation(LOCATION_RIGHT)->user_name);
        right->set_score(chess->GetUserByLocation(LOCATION_RIGHT)->score);
        right->set_status(chess->GetUserByLocation(LOCATION_RIGHT)->stateMachine->GetType());
        right->set_head_image("Unknown head image");
    }
        
    ChessBoardUser *bottom = chessBoard.mutable_bottom_user();
    bottom->set_chess_board_empty((chess->GetUserByLocation(LOCATION_BOTTOM)==NULL) ? true : false);
    if (!bottom->chess_board_empty()) {
        bottom->set_account(chess->GetUserByLocation(LOCATION_BOTTOM)->account);
        bottom->set_user_name(chess->GetUserByLocation(LOCATION_BOTTOM)->user_name);
        bottom->set_score(chess->GetUserByLocation(LOCATION_BOTTOM)->score);
        bottom->set_status(chess->GetUserByLocation(LOCATION_BOTTOM)->stateMachine->GetType());
        bottom->set_head_image("Unknown head image");
    }

    chessBoard.SerializeToString(&data);
    stateMachine->MessageReply(MSG_CHESS_BOARD, data);

}

bool State::UpdateUserInfos(const string &msg)
{
    UpdateUserInfo info;
    
    if (info.ParseFromString(msg)) {
        stateMachine->user_name = info.user_name();
        stateMachine->password = info.password();
        stateMachine->email = info.ex_email();

        stateMachine->user_info.user_name= stateMachine->user_name;
        stateMachine->user_info.email = stateMachine->email;
        stateMachine->user_info.password = stateMachine->password;
        stateMachine->user_info.head_photo = info.head_image();

        //TODO other things here!! Update the info to database
    }

    return true;
}

