#!/bin/bash

echo "========== Shell Script Start =========="
echo "----- make main executable file!!! -----"
make

echo "----- main executable file is made!!! run the file -----"

for n_req in 10, 20, 30, 40, 50, 60, 70, 80, 90, 100
do
	for mean in 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
	do
		for stddev in 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
		do
			echo ./main n_reqeust=${n_req} mean=${mean} stddev=${stddev}
			./main n_request=${n_req} mean=${mean} stddev=${stddev} > result_nreq_"$n_req"_mean_"$mean"_stddev_"$stddev"_nreq_"$n_req".txt
		done
	done
done

echo "----- simulation complete!!! parsing -----"





echo "----- parsing complete!!! erase object file -----"
make clean
