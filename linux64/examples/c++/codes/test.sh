#!/bin/bash

#echo "----- make main executable file!!! -----"
make

echo "----- main executable file is made!!! run the file -----"
for qos in 0.03 #0.06 0.09 0.12 0.15 0.18
do
	case "$qos" in
		0.03)
			approximation_slope="0.086151"
			approximation_constant="0.037927"
			expansion_point="1.88"
			;;
		0.06)
			approximation_slope="0.142262"
			approximation_constant="0.074887"
			expansion_point="1.554"
			;;
		0.09)
			approximation_slope="0.180429"
			approximation_constant="0.109475"
			expansion_point="1.34"
			;;
		0.12)
			approximation_slope="0.205246"
			approximation_constant="0.141578"
			expansion_point="1.174"
			;;
		0.15)
			approximation_slope="0.219324"
			approximation_constant="0.170959"
			expansion_point="1.036"
			;;
		0.18)
			approximation_slope="0.224711"
			approximation_constant="0.197897"
			expansion_point="0.915"
			;;
	esac
	
	for n in 1
	do
		echo "./main $n $qos $approximation_slope $approximation_constant $expansion_point"
		./main $n $qos $approximation_slope $approximation_constant $expansion_point 1 > result_"$qos"_"$n"th_time > output.txt
	done
done

echo "----- move result files to data directory -----"
mv result_* data/results

echo "----- make clean executable file!!! -----"
make clean
