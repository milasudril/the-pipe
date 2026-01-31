#!/usr/bin/env python3

#@	{
#@		"target":{"name":"dummy"},
#@		"dependencies": [{"ref":"./client.test", "origin":"generated"}]
#@	}

import sys
import os
import subprocess

target = sys.argv[1]
target_dir = os.path.dirname(target)
exit(subprocess.run(target_dir + '/client.test').returncode)

