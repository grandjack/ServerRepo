#include "chessboard.h"
#include "debug.h"
#include "state.h"
#include "usersession.h"
#include <string>
#include <string.h>
#include "utils.h"
#include "thread.h"
#include "chessboard.h"
#include "mainthread.h"

using namespace MessageStruct;

GameHall::GameHall(u_int32 id, u_int32 num):gameHallID(id),chessBoardMaxNum(num)
{
    pthread_mutex_init(&mutex, NULL);
}
GameHall::~GameHall()
{
    pthread_mutex_destroy(&mutex);
}

u_int32 GameHall::GetChessValidBoardNum() const
{
    return chessBoardInfo.size();
}

u_int32 GameHall::GetExpectedTotalPeopleNum() const
{
    return chessBoardMaxNum*3;
}

u_int32 GameHall::GetCurrentPeopleNum() const
{
    map<u_int32, ChessBoard*>::const_iterator  iter;
    u_int32 num = 0;
    ChessBoard *chessBoard = NULL;

    for (iter = chessBoardInfo.begin(); iter != chessBoardInfo.end(); iter++) {
        chessBoard = static_cast<ChessBoard *>(iter->second);
        if (chessBoard != NULL) {
            num += chessBoard->GetUserNum();
        }
    }

    return num;
}

ChessBoard* GameHall::GetChessBoardByID(u_int32 id)
{
    ChessBoard *chessBoard = NULL;

    if (id <= 0 || id > chessBoardMaxNum) {
        LOG_ERROR(MODULE_COMMON, " Invalid Chess Board ID.");
        return NULL;
    }
    
    try {
        chessBoard = static_cast<ChessBoard *>(chessBoardInfo.find(id)->second);
    } catch (const exception &e) {
        chessBoard = NULL;
    }

    if (NULL == chessBoard) {
        if(CreateChessBoard(id)) {
            chessBoard = static_cast<ChessBoard *>(chessBoardInfo.find(id)->second);
        }
    }
    
    return chessBoard;
}

bool GameHall::CreateChessBoard(u_int32 id)
{
    bool ret = false;
    ChessBoard *chessBoard = NULL;

    pthread_mutex_lock(&mutex);
    if (GetChessValidBoardNum() > chessBoardMaxNum) {
        LOG_ERROR(MODULE_COMMON, "Exceed max gamehall num.");
        ret = false;
    } else {
    
        chessBoard = new ChessBoard(id, this);
        if (chessBoard != NULL) {
            chessBoardInfo.insert(pair<int, ChessBoard*>(id, chessBoard));
            ret = true;
        } else {
            ret = false;
            LOG_ERROR(MODULE_COMMON, "New ChessBoard failed.");
        }
    }    
    pthread_mutex_unlock(&mutex);
    
    return ret;
}

u_int32 GameHall::GetGameHallID() const
{
    return gameHallID;
}


/*Class ChessBoard definitions*/

ChessBoard::ChessBoard(u_int32 id, GameHall *gameHall):leftUser(NULL),rightUser(NULL),bottomUser(NULL),chessBoardID(id),currentHall(gameHall),
                                        currUserNum(0),first_come_user_locate((u_int32)LOCATION_MAX), total_time(0), single_step_time(0),
                                        recent_winner_locate(LOCATION_MAX),recent_loser_locate(LOCATION_MAX)
{
    pthread_mutex_init(&mutex, NULL);
}

ChessBoard::~ChessBoard()
{
    pthread_mutex_destroy(&mutex);
}

UserSession* ChessBoard::GetUserByLocation(const Location &locate)
{
    UserSession *user = NULL;
    
    switch(locate)
    {
        case LOCATION_LEFT:
            user = leftUser;
            break;
        case LOCATION_RIGHT:
            user = rightUser;
            break;
        case LOCATION_BOTTOM:
            user = bottomUser;
            break;

        default:
            break;
    }

    return user;
}

bool ChessBoard::HasFullUsers()
{
    bool ret = false;

    if (currUserNum == 3)
    {
        ret = true;
    }

    return ret;
}

u_int32 ChessBoard::GetUserNum() const
{
    return currUserNum;
}

