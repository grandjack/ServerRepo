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
            user_info.password = row[1];
            user_info.email = row[2];
            user_info.score = atoi(row[3]);
            user_info.head_photo = row[4];
			n++; 
		} 
		ret = n;
		mysql_free_result(result);
    }  
	
    return ret;
}
/*
int mysql_db_excute(MYSQL *mysql, const char * query_string) {

	MYSQL_RES *result;
	int num_fields, num_rows;
	int ret = 0;
	
	if (mysql_query(mysql,query_string)) {
	    // error
	} else {// query succeeded, process any data returned by it
	    result = mysql_store_result(mysql);
	    if (result) {// there are rows
	        num_fields = mysql_num_fields(result);
	        // retrieve rows, then call mysql_free_result(result)
			LOG_DEBUG(MODULE_DB, "mysql_num_fields[%d]", num_fields);
			ret = num_fields;
	        mysql_free_result(result);
	    } else {// mysql_store_result() returned nothing; should it have?
	        if(mysql_field_count(mysql) == 0) {
	            // query does not return data
	            // (it was not a SELECT)
	            num_rows = mysql_affected_rows(mysql);
				ret = num_rows;
	        } else {// mysql_store_result() should have returned data
				LOG_ERROR(MODULE_DB, "Error: %s", mysql_error(mysql));
				ret = -1;
	        }
	    }
	}

	return ret;
}
*/
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

int creat_users_info_table(MYSQL *mysql) {
	char sql[1024] = { 0 };
	snprintf(sql, sizeof(sql)-1, "CREATE TABLE if NOT exists users_info (\
		account varchar(50) NOT NULL,\
		password varchar(50) NOT NULL,\
		email varchar(50) NOT NULL,\
		score int,\
		head_photo varchar(255),\
		PRIMARY KEY (account)\
		)ENGINE=InnoDB;");


	if(mysql_query(mysql, sql)) {
        LOG_ERROR(MODULE_DB,"create table users_info failed[%s].", mysql_error(mysql));
		return -1;
    }

	return 0;
}

