
#ifndef __MYSQL_DB_HEAD__
#define __MYSQL_DB_HEAD__

#include <mysql/mysql.h>
#include "usersession.h"
#include "state.h"

#define CR_SERVER_GONE_ERROR    2006

int mysql_db_init(MYSQL **connection);

int mysql_db_close(MYSQL *connection);

int mysql_db_connect(MYSQL *connection, const char *host, const char * user, const char * pwd, const char * db_name);  
  
int mysql_db_query(MYSQL *connection, const char * sql, UsersInfo &user_info);

int mysql_db_excute(MYSQL *mysql, const char * query_string, int len);

int creat_users_info_table(MYSQL *mysql);

int mysql_get_binary_data(MYSQL *mysql, const char * query_string, int len, string &data);

int creat_ad_pictures_table(MYSQL *mysql);

int mysql_db_query_ad_info(MYSQL *connection, const char * sql, AdPicturesInfo &ad_info);

int creat_image_version_table(MYSQL *mysql);

int mysql_db_query_image_version(MYSQL *connection, const char * sql, ImageVersions &image_info);

#endif /*__MYSQL_DB_HEAD__*/
