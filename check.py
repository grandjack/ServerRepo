#!/usr/bin/python
# -*- coding: UTF-8 -*-

# Import modules for CGI handling 
import cgi, cgitb 

account=""
token=""
filename="/home/gbx386/libevent_test/"
result=""

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
if form.getvalue('account'):
    account = form.getvalue('account')
    filename += account

if form.getvalue('token'):
    token = form.getvalue('token')


try:    
    file_object = open(filename, "r")
    str = file_object.read(32)
    if str == token:
        result="successfully"
    else:
        result="fault"
    
    file_object.close()
except Exception, ex:
    result="fault"

    
print "Content-type:text/html\r\n\r\n"
print "<html>"
print "<head>"
print "<title>Result</title>"
print "</head>"
print "<body>"
print "<p> Got token %s for %s! </p>" %(result,account)
print "</body>"
print "</html>"