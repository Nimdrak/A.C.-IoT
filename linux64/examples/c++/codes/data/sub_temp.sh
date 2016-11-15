#!/bin/bash

echo "----- our model!!! -----"
awk '{arr[$2] += $6; if (max < $2) {max = $2}} END {for (i = 0; i <= max; i++) {print arr[i]/250}}'  < sub_provisioning_result > 1.txt

echo "----- comparison model!!! -----"
awk '{arr[$2] += $6; if (max < $2) {max = $2}} END {for (i = 0; i <= max; i++) {print arr[i]/250}}'  < c_sub_provisioning_result > 2.txt

echo "----- remove sub_result files!!! -----"
rm sub_provisioning_result 
rm c_sub_provisioning_result
