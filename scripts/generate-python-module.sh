#!/bin/sh

#swig -c++ -outdir ./PyOdbDesignLib -python ./OdbDesignLib/OdbDesignLib.i
# "include" the .cmd file by source executing here, so we don't have to duplicate the commands
source scripts/generate-python-module.cmd