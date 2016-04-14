#include "debug.h"
#include "utils.h"
#include "md5.h"
#include "mysqldb.h"
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "usersession.h"

using namespace std;

#define LOG_FILE        "/tmp/chessAdinit.log"
#define IMAGE_PREFIX    "/home/gbx386/libevent_test/Images/"

typedef enum {
    IMAGE_ID_AD_HALL_TOP_LEFT = 0,
    IMAGE_ID_AD_HALL_TOP_RIGHT,
    IMAGE_ID_AD_HALL_BOTTOM_LEFT,
    IMAGE_ID_AD_HALL_BOTTOM_RIGHT,
    IMAGE_ID_AD_BOARD_TOP_LEFT,
    IMAGE_ID_AD_BOARD_TOP_RIGHT,
    IMAGE_ID_AD_BOARD_BOTTOM_LEFT,
    IMAGE_ID_AD_BOARD_BOTTOM_RIGHT,
    IMAGE_ID_AD_BOARD_MIDDLE,
    IMAGE_ID_LOGIN_BACKGROUND,
    IMAGE_ID_MAX
}ImageID;

struct ImageTuple {
    ImageID image_id;
    char  locat_path[255];
};

const ImageTuple AdImageMap[] = {
    {IMAGE_ID_AD_HALL_BOTTOM_LEFT, IMAGE_PREFIX"Advertisement/GamehallBottomLeftAd/"},
    {IMAGE_ID_AD_HALL_BOTTOM_RIGHT, IMAGE_PREFIX"Advertisement/GamehallBottomRightAd/"},
    {IMAGE_ID_AD_HALL_TOP_LEFT, IMAGE_PREFIX"Advertisement/GamehallTopLeftAd/"},
    {IMAGE_ID_AD_HALL_TOP_RIGHT, IMAGE_PREFIX"Advertisement/GamehallTopRightAd/"},    
    {IMAGE_ID_AD_BOARD_BOTTOM_LEFT, IMAGE_PREFIX"Advertisement/ChessBoardBottomLeftAd/"},        
    {IMAGE_ID_AD_BOARD_BOTTOM_RIGHT, IMAGE_PREFIX"Advertisement/ChessBoardBottomRightAd/"},
    {IMAGE_ID_AD_BOARD_MIDDLE, IMAGE_PREFIX"Advertisement/ChessBoardMiddleAd/"},
    {IMAGE_ID_AD_BOARD_TOP_LEFT, IMAGE_PREFIX"Advertisement/ChessBoardTopLeftAd/"},
    {IMAGE_ID_AD_BOARD_TOP_RIGHT, IMAGE_PREFIX"Advertisement/ChessBoardTopRightAd/"},
    {IMAGE_ID_LOGIN_BACKGROUND, IMAGE_PREFIX"LogginBackground/"}
};

#define AdImageMapNum  (sizeof(AdImageMap)/sizeof(ImageTuple))
MYSQL *pdb_con = NULL;

static const string GetImageMd5HashCode(const string &path);
static bool IniSQLConnection();
static void UpdateImageInfoToDB(const AdPicturesInfo &ad_info);
static const string GetImageLinkUrl(const string &path);

int main() 
{    
    log_init(LOG_FILE, 102400, ERROR);
    IniSQLConnection();
    
    LOG_DEBUG(MODULE_COMMON, "Begain execute...");

    u_int32 i = 0;
    DIR *dirptr = NULL;
    struct dirent *entry;
    struct stat statbuf;
    
    for (i =0; i < AdImageMapNum; i ++) {
        LOG_DEBUG(MODULE_COMMON, "AdImageMap[%d]=%s", i, AdImageMap[i].locat_path);
        if((dirptr = opendir(AdImageMap[i].locat_path)) == NULL) {
            LOG_DEBUG(MODULE_COMMON, "open dir error !");
            
        } else {
            while ((entry = readdir(dirptr)) != NULL) {
                string path(AdImageMap[i].locat_path);
                path += entry->d_name;                
                LOG_DEBUG(MODULE_COMMON, "%s", path.c_str());
                    
                lstat(path.c_str(), &statbuf);
                if(!S_ISDIR(statbuf.st_mode)) {
                    LOG_DEBUG(MODULE_COMMON, "[%s] is a file, and size[%d].", entry->d_name,statbuf.st_size);
                    if (strcmp(entry->d_name, "link_url.txt")) {
                        AdPicturesInfo ad_info;
                        ad_info.image_name = entry->d_name;
                        ad_info.locate_path = path;
                        ad_info.image_id = AdImageMap[i].image_id;
                        //ad_info.image_type = "*.png|*.jpeg|*.bmp";
                        ad_info.image_size = statbuf.st_size;
                        ad_info.link_url = GetImageLinkUrl(string(AdImageMap[i].locat_path) + "link_url.txt");
                        ad_info.existed = true;
                        ad_info.image_hashcode = GetImageMd5HashCode(path);

                        std::size_t found = ad_info.image_name.find_last_of(".");
                        ad_info.image_type = "*" + ad_info.image_name.substr(found);
                        
                        LOG_DEBUG(MODULE_COMMON, "Got the hash code[%s]", ad_info.image_hashcode.c_str());

                        UpdateImageInfoToDB(ad_info);

                        break;//assume that just has one image file!
                    }
                    
                }
            }
            closedir(dirptr);
        }
    }
    
    log_close();
    mysql_db_close(pdb_con);
    
    return 0; 
}

