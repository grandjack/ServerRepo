#include "debug.h"
#include <stdlib.h>
#include "mysqldb.h"

int mysql_db_init(MYSQL **connection) {
	*connection = mysql_init(NULL); // 初始化数据库连接变量 
	
	if(*connection == NULL) {
		LOG_ERROR(MODULE_DB,"mysql_init error[%s]", mysql_error(*connection));
		return -1;
    }

	return 0;
}  
  
int mysql_db_close(MYSQL *connection) {
    if(connection != NULL) {// 关闭数据库连接  
        mysql_close(connection);
    }
	return 0;
}  

int mysql_db_connect(MYSQL *connection, const char *host, const char * user, const char * pwd, const char * db_name) {

    my_bool re_connect = true;

    mysql_options(connection, MYSQL_OPT_RECONNECT, &re_connect);

    connection = mysql_real_connect(connection, host,  
            user, pwd, db_name, 0, NULL, 0);
	
    if(connection == NULL) {
        LOG_ERROR(MODULE_DB,"mysql_real_connect error[%s] db_name[%s]", mysql_error(connection), db_name);
		return -1;
    }
	
    return 0;  
}  
  
int mysql_db_query(MYSQL *connection, const char * sql, UsersInfo &user_info) {
	MYSQL_ROW row;
	MYSQL_RES *result = NULL;
	int ret = 0;
	
    if(mysql_query(connection, sql)) {
        LOG_ERROR(MODULE_DB,"mysql_query error[%s]", mysql_error(connection));
		return -1;
    } else {
        result = mysql_use_result(connection);
		if (result == NULL) {
			LOG_INFO(MODULE_DB, "mysql_use_result false[%s].", sql);
			return -1;
		}
		unsigned int num_fileds = mysql_num_fields(result);
  
		int n = 0;
		while((row = mysql_fetch_row(result))) {
			unsigned int j;
			   
			for(j=0; j < num_fileds; ++j) {
				LOG_DEBUG(MODULE_DB, "%s", row[j]);
			}
            user_info.account = row[0];
            user_info.password = row[1];
            user_info.email = row[2] ? row[2] : "";
            user_info.score = atoi(row[3]);
            //user_info.head_photo = row[4];
            user_info.user_name = row[5] ? row[5] : "";
            user_info.phone = row[6] ? row[6] : ""; ;
			n++; 
		} 
		ret = n;
		mysql_free_result(result);
    }  
	
    return ret;
}

int mysql_db_excute(MYSQL *mysql, const char * query_string, int len) {

	int ret;
	if (mysql == NULL || query_string == NULL) {
		LOG_ERROR(MODULE_DB, "invalid arg.");
		return -1;
	}

again:
	
	if (len > 0) {
		if ((ret = mysql_real_query(mysql, query_string,(unsigned int) len)) != 0) {
			LOG_ERROR(MODULE_DB, "Failed to insert row, Error: %s, cmd[%s]", mysql_error(mysql), query_string);
			if (ret == CR_SERVER_GONE_ERROR) {
				if (mysql_ping(mysql) != 0) {
					LOG_ERROR(MODULE_DB, "mysql_ping error[%s].", mysql_error(mysql));
					return CR_SERVER_GONE_ERROR;
				}else {
					LOG_INFO(MODULE_DB, "reconnect to mysql successfully.");
					goto again;
				}
			} else {
			    return -1;
			}
		}
	}

	return 0;
}

