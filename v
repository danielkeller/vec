#! /bin/bash

(cat vintrin.h; cpp $1) | ./vec $2
