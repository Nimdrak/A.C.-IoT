#!/bin/bash

# fixed seed for variable control
seed=99999


for n in 1 10 20 50 100 # responding interval
do
	for tb in 20000 60000 120000 300000 # load measuring interval
	do
		echo "./main.py -m ua_br -b $tb -n $n -s $seed"
		./main.py -m ua_br -b $tb -n $n -s $seed > results/convergence-tb-n/out-uabr-$tb-$n &
	done
done
echo "./main.py -m ua_tebr -s $seed"
./main.py -m ua_tebr -s $seed > results/convergence-tb-n/out-uatebr &

echo "waiting.."
wait
echo "simulation complete! parsing.."

for n in 1 10 20 50 100
do
	for tb in 20000 60000 120000 300000
	do
		grep ratio results/convergence-tb-n/out-uabr-$tb-$n | awk '{print$1" "$7}' > results/convergence-tb-n/plot-uabr-$tb-$n
	done
done
grep ratio results/convergence-tb-n/out-uatebr | awk '{print$1" "$7}' > results/convergence-tb-n/plot-uatebr

echo "parsing complete!"

cd results/convergence-tb-n/
echo "plotting"
./plotter

echo "complete!"

exit




# eta simulation, gamma=0.2
for eta in 0.1 0.01 0.001 0.0001 0.00001 0.000001
do
	for method in ua_br ua_tebr
	do
		echo "./main.py -e $eta -g 0.2 -s 9999 -m $method -t uk"
#		./main.py -e $eta -s 9999 -m $method -t uk > results/eta/out-$eta-$method &
	done
done

# gamma simulation, eta=0.1
for gamma in 1.6 1.7
do
	for method in ua_br ua_tebr
	do
		echo "./main.py -e 0.1 -g $gamma -s 9999 -m $method -t uk"
		./main.py -e 0.1 -g $gamma -s 9999 -m $method -t uk > results/gamma/out-$gamma-$method &
	done
done

echo "waiting all processes to finish.."
wait

