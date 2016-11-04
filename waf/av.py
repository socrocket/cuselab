#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim: set expandtab:ts=4:sw=4:setfiletype python
import os

def options(self):
    self.add_option(
        "--with-av", 
        type='string', 
        dest="avdir", 
        help="Basedir of your AV installation", 
        default=os.environ.get("AV_HOME")
    )

def find(self, path = None):
    avlib = os.path.abspath(os.path.expanduser(os.path.expandvars(os.path.join(path, "lib"))))
    avdir = os.path.abspath(os.path.expanduser(os.path.expandvars(os.path.join(path, "include"))))

    self.check_cxx(
      lib          = 'avformat',
      uselib_store = 'AV',
      mandatory    = True,
      libpath      = avlib,
      errmsg       = "LIBAV_FORMAT Library not found.",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      header_name  = 'libavformat/avformat.h',
      uselib_store = 'AV',
      mandatory    = True,
      includes     = avdir,
      defines      = "__STDC_CONSTANT_MACROS",
      uselib       = 'AV',
      okmsg        = "ok"
    ) 
    self.check_cxx(
      lib          = 'bz2',
      uselib_store = 'AV',
      mandatory    = True,
      errmsg       = "libbz2 not found",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      lib          = 'avcodec',
      uselib_store = 'AV',
      #linkflags    = '-lz', this gets placed in the wrong position
      mandatory    = True,
      libpath      = avlib,
      errmsg       = "LIBAV_CODEC Library not found.",
      okmsg        = "ok"
    )
    self.check_cxx(
      lib          = 'z',
      uselib_store = 'AV',
      mandatory    = True,
      errmsg       = "zlib not found",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      header_name  = 'libavcodec/avcodec.h',
      uselib_store = 'AV',
      mandatory    = True,
      includes     = avdir,
      defines      = "__STDC_CONSTANT_MACROS",
      uselib       = 'AV',
      okmsg        = "ok"
    ) 
    


    self.check_cxx(
      lib          = 'avdevice',
      uselib_store = 'AV',
      mandatory    = True,
      libpath      = avlib,
      errmsg       = "LIBAV_DEVICE Library not found.",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      lib          = 'asound',
      uselib_store = 'AV',
      mandatory    = True,
      errmsg       = "libasound not found",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      header_name  = 'libavdevice/avdevice.h',
      uselib_store = 'AV',
      mandatory    = True,
      includes     = avdir,
      defines      = "__STDC_CONSTANT_MACROS",
      uselib       = 'AV',
      okmsg        = "ok"
    ) 

    self.check_cxx(
      lib          = 'avfilter',
      uselib_store = 'AV',
      mandatory    = True,
      libpath      = avlib,
      errmsg       = "LIBAV_FILTER Library not found.",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      header_name  = 'libavfilter/avfilter.h',
      uselib_store = 'AV',
      mandatory    = True,
      includes     = avdir,
      defines      = "__STDC_CONSTANT_MACROS",
      uselib       = 'AV',
      okmsg        = "ok"
    ) 
    
    self.check_cxx(
      lib          = 'avutil',
      uselib_store = 'AV',
      mandatory    = True,
      libpath      = avlib,
      errmsg       = "LIBAV_UTIL Library not found.",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      header_name  = 'libavutil/avutil.h',
      uselib_store = 'AV',
      mandatory    = True,
      includes     = avdir,
      defines      = "__STDC_CONSTANT_MACROS",
      uselib       = 'AV',
      okmsg        = "ok"
    )

    self.check_cxx(
      lib          = 'swscale',
      uselib_store = 'AV',
      mandatory    = True,
      libpath      = avlib,
      errmsg       = "LIBSWSCALE Library not found.",
      okmsg        = "ok"
    ) 
    self.check_cxx(
      header_name  = 'libswscale/swscale.h',
      uselib_store = 'AV',
      mandatory    = True,
      includes     = avdir,
      defines      = "__STDC_CONSTANT_MACROS",
      uselib       = 'AV',
      okmsg        = "ok"
    ) 
    
def configure(self):
    try:
        if self.options.avdir:
            find(self, self.options.avdir)
        else:
            find(self)
    except:
        name    = "libav"
        version = "0.8.6"
        self.dep_build(
            name    = name, 
            version = version,
            tar_url = "https://libav.org/releases/%(base)s.tar.gz",
        )
        find(self, self.dep_path(name, version))
        #print """
        #  Dependency libav ist missing, please see media/README file!
        #"""

