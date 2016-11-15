#!/bin/bash

echo "----- remove previous results!!! -----"
rm *result

echo "----- parsing the data!!! -----"
./our_parsing.pl accepted_requests
./control_parsing.pl c_accepted_requests

echo "----- count the accepted requests!!! -----"
wc our.count > accepted_result
wc control.count > c_accepted_result 

echo "----- remove insignificant text file!!! -----"
rm *.count
rm *_requests 