void ChessBoard::BroadCastMsg(const MessageType type, const string &str, int locate_filter)
{
    UserSession *user = NULL;

    for (int locate = (int)LOCATION_UNKNOWN; locate < (int)LOCATION_MAX; ++locate) {
        if (locate != locate_filter) {
            if ((user = GetUserByLocation((Location)locate)) != NULL) {
                user->MessageReply(type, str);
            }
        }
    }
}

bool ChessBoard::AddUser(UserSession *user)
{
    bool ret = false;

    pthread_mutex_lock(&mutex);
    if ((user != NULL) && (!HasFullUsers())) {
        switch(user->locate)
        {
            case LOCATION_LEFT:
                if (leftUser == NULL) {
                    leftUser = user;
                    ret = true;
                    ++currUserNum;
                }
                break;
            case LOCATION_RIGHT:
                if (rightUser== NULL) {
                    rightUser = user;
                    ret = true;
                    ++currUserNum;
                }
                break;
            case LOCATION_BOTTOM:
                if (bottomUser== NULL) {
                    bottomUser = user;
                    ret = true;
                    ++currUserNum;
                }
                break;
            
            default:
                ret = false;
                break;
        }
    }
    pthread_mutex_unlock(&mutex);

    return ret;
}

bool ChessBoard::LeaveOutRoom(const UserSession *user)
{
    bool ret = false;
    
    pthread_mutex_lock(&mutex);
    if ((user != NULL) && (GetUserNum() > 0)) {
        switch(user->locate)
        {
            case LOCATION_LEFT:
                if (leftUser != NULL) {
                    leftUser = NULL;
                    ret = true;
                    --currUserNum;
                }
                break;
            case LOCATION_RIGHT:
                if (rightUser != NULL) {
                    rightUser = NULL;
                    ret = true;
                    --currUserNum;
                }
                break;
            case LOCATION_BOTTOM:
                if (bottomUser != NULL) {
                    bottomUser = NULL;
                    ret = true;
                    --currUserNum;
                }
                break;
            
            default:
                ret = false;
                break;
        }
    }

    if (user->locate == (Location)first_come_user_locate) {
        first_come_user_locate = (u_int32)LOCATION_MAX;
    }
    pthread_mutex_unlock(&mutex);

    return ret;
}


bool ChessBoard::LeaveOutRoom(const Location &locate)
{
    bool ret = false;

    pthread_mutex_lock(&mutex);
    if (GetUserNum() > 0) {
        switch(locate)
        {
            case LOCATION_LEFT:
                if (NULL != leftUser) {
                    leftUser = NULL;
                    --currUserNum;
                    ret = true;
                }
                break;
                
            case LOCATION_RIGHT:
                if (NULL != rightUser) {
                    rightUser = NULL;
                    --currUserNum;
                    ret = true;
                }
                break;
                
            case LOCATION_BOTTOM:
                if (NULL != bottomUser) {
                    bottomUser = NULL;
                    --currUserNum;
                    ret = true;
                }
                break;

            default:
                ret = false;
                break;
        }
    }
    if (locate == (Location)first_come_user_locate) {
        first_come_user_locate = (u_int32)LOCATION_MAX;
    }
    pthread_mutex_unlock(&mutex);

    return ret;
}

bool ChessBoard::TranslateMsg(const u_int32 msg_type, const string &msg)
{
    bool ret = false;

    /*handling for auth message*/
    switch(msg_type)
    {            
        case MSG_MOVE_CHESS:
            ret = MoveChessHandle(msg);
            break;
            
        case MSG_USER_MSG:
            ret = UserMessageHandle(msg);
            break;
            
        case MSG_RECONCILED_REQ:
        case MSG_RECONCILED_RESP:
            ret = ReconciledHandle(msg, (MessageType)msg_type);
            break;
            
        case MSG_GIVE_UP:
            ret = GiveUpHandle(msg);
            break;
            
        case MSG_UNDO_REQ:
        case MSG_UNDO_REPS:
            ret = UndoHandle(msg, (MessageType)msg_type);
            break;

        case MSG_GAME_READY_REQ:
            ret = GameReadyHandle(msg);
            break;
            
        default:
            break;
    }

    return ret;
}

