#! /usr/bin/env python
# vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 filetype=python :
top = '..'
REPOSITORY_PATH = "cuselab"
REPOSITORY_NAME = "SoCRocket CuSE Lab Repository"
REPOSITORY_DESC = """This repository contains top Level designs, models and software for the lab."""
REPOSITORY_TOOLS = [
    'av',
    'sdl'
]

def build(self):
  self.recurse_all()
  self.recurse_all_tests()
