#!/usr/bin/env python
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# setup_patch.py
# Copyright (C) 2012 Simon Newton

import logging
from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OlaClient, Plugin

"""A simple script to find all RDM enabled ports and auto-patch them to
universes.
"""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class AutoPatcher(object):
  """A class that patches RDM enabled Output Ports to universes."""
  def __init__(self, wrapper, callback, force=False):
    """Create a new AutoPatcher object.

    Args:
      wrapper: an ola.ClientWrapper object.
      callback: The function to be called when patching is complete (or an
        error occurs. The callback is passed three arguments:
          (status, ports_found, ports_patched)
      force: override the patching for ports which are already patched
    """
    self._wrapper = wrapper
    # start from 1 to avoid confusion
    self._next_universe = 1
    self._callback = callback
    self._ports_found = 0
    self._ports_patched = 0
    self._port_attempts = 0
    self._force = force

  def Patch(self):
    client = self._wrapper.Client()
    client.FetchDevices(self._DeviceList)

  def _DeviceList(self, status, devices):
    if not status.Succeeded():
      self._callback(False, self._ports_found, self._ports_patched)

    for device in sorted(devices):
      if device.plugin_id == Plugin.OLA_PLUGIN_ARTNET:
        # skip over Art-Net devices
        continue

      for port in device.output_ports:
        if port.active and not self._force:
          # don't mess with existing ports
          continue

        if port.supports_rdm:
          self.PatchPort(device, port, self._next_universe)
          self._next_universe += 1
          self._ports_found += 1

    if self._ports_found == 0:
      self._callback(True, self._ports_found, self._ports_patched)

  def PatchPort(self, device, port, universe):
    client = self._wrapper.Client()
    logging.info('Patching %d:%d:output to %d' %
                 (device.alias, port.id, universe))
    universe_name = [device.name]
    if port.description:
      universe_name.append(port.description)
    universe_name = ': '.join(universe_name)
    client.PatchPort(device.alias,
                     port.id,
                     True,
                     OlaClient.PATCH,
                     universe,
                     lambda s: self._PatchComplete(universe, universe_name, s))

  def _PatchComplete(self, universe, universe_name, status):
    """Called when the patch is complete."""
    if status.Succeeded():
      self._ports_patched += 1
      self._wrapper.Client().SetUniverseName(universe,
                                             universe_name,
                                             self._UniverseNameComplete)
    else:
      self._IncrementPortAttempts()

  def _UniverseNameComplete(self, status):
    self._IncrementPortAttempts()

  def _IncrementPortAttempts(self):
    """This increments the port attempt counter, and exits if we have no ports
       left to patch.
    """
    self._port_attempts += 1
    if self._port_attempts == self._ports_found:
      self._callback(True, self._ports_found, self._ports_patched)


class PatchResults(object):
  """This just stores the results of an auto-patch attempt."""
  def __init__(self):
    self.status = False
    self.ports_found = 0
    self.ports_patched = 0


def PatchPorts(wrapper=None):
  """Perform the patch and return the results when complete."""
  patch_results = PatchResults()
  if not wrapper:
    wrapper = ClientWrapper()

  def PatchingComplete(ok, found, patched):
    # called when the patching is complete, this stops the wrapper and updates
    # the status code
    wrapper.Stop()
    patch_results.status = ok
    patch_results.ports_found = found
    patch_results.ports_patched = patched

  patcher = AutoPatcher(wrapper, PatchingComplete, True)
  patcher.Patch()
  wrapper.Run()

  return patch_results


def main():
  logging.basicConfig(
      level=logging.INFO,
      format='%(message)s')

  patch_results = PatchPorts()

  if patch_results.status:
    print ('Patched %d of %d ports' %
           (patch_results.ports_patched, patch_results.ports_found))
  else:
    print 'Failed to patch'


if __name__ == '__main__':
    main()