int mysql_get_binary_data(MYSQL *mysql, const char * query_string, int len, string &data) {

	int ret;
	if (mysql == NULL || query_string == NULL) {
		LOG_ERROR(MODULE_DB, "invalid arg.");
		return -1;
	}

again:
	
	if (len > 0) {
		if ((ret = mysql_real_query(mysql, query_string,(unsigned int) len)) != 0) {
			LOG_ERROR(MODULE_DB, "Failed to insert row, Error: %s, cmd[%s]", mysql_error(mysql), query_string);
			if (ret == CR_SERVER_GONE_ERROR) {
				if (mysql_ping(mysql) != 0) {
					LOG_ERROR(MODULE_DB, "mysql_ping error[%s].", mysql_error(mysql));
					return CR_SERVER_GONE_ERROR;
				}else {
					LOG_INFO(MODULE_DB, "reconnect to mysql successfully.");
					goto again;
				}
			} else {
			    return -1;
			}
		} else {
		
            MYSQL_RES *result = mysql_store_result(mysql);
            if (result)  // there are rows
            {
                // retrieve rows, then call mysql_free_result(result)
                MYSQL_ROW row;
                unsigned int num_fields;
                unsigned int i;

                num_fields = mysql_num_fields(result);
                while ((row = mysql_fetch_row(result)))
                {
                   unsigned long *lengths;
                   lengths = mysql_fetch_lengths(result);
                   for(i = 0; i < num_fields; i++)
                   {
                       LOG_INFO(MODULE_DB, "Length[%ld] [%s] ", lengths[i],
                              row[i] ? "Binary Image(can NOT display)" : "NULL");
                       if (row[i] != NULL) {
                           string tmp((char*)row[i], lengths[i]);
                           data = tmp;
                       }
                   }
                }
                mysql_free_result(result);

            }
            else  // mysql_store_result() returned nothing; should it have?
            {
                if(mysql_field_count(mysql) != 0) {
                    LOG_ERROR(MODULE_DB, "Error: %s\n", mysql_error(mysql));
                }
            }
		}
	}

	return 0;
}

int creat_users_info_table(MYSQL *mysql) {
	char sql[1024] = { 0 };
	snprintf(sql, sizeof(sql)-1, "CREATE TABLE if NOT exists users_info (\
		account varchar(50) NOT NULL,\
		password varchar(50) NOT NULL,\
		email varchar(50) NOT NULL,\
		score int,\
		head_photo mediumblob,\
		user_name varchar(50),\
		phone varchar(20),\
		PRIMARY KEY (account)\
		)ENGINE=InnoDB;");


	if(mysql_query(mysql, sql)) {
        LOG_ERROR(MODULE_DB,"create table users_info failed[%s].", mysql_error(mysql));
		return -1;
    }

	return 0;
}

int creat_ad_pictures_table(MYSQL *mysql) {
	char sql[1024] = { 0 };
	snprintf(sql, sizeof(sql)-1, "CREATE TABLE if NOT exists ad_pictures (\
		id int NOT NULL,\
		existed tinyint(1) NOT NULL,\
		image_name varchar(100),\
		image_type varchar(50),\
		image_hashcode varchar(33),\
		image_size int,\
		link_url varchar(125),\
		locate_path varchar(125),\
		PRIMARY KEY (id)\
		)ENGINE=InnoDB;");


	if(mysql_query(mysql, sql)) {
        LOG_ERROR(MODULE_DB,"create table ad_pictures failed[%s].", mysql_error(mysql));
		return -1;
    }

	return 0;
}

int mysql_db_query_ad_info(MYSQL *connection, const char * sql, AdPicturesInfo &ad_info) {
	MYSQL_ROW row;
	MYSQL_RES *result = NULL;
	int ret = 0;
	
    if(mysql_query(connection, sql)) {
        LOG_ERROR(MODULE_DB,"mysql_query error[%s]", mysql_error(connection));
		return -1;
    } else {
        result = mysql_use_result(connection);
		if (result == NULL) {
			LOG_INFO(MODULE_DB, "mysql_use_result false[%s].", sql);
			return -1;
		}
		unsigned int num_fileds = mysql_num_fields(result);
  
		int n = 0;
		while((row = mysql_fetch_row(result))) {
			unsigned int j;
			   
			for(j=0; j < num_fileds; ++j) {
				LOG_DEBUG(MODULE_DB, "%s", row[j]);
			}
            ad_info.image_id = atoi(row[0]);
            ad_info.existed = atoi(row[1])? true : false;
            ad_info.image_name = row[2] ? row[2] : "";
            ad_info.image_type = row[3] ? row[3] : "";
            ad_info.image_hashcode = row[4] ? row[4] : "";
            ad_info.image_size = row[5] ? atoi(row[5]) : 0;
            ad_info.link_url = row[6] ? row[6] : "";
            ad_info.locate_path = row[7] ? row[7] : "";
			n++; 
		} 
		ret = n;
		mysql_free_result(result);
    }  
	
    return ret;
}

