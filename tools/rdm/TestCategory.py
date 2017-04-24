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
# TestCategory.py
# Copyright (C) 2011 Simon Newton

__author__ = 'nomis52@gmail.com (Simon Newton)'


class TestCategory(object):
  """The category a test is part of."""
  SYMBOLS_TO_VALUES = {
    # These correspond to categories in the E1.20 document
    'NETWORK_MANAGEMENT': 'Network Management',
    'STATUS_COLLECTION': 'Status Collection',
    'RDM_INFORMATION': 'RDM Information',
    'PRODUCT_INFORMATION': 'Product Information',
    'DMX_SETUP': 'DMX512 Setup',
    'DIMMER_SETTINGS': 'Dimmer Settings',
    'IP_DNS_CONFIGURATION': 'IP and DNS Configuration',
    'SENSORS': 'Sensors',
    'POWER_LAMP_SETTINGS': 'Power / Lamp Settings',
    'DISPLAY_SETTINGS': 'Display Settings',
    'CONFIGURATION': 'Configuration',
    'CONTROL': 'Control',
    # And others for things that don't quite fit
    'CORE': 'Core Functionality',
    'ERROR_CONDITIONS': 'Error Conditions',
    'SUB_DEVICES': 'Sub Devices',
    'UNCLASSIFIED': 'Unclassified',
  }

  CATEGORIES_ = []

  def __init__(self, category):
    self._category = category

  def __str__(self):
    return self._category

  def __hash__(self):
    return hash(self._category)

  @staticmethod
  def Categories():
    """Return a list of all TestCategories."""
    return TestCategory.CATEGORIES_


# Make the symbols accessible, i.e. TestCategory.STATUS_COLLECTION
for symbol, description in TestCategory.SYMBOLS_TO_VALUES.items():
  obj = TestCategory(description)
  setattr(TestCategory, symbol, obj)
  TestCategory.CATEGORIES_.append(obj)
