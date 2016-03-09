# coding: UTF-8
import os, subprocess
import time
import hashlib
import MySQLdb

import signal
import threading

def sig_handler(sig, frame):
    try:
        print "Catch the signal %d" %sig
    except Exception, ex:
        print ex
        exit(0)

    
    
try:
    conn=MySQLdb.connect(host='123.57.180.67',user='gbx386',passwd='760226',db='ThreePeoplePlayChessDB')
    cur=conn.cursor()

    cur.execute('create table test(id int,info varchar(20))')
     
    value=[1,'hi rollen']
    cur.execute('insert into test values(%s,%s)',value)
     
    values=[]
    for i in range(20):
        values.append((i,'hi rollen'+str(i)))
         
    cur.executemany('insert into test values(%s,%s)',values)
 
    cur.execute('update test set info="I am rollen" where id=3')
 
    conn.commit()
    cur.close()
    conn.close()
 
except MySQLdb.Error,e:
     print "Mysql Error %d: %s" % (e.args[0], e.args[1])