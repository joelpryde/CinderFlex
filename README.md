# CinderFlex
NVidia Flex support for Cinder.

CinderFlex
===================

####Introduction

This block/sample allows Cinder to operated with the NVidia Flex framework.  FleX is a particle based simulation technique for real-time visual effects.  In particular it is useful for utilizing CUDA in order to do particle and fluid effects similar to RealFlow, nCloth or Lagoa.  This sample just demonstrates how to setup Flex with Cinder and do some basic particle/fluid effects.

###### Setup, Options

Currently Flex is not supported on OSX so for the time being this sample is Windows only.  You will been to install Flex from [here](https://developer.nvidia.com/flex).  You will also need install the CUDA SDK from [here](https://developer.nvidia.com/cuda-downloads).  Finally you will need to set your FLEX_PATH environment variable to wherever you installed Flex and CINDER_PATH to wherever your latest version of Cinder is located (currently building against 0.9.1dev).

This may or may not work without an NVidia card (though I would sort of be astonished if it did).