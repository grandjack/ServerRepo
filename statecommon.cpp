#include "state.h"
#include "usersession.h"
#include <string>
#include <string.h>
#include "utils.h"
#include "debug.h"
#include "thread.h"
#include "mainthread.h"
#include "chessboard.h"
#include "md5.h"
#include <stdio.h>

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

    sumary.set_account(stateMachine->user_info.account);
    sumary.set_username(stateMachine->user_info.user_name);
    sumary.set_score(stateMachine->user_info.score);
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
            if (chessBoard != NULL) {
                
                chessBoardInfo = hallInfo.add_chess_board();

                chessBoard->WrapChessBoardInfo(*chessBoardInfo);
            }
        }

        hallInfo.SerializeToString(&data);
    } else {
        LOG_ERROR(MODULE_COMMON, "Get gameHall object failed!");
        ret = false;
    }

    return ret;
}

bool State::UpdateUserInfos(const string &msg)
{
    UpdateUserInfo info;
    LOG_DEBUG(MODULE_COMMON, "UpdateUserInfos ...");

    try {
    if (info.ParseFromString(msg)) {
        

        //TODO other things here!! Update the info to database
        if (info.user_name().compare(stateMachine->user_info.user_name)) {
            stateMachine->user_info.user_name = info.user_name();
            stateMachine->thread->UpdateUserNameToDB(stateMachine->user_info.account, stateMachine->user_info.user_name);
        }

        if (info.password().compare(stateMachine->user_info.password)) {
            stateMachine->user_info.password = info.password();
            stateMachine->thread->UpdateUserPasswordToDB(stateMachine->user_info.account, stateMachine->user_info.password);
        }

        if (info.has_ex_email()) {
            if (info.ex_email().compare(stateMachine->user_info.email)) {
                stateMachine->user_info.email = info.ex_email();
                stateMachine->thread->UpdateUserEmailToDB(stateMachine->user_info.account, stateMachine->user_info.email);
            }
        }

        if (info.has_phone()) {
            if (info.phone().compare(stateMachine->user_info.phone)) {
                stateMachine->user_info.phone = info.phone();
                stateMachine->thread->UpdateUserPhoneToDB(stateMachine->user_info.account, stateMachine->user_info.phone);
            }
        }

        if (info.has_head_image()) {
            if (info.head_image().size() > 0) {
                stateMachine->user_info.head_photo = info.head_image();
                stateMachine->thread->UpdateHeadImageToDB(stateMachine->user_info.account, stateMachine->user_info.head_photo);
            }
        }
        
        LOG_DEBUG(MODULE_COMMON, "account :%s", stateMachine->user_info.account.c_str());
        LOG_DEBUG(MODULE_COMMON, "user_name :%s", stateMachine->user_info.user_name.c_str());
        LOG_DEBUG(MODULE_COMMON, "email :%s", stateMachine->user_info.email.c_str());
        LOG_DEBUG(MODULE_COMMON, "password :%s", stateMachine->user_info.password.c_str());
        LOG_DEBUG(MODULE_COMMON, "head_photo.size :%d", stateMachine->user_info.head_photo.size());
        
    }
    } catch(exception &e) {
        LOG_ERROR(MODULE_COMMON, "Parse user info failed!");
    }

    return true;
}


StateAdPictureDownload::StateAdPictureDownload(StateMachine *machine):State(machine)
{
    type = STATE_DOWN_AD;
}

StateAdPictureDownload::~StateAdPictureDownload(){}

bool StateAdPictureDownload::MsgHandle(const u_int32 msg_type, const string &msg)
{
    bool ret = false;

    /*handling for auth message*/
    switch(msg_type)
    {
        case MSG_AD_IMAGE_REQ:
            ret = AdPictureItemHandle(msg);
            break;
        
        case MSG_ECHO:
            ret = EchoMsgHandle(msg);
            break;

        default:
            break;
    }

    return ret;
}

bool StateAdPictureDownload::AdPictureItemHandle(const string &msg)
{
    bool ret = true;
    //AdPictureItemReply item;
    AdPicturesInfo info;
    AdPictureContentReply reply;
    AdPictureReq item;
    string data;

    if (item.ParseFromString(msg)) {
        if (item.has_last_one()) {
            if (item.last_one()) {
                stateMachine->SetNextState(new StateGameReady(stateMachine));
            }
        }

        stateMachine->thread->GetAdPicturesInfoFromDB(item.image_id(), info);
        DownloadImageInfo(info);

        {
            if (info.existed) {
                if (item.has_image_hashcode()) {
                    if (info.image_hashcode.compare(item.image_hashcode())) {//not equal
                        //download the image
                        DownloadImage(info.locate_path);
                    } else {
                        //ignore
                        reply.set_synced(false);
                        reply.SerializeToString(&data);
                        stateMachine->MessageReply(MSG_AD_IMAGE_CONTENT, data);
                    }
                } else {
                    //download the image
                    DownloadImage(info.locate_path);
                }
            } else {
                reply.set_synced(false);
                reply.SerializeToString(&data);
                stateMachine->MessageReply(MSG_AD_IMAGE_CONTENT, data);
            }
        }
    }

    return ret;
}

void StateAdPictureDownload::DownloadImage(const string &file_path)
{
    FILE *fptr = NULL;
    AdPictureContentReply reply;
    string data;
    char buf[1024] = { 0 };
    size_t rdSize = 0;

    fptr = fopen(file_path.c_str(), "r");
    if (fptr != NULL) {
        while((rdSize = fread(buf, 1, sizeof(buf), fptr)) > 0) {
            const string sub_data(buf, rdSize);
            reply.set_synced(true);
            reply.set_content(sub_data);
            reply.SerializeToString(&data);
            stateMachine->MessageReply(MSG_AD_IMAGE_CONTENT, data);
        }

        reply.set_synced(false);
        reply.set_ended(true);
        reply.SerializeToString(&data);
        stateMachine->MessageReply(MSG_AD_IMAGE_CONTENT, data);

        fclose(fptr);
    }
}

void StateAdPictureDownload::DownloadImageInfo(const AdPicturesInfo &ad_info)
{
    AdPictureItemReply reply;
    string data;

    reply.set_image_id(ad_info.image_id);
    reply.set_existed(ad_info.existed);
    
    if (ad_info.existed) {
        reply.set_image_type(ad_info.image_type);
        reply.set_image_name(ad_info.image_name);
        reply.set_image_hashcode(ad_info.image_hashcode);
        reply.set_url(ad_info.link_url);
        reply.set_image_size(ad_info.image_size);
    }
    
    reply.SerializeToString(&data);
    stateMachine->MessageReply(MSG_AD_IMAGE_INFO, data);

}

