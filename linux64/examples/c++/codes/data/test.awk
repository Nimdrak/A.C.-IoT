BEGIN {utilization}
{
	mean += $6;
	utilization += $7;
}
END {print (mean/100);
	print(utilization/100);}

