#!/bin/bash

HOSTNAME="localhost"                                         #数据库HOST
PORT="3306"
USERNAME="gbx386"
PASSWORD="760226"
DBNAME="ThreePeoplePlayChessDB"                              #要创建的数据库的库名称

PID_FILE="/tmp/data_collected.pid"
LOG_FILE="/tmp/data_collected.log"

if [ -n "$1" ];then
	HOSTNAME=$1
fi

MYSQL_CMD="mysql -h${HOSTNAME}  -P${PORT}  -u${USERNAME} -p${PASSWORD}"
echo ${MYSQL_CMD} 

create_db_sql="create database IF NOT EXISTS ${DBNAME}"

echo ${create_db_sql}  | ${MYSQL_CMD}                         #创建数据库
if [ $? -ne 0 ]                                               #判断是否创建成功
then
	echo "create databases ${DBNAME} failed ..."
	exit 1
fi

echo "start Server daemon...."
pid=`cat ${PID_FILE}`
kill -9 ${pid} >/dev/null 2>&1
data_collected --daemon-enable --threads=4 --pid-file=${PID_FILE} --log-file=${LOG_FILE} --db-host=${HOSTNAME} --db-name=${DBNAME} --debug-level=2
echo "Server daemon started"

exit 0
