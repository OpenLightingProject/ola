#!/usr/bin/python

from lla import *
import sys

class Observer(LlaClientObserver):
  """ Handle the events """
  def new_dmx(self, uni, length, data):
    """ Called with new dmx buffers """
    return 0


def main():
  # create a new LlaClient
  con = LlaClient()

  # create an observer object and register
  ob = Observer()
  con.set_observer(ob)

  if con.start():
    sys.exit()

  con.register_uni(1, LlaClient.REGISTER)

  while True:
    con.fd_action(1)


if __name__ == '__main__':
  main()

