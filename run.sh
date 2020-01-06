#!/bin/bash

rm test
make test
echo "Executing ./test..."
(time ./test 2>err.log 1>>output.log) 2>> time.log & disown