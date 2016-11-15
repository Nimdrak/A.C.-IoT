#!/bin/bash

echo "----- remove previous results!!! -----"
rm *result

echo "----- add all accepted requests!!! -----"
awk -f sum.awk accepted_requests > accepted_result
awk -f c_sum_original.awk c_accepted_requests > c_accepted_result

echo "----- cat result!!! -----"
cat accepted_result
cat c_accepted_result

echo "----- remove insignificant text file!!! -----"
rm *_requests
