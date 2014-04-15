#!/bin/bash
xxd -u -a -g 1 -c 16 -s Ox$1 -l 512 80m.img