//Notes
//Thread 1 call the function of the object that belongs to Thread 2, and which thread will the executing on?
//How about change the target function to static ?
//Cautions
//
bool ChessBoard::MoveChessHandle(const string &msg)
{
    MoveChess moveChess;
    MoveChess* pmoveChess = NULL;
    MoveAction announceAction;
    string data;
    UserSession *src_user = NULL;
    UserSession *tar_user = NULL;

    if (moveChess.ParseFromString(msg)) {
        src_user = GetUserByLocation((Location)moveChess.src_user_locate());
        if (moveChess.is_winner()) {
            //Winer plus 10 score
            recent_winner_locate = (Location)moveChess.src_user_locate();
            recent_loser_locate = (Location)moveChess.target_user_locate();
            
            src_user->IncreaseScore();
            if ((tar_user = GetUserByLocation((Location)moveChess.target_user_locate())) != NULL) {
                tar_user->ReduceScore();
                tar_user->gameOver = true;
                tar_user->status = STATUS_ENDED;
            } else {
                LOG_ERROR(MODULE_COMMON,"Get target user failed.");
            }
            
            LOG_DEBUG(MODULE_COMMON, "User[%s] won user locate[%d]", src_user->user_info.account.c_str(), moveChess.target_user_locate());
        } else {
            recent_winner_locate = LOCATION_MAX;
            recent_loser_locate = LOCATION_MAX;
        }
        LOG_DEBUG(MODULE_COMMON, "src user locate: %d, current user locate: %d", moveChess.src_user_locate(),src_user->locate);


        announceAction.set_src_user_locate(src_user->locate);
        if (GetActiveUsersNum() > 1) {
            if (src_user->locate == LOCATION_LEFT) {
                if (((tar_user = GetUserByLocation(LOCATION_BOTTOM)) != NULL) &&
                    tar_user->gameOver) {
                    announceAction.set_token_locate((u_int32)LOCATION_RIGHT);
                } else {
                    announceAction.set_token_locate((u_int32)LOCATION_BOTTOM);
                }
                
            } else if (src_user->locate == LOCATION_BOTTOM) {
                if (((tar_user = GetUserByLocation(LOCATION_RIGHT)) != NULL) &&
                    tar_user->gameOver) {
                    announceAction.set_token_locate((u_int32)LOCATION_LEFT);
                } else {
                    announceAction.set_token_locate((u_int32)LOCATION_RIGHT);
                }
                
            } else if (src_user->locate == LOCATION_RIGHT) {
                if (((tar_user = GetUserByLocation(LOCATION_LEFT)) != NULL) &&
                    tar_user->gameOver) {
                    announceAction.set_token_locate((u_int32)LOCATION_BOTTOM);
                } else {
                    announceAction.set_token_locate((u_int32)LOCATION_LEFT);
                }
            }
        } else {
            // src_user is the last winner!!
            announceAction.set_token_locate((u_int32)LOCATION_MAX);
            LOG_DEBUG(MODULE_COMMON, "######## The last winner is %s", src_user->user_info.account.c_str());
        }
        
        pmoveChess = announceAction.mutable_movechess();
        *pmoveChess = moveChess;


        announceAction.SerializeToString(&data);
        //Thread 1 call the function of the object that belongs to Thread 2, and which thread will the executing on?
        //How about change the target function to static ?
        //Cautions
        //
        //Notes
        BroadCastMsg(MSG_ANNOUNCE_MOVE, data, LOCATION_MAX);

        //If has ate someone ,need to notify all users & update ststus
        if (moveChess.is_winner()) {
            ChessBoardInfo chessBoard;
            WrapChessBoardInfo(chessBoard);
            chessBoard.SerializeToString(&data);
            BroadCastMsg(MSG_CHESS_BOARD, data, LOCATION_MAX);
        }
    }

    return true;
}

bool ChessBoard::UserMessageHandle(const string &msg)
{
    UserMessage userMsg;
    
     if (userMsg.ParseFromString(msg)) {
        BroadCastMsg(MSG_USER_MSG, msg, userMsg.src_user_locate());
     }

     return true;
}


//Qiu He
bool ChessBoard::ReconciledHandle(const string &msg, MessageType type)
{
     Reconciled peace;
     UserSession *user = NULL;

    if (peace.ParseFromString(msg)) {
        if (type == MSG_RECONCILED_REQ || peace.apply_or_reply() == 0) {
            BroadCastMsg(type, msg, (Location)peace.src_user_locate());
        } else if (type == MSG_RECONCILED_RESP || peace.apply_or_reply() == 1) {
            //just reply for the original user
            if((user=GetUserByLocation((Location)peace.tar_user_locate())) != NULL) {
                user->MessageReply(type, msg);
            }
        }
    }

    return true;
}

