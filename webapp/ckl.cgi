#!/usr/bin/python
# Licensed to Cloudkick, Inc under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# libcloud.org licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#### BEGIN USER MODIFICATIONS

# Secret key used to authenticate clients for write permissions
SECRET_KEY = "my-secret"

# Path to SQlite Database for this instance.
DATABASE_PATH = "/var/db/ckl/ckl.db"

# Weither to use FastCGI or normal CGI
MODE='cgi'

#### END USER MODIFICATIONS

import sys
import cgi
import flup
import sqlite3
import traceback
import time

def get_conn():
  _SQL_CREATE = ["""
    CREATE TABLE IF NOT EXISTS
      events (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp NUMERIC NOT NULL,
        hostname VARCHAR(256) NOT NULL,
        remote_ip VARCHAR(256) NOT NULL,
        username VARCHAR(256) NOT NULL,
        message TEXT NOT NULL,
        script TEXT);
    """,
    """
    CREATE INDEX IF NOT EXISTS
      ix_events_hostname ON events (hostname);
    """]
  conn = sqlite3.connect(DATABASE_PATH)
  for q in _SQL_CREATE:
    conn.execute(q);
  conn.commit();
  return conn

def process_post(environ, start_response):
  form = cgi.FieldStorage(fp=environ['wsgi.input'],
                          environ=environ)

  secret = form.getfirst("secret", "")
  if secret != SECRET_KEY:
    start_response("403 Forbidden", [("content-type","text/plain")])
    return ["Invalid Secret"]

  ts = form.getfirst("ts", 0)
  hostname = form.getfirst("hostname", "")
  remote_ip = environ['REMOTE_ADDR']
  username = form.getfirst("username", "")
  msg =  form.getfirst("msg", "")
  script = form.getfirst("scriptlog")
  c = get_conn()
  c.execute("""
      INSERT INTO events VALUES (NULL, ?, ?, ?, ?, ?, ?)
                  """,
                  [ts, hostname, remote_ip, username, msg, script])
  c.commit()
  start_response("200 OK", [("content-type","text/plain")])
  return ["saved\n"]

def mainapp(environ, start_response):
  meth = environ['REQUEST_METHOD']
  if meth == "POST":
    return process_post(environ, start_response)
  c = get_conn().cursor()
  c.execute("SELECT timestamp,hostname,username,message,script FROM events ORDER BY id DESC LIMIT 500")
  start_response("200 OK", [("content-type","text/plain")])
  output = ["server changelog: \n"]
  for row in c:
    (timestamp,hostname,username,message,script) = row
    t = time.gmtime(timestamp)
    output.append("%s by %s on %s\n  %s\n" % (time.strftime("%Y-%m-%d %H:%M:%S UTC", t), username, hostname, message))
    if script != None and len(script) > 1:
      output.append("Logged session: \n%s\n" % script)
  return output

def main(environ, start_response):
  try:
    return mainapp(environ, start_response)
  except:
    status = "500 Oops"
    response_headers = [("content-type","text/plain")]
    start_response(status, response_headers, sys.exc_info())
    return ["Problem running ckl.cgi\n\n"+ traceback.format_exc()]

if __name__ == "__main__":
  if MODE == 'fcgi':
    from flup.server.fcgi import WSGIServer
  else:
    from flup.server.cgi import WSGIServer

  WSGIServer(main).run()
