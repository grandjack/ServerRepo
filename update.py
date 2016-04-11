#!/usr/bin/python
# -*- coding: UTF-8 -*-

import cgi, cgitb
import MySQLdb
import md5

result=""
account=""

def UpdatePassword(account, pwd):

    db = MySQLdb.connect("123.57.180.67","gbx386","760226","ThreePeoplePlayChessDB")

    cursor = db.cursor()

    sql = "UPDATE users_info SET password='%s' WHERE account='%s'" % (pwd, account)
    try:
        cursor.execute(sql)
        db.commit()
    except:
        db.rollback()
        result="UPDATE DB failed"
       
    db.close()
    

def GetStringMd5(src):
    m1 = md5.new()
    m1.update(src)
    return m1.hexdigest()

    
def HandleRequest():
    global result
    global account
    
    token=""
    pwd=""
    repwd=""
    filename="/usr/share/web/data/"
    
    try:
        # Create instance of FieldStorage
        form = cgi.FieldStorage()
        
        # Get data from fields
        if form.getvalue('account'):
            account = form.getvalue('account')
            filename += account

        if form.getvalue('token'):
            token = form.getvalue('token')

        if form.getvalue('pwd'):
            pwd = form.getvalue('pwd')

        if form.getvalue('repwd'):
            repwd = form.getvalue('repwd')

    except Exception, ex:
        result="can NOT get the field from the form"
        return


    if ((pwd != repwd) or len(pwd) <= 0):
        result="password invalid"
    else:
        try:
            file_object = open(filename, "r")
            str = file_object.read(32)
            if str == token:
                result="successfully"
                
                #Now you can update you password!
                UpdatePassword(account, GetStringMd5(pwd));
            else:
                result="token NOT equal"

            file_object.close()
        except Exception, ex:
            result="Time out or can NOT get the file"


def PrintMessage():
    print "Content-type:text/html\r\n\r\n"
    print "<html>"
    print "<head>"
    print "<title>Result</title>"
    print "</head>"
    print "<body>"
    print "<p> Update Password %s for %s! </p>" %(result,account)
    print "</body>"
    print "</html>"


if __name__ == '__main__':
    HandleRequest()
    PrintMessage()
    
