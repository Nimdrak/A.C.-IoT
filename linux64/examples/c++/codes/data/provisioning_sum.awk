BEGIN {k1provisionedbw k1utilization
k2provisionedbw k2utilization
k5provisionedbw k5utilization
k10provisionedbw k10utilization
k20provisionedbw k20utilization
k50provisionedbw k50utilization}
{if ($2 == 1) {
	k1provisionedbw += $4; k1utilization += $6;}
else if ($2 == 2) {
	k2provisionedbw += $4; k2utilization += $6;}
else if ($2 == 5) {
	k5provisionedbw += $4; k5utilization += $6;}
else if ($2 == 10) {
	k10provisionedbw += $4; k10utilization += $6;}
else if ($2 == 20) {
	k20provisionedbw += $4; k20utilization += $6;}
else if ($2 == 50) {
	k50provisionedbw += $4; k50utilization += $6;}
}
END {print k1provisionedbw/50;
	print k2provisionedbw/50;
	print k5provisionedbw/50;
	print k10provisionedbw/50;
	print k20provisionedbw/50;
	print k50provisionedbw/50;
}