bool ChessBoard::GiveUpHandle(const string &msg)
{
    GiveUp giveup;
    UserSession *user = NULL;
    
     if (giveup.ParseFromString(msg)) {
        if ((user = GetUserByLocation((Location)giveup.src_user_locate())) != NULL ) {
            //do other job here to notify the sender user what state he will go.

            LeaveRoomHandle(user);

            if ((giveup.has_opt()) && (!giveup.opt().compare("exit"))) {//exit from the game room                
                user->SetNextState(new StateGameReady(user));
            }
        }

     }

     return true;
}
//Hui Qi
bool ChessBoard::UndoHandle(const string &msg, MessageType type)
{
     Undo undo;
     UserSession *user = NULL;

    if (undo.ParseFromString(msg)) {
        //just translate the message
        if (undo.rep_or_respon() == 0) {//apply request
            user = GetUserByLocation((Location)undo.tar_user_locate());
            if (user) {
                user->MessageReply(type, msg);
            } else {
                LOG_ERROR(MODULE_COMMON, "Get the user failed, locate[%u]", undo.tar_user_locate());
            }
        } else if (undo.rep_or_respon() == 1) {
            LOG_DEBUG(MODULE_COMMON, "BroadCastMsg Undo response, tar_user_locate=%d", undo.tar_user_locate());
            BroadCastMsg(type, msg, (Location)undo.tar_user_locate());
            if ((undo.has_status()) && (undo.status())) {
                if (recent_winner_locate != LOCATION_MAX) {
                    try {
                        GetUserByLocation(recent_winner_locate)->ReduceScore();
                        GetUserByLocation(recent_loser_locate)->IncreaseScore();
                        GetUserByLocation(recent_loser_locate)->gameOver = false;
                    } catch(const exception &e) {
                        LOG_ERROR(MODULE_COMMON, "Set status failed.");
                    }
                }
            }
        }
    }


    return true;
}

u_int32 ChessBoard::GetActiveUsersNum() const
{
    u_int32 num = 0;
    if ((leftUser != NULL) && (leftUser->gameReady) &&(!leftUser->gameOver))
    {
        ++num;
    }

    if ((rightUser != NULL) && (rightUser->gameReady) && (!rightUser->gameOver))
    {
        ++num;
    }

    if ((bottomUser != NULL) && (bottomUser->gameReady) && (!bottomUser->gameOver))
    {
        ++num;
    }

    return num;
}

u_int32 ChessBoard::GetChessBoardID() const
{
    return chessBoardID;
}

bool ChessBoard::GameReadyHandle(const string &msg)
{
    GameReadyReq gameReady;
    GameStatusReply status;
    string data;
    UserSession *user = NULL;
    
    if (gameReady.ParseFromString(msg)) {
        user = GetUserByLocation((Location)gameReady.src_user_locate());
        if (user != NULL) {
            user->gameReady = true;
            user->status = STATUS_PLAYING;
        }

        if ((gameReady.src_user_locate() == first_come_user_locate) &&
            (gameReady.has_total_time()) &&
            (gameReady.has_single_step_time())) {
            this->total_time = gameReady.total_time();
            this->single_step_time = gameReady.single_step_time();
        }

        status.set_total_time(this->total_time);
        status.set_single_step_time(this->single_step_time);
        LOG_DEBUG(MODULE_COMMON, "total_time [%d]  single_step_time[%d]", total_time,single_step_time);

        if ((user = GetUserByLocation(LOCATION_LEFT)) != NULL) {
            status.set_left_user_status(user->gameReady);
        } else {
            status.set_left_user_status(false);
        }

        if ((user = GetUserByLocation(LOCATION_RIGHT)) != NULL) {
            status.set_right_user_status(user->gameReady);
        } else {
            status.set_right_user_status(false);
        }

        if ((user = GetUserByLocation(LOCATION_BOTTOM)) != NULL) {
            status.set_bottom_user_status(user->gameReady);
        } else {
            status.set_bottom_user_status(false);
        }

        status.set_token_locate(LOCATION_BOTTOM);

        status.SerializeToString(&data);

        BroadCastMsg(MSG_GAME_STATUS, data, LOCATION_MAX);
    }

    return true;
}

