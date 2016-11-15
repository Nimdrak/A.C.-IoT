#!/bin/bash

for n_req in 10, 20, 30, 40, 50, 60, 70, 80, 90, 100
do
	echo ./main n_request=${n_req}
	./main -n_request=${n_req} > result_nreq_"$n_req".txt
done


for mean in 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
do
	for stddev in 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
	do
		echo ./main -n_reqeust=${n_req} mean=${mean} stddev=${stddev}
		./main -n_request=${n_req} -mean=${mean} -stddev=${stddev} > result_nreq_"$n_req"_mean_"$mean"_stddev_"$stddev"_nreq_"$n_req".txt
	done
done

echo "simulation complete! parsing"

for nStas in 1 2 3 4 5 6 7 8 9 10 
do
	echo ./waf nStas=${nStas} nUes=${nStas} 
	./waf --run "scratch/wifi-ap -nStas=${nStas} -nUes=1 " > Total_200s_"$nStas"_"$nStas"_LTEW.txt

	mv wifi-ap.cwnd	wifi-ap_"$nStas"_"$nStas"_LTEW.cwnd 
	mv wifi-ap.rtt wifi-ap_"$nStas"_"$nStas"_LTEW.rtt
	mv wifi-ap.rto wifi-ap_"$nStas"_"$nStas"_LTEW.rto
	mv wifi-ap.timeout wifi-ap_"$nStas"_"$nStas"_LTEW.timeout
	mv wifi-ap.frc wifi-ap_"$nStas"_"$nStas"_LTEW.frc

done

echo "simulation complete! parsing"

for i in 1 2 3 5 6 7 8 
do
	for nStas in 1 2 3 4 5 6 7 8 9 10 
	do
		awk -f output"$i"-wifi.awk Total_200s_"$nStas"_"$nStas"_LTEW.txt > graph_"$i"_200s_"$nStas"_"$nStas"_LTEW_WIFI.txt
		awk -f output"$i"-lte.awk Total_200s_"$nStas"_"$nStas"_LTEW.txt > graph_"$i"_200s_"$nStas"_"$nStas"_LTEW_LTE.txt
	done
done


for nStas in 1 2 3 4 5 6 7 8 9 10 
do
	awk -f output4.awk Total_200s_"$nStas"_"$nStas"_LTEW.txt > graph_4_200s_"$nStas"_"$nStas"_LTEW_WIFI.txt
done

echo "parsing complete!"



