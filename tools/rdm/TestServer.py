#!/usr/bin/python
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# TestServer.py
# Copyright (C) 2010 Simon Newton

from wsgiref.simple_server import make_server
import json

settings = {
  'PORT': 9999,
  'headers': [('Content-type', 'application/json')],
}

status = {
  '200': '200 OK',
  '404': '404 Not Found',
}

class TestServer():
  
  def __init__(self, options):
    settings.update(options)
    self.httpd = make_server('', settings['PORT'], self.__request_handler)

  def start_serving(self):
    self.httpd.serve_forever()

  def __request_handler(self, environ, start_response):
    start_response(status['200'], settings['headers'])
    params = environ['QUERY_STRING'].split('&')
    get_params = {}
    
    for param in params:
      param = param.split('=')
      if len(param) > 1:
        get_params[str(param[0])] = str(param[1])

    return json.dumps(get_params)

  def run_tests(self):
    None

  def handle_responses(self):
    None

  def parse_options(self):
    None

def main():
  test_server = TestServer({})
  test_server.start_serving()
  
if __name__ == '__main__':
  main()
