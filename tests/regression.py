#!/usr/bin/python

from lla import *
import select
import sys
import time
from operator import add, mul


class Observer(LlaClientObserver):
  """
   The observer class, we just pass the calls through to the tester object
  """
  def __init__(self, test):
    """
     Parmeters:
      test: the Tester object
    """
    LlaClientObserver.__init__(self)
    self.t = test

  def plugins(self, plugins):
    return self.t.plugins(plugins)

  def devices(self, devs):
    return self.t.devices(devs)

  def ports(self, dev):
    return self.t.ports(dev)

  def plugin_desc(self, args):
    return 0



class Tester:
  def __init__(self, con):
    self.con = con
    self.sd = -1
    self.terminate = False

  def setup(self):
    self.con.start()

    # get the file descriptor used
    self.sd = self.con.fd()

  def term(self):
    self.terminate = True

  def stop(self):
    self.con.stop()

  def run(self, timeout=0):
    self.terminate= False
    input = [self.sd, sys.stdin]

    while(not self.terminate):
      inputready,outputready,exceptready = select.select(input,[],[])

      for s in inputready:
        if s == self.sd:
          self.con.fd_action(0)
        elif s == sys.stdin:
          l = sys.stdin.readline()
          print l

  def plugins(self, plugins):
    return 0

  def devices(self, devs):
    return 0

  def ports(self, dev):
    return 0

  def plugin_desc(self, args):
    return 0



class PatchTester(Tester):
  def __init__(self, con):
    Tester.__init__(self, con)
    self.uni = 0
    self.devs = []
    self.pending_devs = []

  def test_patching(self):
    """
    This is the main test function
    """

    self.con.fetch_dev_info(LLA_PLUGIN_ALL)
    self.run()

    # patch each port start from 0
    print "Patching ports from 0"
    self.patch_ports(0)
    time.sleep(1)

    # repatch ports again
    print "Patching ports from 0 again"
    self.patch_ports(0)
    time.sleep(1)

    # patch ports to a different universe
    prt_count = reduce(add, [ d.port_count() for d in self.devs])
    print "Patching ports from %i" % prt_count
    self.patch_ports(prt_count)
    time.sleep(1)

    print "Unpatching ports"
    self.unpatch_ports()

  def patch_ports(self, offset):
    """
     Iterate over the devices and patch each port incrementally starting from
     offset
    Parameters:
     offset: the inital universe offset
   """
    uni = offset
    for dev in self.devs:
     for port in dev.get_ports():
      self.con.patch(dev.get_id(), port.get_id(), LlaClient.PATCH, offset)
      offset+=1

  def unpatch_ports(self):
    """
     Iterate over the devices and unpatch each port
   """
    for dev in self.devs:
     for port in dev.get_ports():
      self.con.patch(dev.get_id(), port.get_id(), LlaClient.UNPATCH, 0)

  def devices(self, devices):
    self.devs = devices
    self.pending_devs = [ d.get_id() for d in devices]
    for dev in devices:
      self.con.fetch_port_info(dev)
    return 0

  def ports(self, dev):
    self.pending_devs.remove(dev.get_id())

    if len(self.pending_devs) == 0:
      self.term()
    return 0


class PluginTester(Tester):

  def plugins(self, plugins):
   """
    Called when new plugin information arrives
    Parameters:
     plugins: a list of LlaPlugin objects
   """
   for plug in plugins:
     print plug.get_name()
   return 0


def main():
  """
    Create a new connection and run the test cases
  """
  # create a new LlaClient
  con = LlaClient()
  t = PatchTester(con)

  # create an observer object and register
  ob = Observer(t)
  con.set_observer(ob)

  t.setup()
  t.test_patching()
  t.stop()


if __name__ == '__main__':
  main()

