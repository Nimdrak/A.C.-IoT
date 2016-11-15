#!/bin/bash

#echo "----- make main executable file!!! -----"
make

echo "----- main executable file is made!!! run the file -----"

#for qos in 0.03 0.06 0.09 0.12 #0.15
for qos in 0.18 #0.05 0.1
do
	case "$qos" in
		0.01)
			approximation_slope="0.027048099"#"0.0344876"
			approximation_constant="0.0925728"
			expansion_point="2.325"
			;;	
		0.03)
			approximation_slope="0.068143566"#"0.0861516"
			approximation_constant="0.199893"
			expansion_point="1.88"
			;;
		0.05)
			approximation_slope="0.103961095"#"0.126606"
			approximation_constant="0.270963"
			expansion_point="1.64"
			;;
		0.06)
			approximation_slope="0.11908103"#"0.142079"
			approximation_constant="0.295679"
			expansion_point="1.555"
			;;
		0.09)
			approximation_slope="0.16255505"#"0.180429"
			approximation_constant="0.35125"
			expansion_point="1.34"
			;;
		0.1)
			approximation_slope="0.175847430"#"0.190145"
			approximation_constant="0.363981"
			expansion_point="1.28"
			;;
		0.12)
			approximation_slope="0.200037379"#"0.205119"
			approximation_constant="0.382388"
			expansion_point="1.175"
			;;
		0.15)
			approximation_slope="0.233505176"#"0.219398"
			approximation_constant="0.398255"
			expansion_point="1.035"
			;;
		0.18)
			approximation_slope="0.262487705"#"0.224711"
			approximation_constant="0.403507"
			expansion_point="0.915"
			;;
	esac
	
	for k in 1
	do
		for n in 1 
		do 
			echo "./main $n $qos $approximation_slope $approximation_constant $expansion_point $k"
			./main $n $qos $approximation_slope $approximation_constant $expansion_point $k > result_"$qos"_"$k"_"$n"th_time
		done
	done
done

echo "----- move result files to data directory -----"
mv result_* data/results

echo "----- make clean executable file!!! -----"
make clean