const string GetImageMd5HashCode(const string &path)
{
    FILE *fptr = NULL;
    char buf[1024] = { 0 };
    size_t rdSize = 0;
    string md5_hash_code;

    fptr = fopen(path.c_str(), "r");
    if (fptr != NULL) {
        MD5 *md5hash = new MD5();
        while((rdSize = fread(buf, 1, sizeof(buf), fptr)) > 0) {
            md5hash->update(buf, rdSize);            
        }
        md5hash->finalize();
        md5_hash_code = md5hash->hexdigest();
        
        delete md5hash;

        fclose(fptr);
    }

    return md5_hash_code;
}

const string GetImageLinkUrl(const string &path)
{
    FILE *fptr = NULL;
    char buf[1024] = { 0 };
    string url = "";
    
    fptr = fopen(path.c_str(), "r");
    if (fptr != NULL) {
        if(fgets(buf, sizeof(buf), fptr) != NULL) {
            url = buf;
        }
        
        fclose(fptr);
    }

    return url;
}

bool IniSQLConnection()
{
    int ret = 0;
    const char *sql = "DROP TABLE ad_pictures";

    mysql_db_init(&pdb_con);
    if (pdb_con == NULL) {
        LOG_ERROR(MODULE_DB, "mysql_db_init failed.");
        return false;
    }
        
    ret = mysql_db_connect(pdb_con, "123.57.180.67", "gbx386", "760226", "ThreePeoplePlayChessDB");
    if (ret != 0) {
        LOG_ERROR(MODULE_DB, "mysql_db_connect failed.");
        return false;
    }

    mysql_db_excute(pdb_con, sql, strlen(sql));

    creat_ad_pictures_table(pdb_con);

    return true;
}

void UpdateImageInfoToDB(const AdPicturesInfo &ad_info)
{
    int ret = 0;
    char sql_cmd[1024] = {0};

    snprintf(sql_cmd, sizeof(sql_cmd)-1, "INSERT INTO ad_pictures (id,existed,image_name,image_type,image_hashcode,image_size,link_url,locate_path) \
VALUES (%d,%d,'%s','%s','%s',%d,'%s','%s')",\
            ad_info.image_id,ad_info.existed? 1 : 0, ad_info.image_name.c_str(), ad_info.image_type.c_str(), ad_info.image_hashcode.c_str(),
            ad_info.image_size, ad_info.link_url.c_str(), ad_info.locate_path.c_str());
            
    ret = mysql_db_excute(pdb_con, sql_cmd, strlen(sql_cmd));
    if (ret != 0) {
        LOG_INFO(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        /*
        //Should insert it!
        snprintf(sql_cmd, sizeof(sql_cmd)-1, "UPDATE ad_pictures SET existed=%d,image_name='%s',image_type='%s',image_hashcode='%s',image_size=%d,\
link_url='%s',locate_path='%s' WHERE id=%d", ad_info.existed? 1 : 0, ad_info.image_name.c_str(), ad_info.image_type.c_str(), ad_info.image_hashcode.c_str(),
            ad_info.image_size, ad_info.link_url.c_str(), ad_info.locate_path.c_str(),ad_info.image_id);


        LOG_DEBUG(MODULE_DB, "sql_cmd[%s]", sql_cmd);
        ret = mysql_db_excute(pdb_con, sql_cmd, strlen(sql_cmd));
        if (ret != 0) {
            LOG_ERROR(MODULE_DB, "mysql_db_excute failed, ret[%d]", ret);
        }*/
    } else {
        LOG_DEBUG(MODULE_DB, "Insert successfully!");
    }
}

