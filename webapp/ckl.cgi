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

import flup
import sqlite3
import traceback


def mainapp(environ, start_response):
  start_response("200 OK", [("content-type","text/plain")])
  return ["hi"]

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
