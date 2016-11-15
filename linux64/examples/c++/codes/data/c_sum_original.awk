BEGIN {cs003 cs006 cs009 cs012 cs015 cs018}
{if ($3 == 0.03)
	cs003 += $1
else if ($3 == 0.06)
	cs006 += $1
else if ($3 == 0.09)
	cs009 += $1
else if ($3 == 0.12)
	cs012 += $1
else if ($3 == 0.15)
	cs015 += $1
else if ($3 == 0.18)
	cs018 += $1
}
END {print cs003; 
	print cs006;
	print cs009;
	print cs012;
	print cs015;
	print cs018;}
