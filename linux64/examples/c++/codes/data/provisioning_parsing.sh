#!/bin/bash

echo "----- remove previous average files!!! -----"
rm average_provisioned_utilization
rm c_average_provisioned_utilization

echo "----- get the average provisioned bw and utilization!!! -----"
awk -f provisioning_sum.awk provisioning_result > average_provisioned_utilization
awk -f provisioning_sum.awk c_provisioning_result > c_average_provisioned_utilization

echo "----- show our result!!! -----"
cat average_provisioned_utilization

echo "----- show comparison result!!! -----"
cat c_average_provisioned_utilization

echo "----- remove result files!!! -----"
rm provisioning_result 
rm c_provisioning_result 
