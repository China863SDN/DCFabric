
#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import os
import json
import httplib
import socket
import struct

ctl_ip = sys.argv[1]
ctl_port = sys.argv[2]
url = "/gn/cluster/query/json"
try:
    conn = httplib.HTTPConnection(ctl_ip, ctl_port)
    conn.request(method="GET", url=url)
    response = conn.getresponse()
    res = response.read()
    #print res
    conn.close()
except Exception, e:
    os._exit(1)


s = json.loads(res)
ctl_ip_convert=struct.unpack("I",socket.inet_aton(str(ctl_ip)))[0]
#print ctl_ip_convert
for ctl in s["clusterInfo"]:
    if ((ctl["role"] == "Master") or (ctl["role"] == "Equal")) and (ctl["status"] == "up"):
        if int(ctl["ctlIP"]) == ctl_ip_convert:
            print ctl_ip + " is master." 
            os._exit(0)

print ctl_ip +" is not master."
os._exit(1)
