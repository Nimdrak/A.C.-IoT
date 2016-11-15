BEGIN {min_value1 min_value2 min_value3 min_value4 min_value5 min_value6}

{if ($3 < 0.99 && $5 == 0.01)
	min_value1 = $3
	else if ($3 < 0.98 && $5 == 0.02)
		min_value2 = $3
	else if ($3 < 0.97 && $5 == 0.03)
		min_value3 = $3
	else if ($3 < 0.96 && $5 == 0.04)
		min_value4 = $3
	else if ($3 < 0.95 && $5 == 0.05)
		min_value5 = $3
	else if ($3 < 0.94 && $5 == 0.06)
		min_value6 = $3
}
END {print min_value1;print min_value2;print min_value3;print min_value4;print min_value5;print min_value6;} 
