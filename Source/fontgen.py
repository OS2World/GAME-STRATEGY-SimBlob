# This program creates a postscript file that will display the Hershey fonts
import sys
sys.stdout = open('fontgen.ps','wt')

print """
%!
% Header
/fontsz 10 def
/Hershey-Plain-Simplex-Bold-Oblique findfont fontsz scalefont setfont

1 setlinewidth

% Page
10 414 translate

"""

x = 20
y = 6.5
print 0,y," moveto 10 0 rlineto stroke"
for c in range(32,127):
    s = chr(c)
    if s in ['"',')','(','\\']: s = "\\" + s
    print x,y," moveto 0 12 rmoveto 0 10 rlineto 0 -22 rmoveto stroke"
    print x,y," moveto 0.01 rotate ("+s+") show -0.01 rotate"
    print "0 12 rmoveto 0 10 rlineto 0 -22 rmoveto stroke"
    x = x+20
    if c % 16 == 15:
	x,y = 20, y+37
	print 0,y," moveto 10 0 rlineto stroke"

print "showpage"
sys.stdout.close()
