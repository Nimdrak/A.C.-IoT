BEGIN {cs003accepted cs003avgqos cs003violation
	cs006accepted cs006avgqos cs006violation
	cs009accepted cs009avgqos cs009violation
	cs012accepted cs012avgqos cs012violation
	cs015accepted cs015avgqos cs015violation
	cs018accepted cs018avgqos cs018violation}
{if ($5 == 0.03) {
	cs003accepted += $1; cs003avgqos += $3; cs003violation += $4;} 
else if ($5 == 0.06) {
	cs006accepted += $1; cs006avgqos += $3; cs006violation += $4;} 
else if ($5 == 0.09) {
	cs009accepted += $1; cs009avgqos += $3; cs009violation += $4;} 
else if ($5 == 0.12) {
	cs012accepted += $1; cs012avgqos += $3; cs012violation += $4;} 
else if ($5 == 0.15) {
	cs015accepted += $1; cs015avgqos += $3; cs015violation += $4;} 
else if ($5 == 0.18) {
	cs018accepted += $1; cs018avgqos += $3; cs018violation += $4;} 
}
END {print cs003accepted, cs003avgqos, cs003violation;
print cs006accepted, cs006avgqos, cs006violation;
print cs009accepted, cs009avgqos, cs009violation;
print cs012accepted, cs012avgqos, cs012violation;
print cs015accepted, cs015avgqos, cs015violation;
print cs018accepted, cs018avgqos, cs018violation;}
