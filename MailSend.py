#!/usr/bin/python
#coding=utf-8
import sys
import smtplib  
from email.mime.text import MIMEText
mailto_list=["517564967@qq.com"]
mail_host="smtp.163.com"
mail_user="grandjack760@163.com"
mail_pass="760226"
token=""
content=""
  
def send_mail(to_list,sub,content):
    me=mail_user    #"hello"+"<"+mail_user+">"
    msg = MIMEText(content,_subtype='html',_charset='UTF-8')
    msg['Subject'] = sub
    msg['From'] = me  
    msg['To'] = ";".join(to_list)  
    try:  
        s = smtplib.SMTP(mail_host,25, "localhost", 10)
        s.login(mail_user,mail_pass)
        s.sendmail(me, to_list, msg.as_string())
        s.close()  
        return True  
    except Exception, e:  
        #print str(e)  
        return False  
if __name__ == '__main__':
    if len(sys.argv) != 3:
        print 'Usage: Account Token'
        exit(1)
    else:
        mailto_list.append(sys.argv[1]);
        token=sys.argv[2]
        content="<a href='http://123.57.180.67/cgi-bin/check.py?account=" +sys.argv[1]+ "&token="+sys.argv[2]+"'>请在5分钟之内点击修改密码</a> <p> 请记住您的口令为:" + token + "</p>"
        
    if send_mail(mailto_list,"三人中国象",content):
        exit(0)
    else:
        exit(1)
