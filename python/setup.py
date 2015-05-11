#!/usr/bin/env python

from setuptools import setup


setup(
    name='ola',
    version='0.9.5',
    author='Simon Newton',
    author_email='nomis52@gmail.com',
    description='OLA Python client bindings.',
    license='GPL',
    url='https://www.openlighting.org',
    packages=['ola', 'ola.rpc'],
    zip_safe=False,
    install_requires=['protobuf>=2.6.1'],
)
