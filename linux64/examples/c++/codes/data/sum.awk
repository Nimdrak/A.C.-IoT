BEGIN {s003accepted s003avgqos s003violation
	s006accepted s006avgqos s006violation
	s009accepted s009avgqos s009violation
	s012accepted s012avgqos s012violation
	s015accepted s015avgqos s015violation
	s018accepted s018avgqos s018violation}
{if ($5 == 0.03) {
	s003accepted += $1; s003avgqos += $3; s003violation += $4;} 
else if ($5 == 0.06) {
	s006accepted += $1; s006avgqos += $3; s006violation += $4;} 
else if ($5 == 0.09) {
	s009accepted += $1; s009avgqos += $3; s009violation += $4;} 
else if ($5 == 0.12) {
	s012accepted += $1; s012avgqos += $3; s012violation += $4;} 
else if ($5 == 0.15) {
	s015accepted += $1; s015avgqos += $3; s015violation += $4;} 
else if ($5 == 0.18) {
	s018accepted += $1; s018avgqos += $3; s018violation += $4;} 
}
END {print s003accepted, s003avgqos, s003violation;
print s006accepted, s006avgqos, s006violation;
print s009accepted, s009avgqos, s009violation;
print s012accepted, s012avgqos, s012violation;
print s015accepted, s015avgqos, s015violation;
print s018accepted, s018avgqos, s018violation;}
