from array import array
import sys
import time
from ola.ClientWrapper import ClientWrapper
from ola.DMXConstants import DMX_MIN_SLOT_VALUE, DMX_MAX_SLOT_VALUE, \
    DMX_UNIVERSE_SIZE

wrapper = None


def DmxSent(status):
  if status.Succeeded():
    pass
  else:
    print('Error: %s' % status.message)

  global wrapper
  if wrapper:
    wrapper.Stop()


def main():
  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  universe = 1
  data = array('B')

  for i in range(0, 256):
    for j in range(1, 513):
      data.append(i)
      print("C: " + str(j) + " V: " + str(i))
    client.SendDmx(universe, data, DmxSent)
    wrapper.Run()
    time.sleep(.04)

  for i in range(255, -1, -1):
    for j in range(1, 513):
      data.append(i)
      print("C: " + str(j) + " V: " + str(i))
    client.SendDmx(universe, data, DmxSent)
    wrapper.Run()
    time.sleep(.04)

if __name__ == '__main__':
  main()