void ChessBoard::WrapChessBoardInfo(ChessBoardInfo &chessBoard)
{
    //ChessBoardInfo chessBoard;
    string data;
    ChessBoard *chess = this;
    UserSession *user = NULL;
        
    chessBoard.set_id(chess->GetChessBoardID());
    chessBoard.set_people_num(chess->GetUserNum());
    
    ChessBoardUser *left = chessBoard.mutable_left_user();
    left->set_chess_board_empty(((user=chess->GetUserByLocation(LOCATION_LEFT))==NULL) ? true : false);
                    
    if (!left->chess_board_empty()) {
        left->set_account(user->user_info.account);
        left->set_user_name(user->user_info.user_name);
        left->set_score(user->user_info.score);
        left->set_status(user->status);
        left->set_head_image(user->user_info.head_photo);
    }
        
    ChessBoardUser *right = chessBoard.mutable_right_user();
    right->set_chess_board_empty(((user=chess->GetUserByLocation(LOCATION_RIGHT))==NULL) ? true : false);
    if (!right->chess_board_empty()) {
        right->set_account(user->user_info.account);
        right->set_user_name(user->user_info.user_name);
        right->set_score(user->user_info.score);
        right->set_status(user->status);
        right->set_head_image(user->user_info.head_photo);
    }
        
    ChessBoardUser *bottom = chessBoard.mutable_bottom_user();
    bottom->set_chess_board_empty(((user=chess->GetUserByLocation(LOCATION_BOTTOM))==NULL) ? true : false);
    if (!bottom->chess_board_empty()) {
        bottom->set_account(user->user_info.account);
        bottom->set_user_name(user->user_info.user_name);
        bottom->set_score(user->user_info.score);
        bottom->set_status(user->status);
        bottom->set_head_image(user->user_info.head_photo);
    }

}

bool ChessBoard::GameBegine() const
{
    bool ret = false;
    int num = 0;
    
    if ((leftUser != NULL) && (leftUser->gameReady))
    {
        ++num;
    }

    if ((rightUser != NULL) && (rightUser->gameReady))
    {
        ++num;
    }

    if ((bottomUser != NULL) && (bottomUser->gameReady))
    {
        ++num;
    }

    if (num == 3) {
        ret = true;
    }

    return ret;
}

void ChessBoard::LeaveRoomHandle(UserSession *user)
{
    ChessBoardInfo chessBoardInfo;
    string data;
    GiveUp give_up;
        
    if (user == NULL) {
        return;
    }
    
    if (user->currChessBoard->GameBegine() && (!user->gameOver)) {
        user->ReduceScore(30);

        give_up.set_src_user_locate((unsigned int)user->locate);
        give_up.SerializeToString(&data);
        BroadCastMsg(MSG_GIVE_UP, data, (int)user->locate);
    }
    
    LeaveOutRoom(user);
    
    WrapChessBoardInfo(chessBoardInfo);
    chessBoardInfo.SerializeToString(&data);
    //notify the others that the user has exit
    BroadCastMsg(MSG_CHESS_BOARD, data, (int)user->locate);

    
    BroadCastHallInfo(user);
    
}

//temporary handle & because it may cross many threads !!!!!!!!!!!!!!!!!!!!!!!!!
void ChessBoard::BroadCastHallInfo(UserSession *user)
{
    string data;
    
    user->stateMachine->WrapHallInfo(currentHall->GetGameHallID(), data);

    u_int8 thread_num =  MainThread::GetMainThreadObj()->GetThreadsNum();
    for (u_int8 i = 0; i < thread_num; ++i) {
        WorkThread *thread = MainThread::GetMainThreadObj()->GetOneThreadByIndex(i);
        if (thread != NULL) {

            std::map<int,UserSession*>::iterator iter;
            for ( iter = thread->fdSessionMap.begin(); iter != thread->fdSessionMap.end(); iter++ ) {                
                UserSession *tmp_user = iter->second;
                if (tmp_user != NULL) {
                    tmp_user->MessageReply(MSG_HALL_INFO,data);
                    tmp_user->stateMachine->GameHallSumaryHandle(data);
                }
            }

        }
    }
   
}

