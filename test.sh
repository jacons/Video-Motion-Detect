#!/bin/bash

make clean
make

echo "RESULTS"
echo "---Sequential version---"
./main 0 1  17 0.50461 0
echo "---Thread C++ version---"
./main 1 20 17 0.50461 0
echo "---Fastflow A version---"
./main 2 20 17 0.50461 0
echo "---Fastflow B version---"
./main 3 10 17 0.50461 0
echo ""
echo "TIME EXECUTION us"
echo "---Sequential version---"
./main 0 1  17 0.50461 1
echo "---Thread C++ version---"
./main 1 20 17 0.50461 1
echo "---Fastflow A version---"
./main 2 20 17 0.50461 1
echo "---Fastflow B version---"
./main 3 20 17 0.50461 1