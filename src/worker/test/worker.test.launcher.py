#!/usr/bin/env python3

#@	{
#@		"target":{"name":"dummy"},
#@		"dependencies": [{"ref":"./worker.test", "origin":"generated"}]
#@	}

import sys
import os
import subprocess

target = sys.argv[1]
target_dir = os.path.dirname(target)
exit(subprocess.run(target_dir + '/worker.test').returncode)

