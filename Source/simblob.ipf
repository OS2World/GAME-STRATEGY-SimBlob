:userdoc.
:title.BlobCity Help
:h1 res=404 name=PANEL_MAIN.Controls
:i1 id=Controls.Controls
:p.
I have attempted to make the game use custom mouse settings that you
have set in the properties page of the "Mouse Object". However, I have
not extensively tested it, and the notes here assume the default
left-handed mouse settings. Let me know if you have problems with the
game's use of the mouse.

:dl.
:dt.World Map
:dd.
A LMB click changes the view to be centered wherever you clicked. You
can also drag with the LMB to move the view, but this may be slow on
some systems.

:dt.Toolbar

:dd.LMB click allows you to choose the tool to use. Most of the tools
let you build something, and are collectively called the build tools.

:dt.Map View
:dd.

:i1.Tools
:dl.
:dt.Build Tools

:dd.LMB click or drag draws the currently selected toolbar item. If
you are drawing roads, canals, bridges, or walls, you can drag from
one point to another, and a line will be drawn between those
points. If you are erasing, or drawing trees, fires, or gates, then
dragging will draw "freehand". The item will not be drawn immediately,
but instead a black or green hexagon will be drawn at that location,
and a blob will go to that location to build whatever you wanted.

:p.Using Ctrl with the LMB will use the Eraser instead of the toolbar
selection. Using Shift with the LMB will draw straight instead of
freehand, or freehand instead of straight.

:p.RMB click does different things in different versions; it is a
convenient place for me to put test code.

:dt.Blob Tool

:dd.The blob tool lets you create and move blobs. RMB drag moves a
blob from one place to another. LMB click creates a blob and sends him
to the point at which you clicked.

:dt.Select Tool

:dd.The select tool (not yet finished) displays information about a
selected object. At present, it simply displays information about the
object underneath the mouse pointer.

:edl.

:edl.

:h1 res=256 name=PANEL_HELP.BlobCity
:i1 id=BlobCity.BlobCity
:i1.SimBlob

:p.SimBlob is the name of a project I started in 1994, to produce a
game that would let me experiment with various things I was learning
at school. In particular, I wanted to experiment with bitmap graphics,
multithreaded programming, user interfaces, and cellular
automata. SimBlob started out as a strategy game for OS/2 with
simulation of the environment and economy affecting your quest to
conquer neighboring towns. The setting is an early (green) Mars, with
blobs being the most intelligent life form on the planet. The original
plan turned out to be too ambitious for me, given that I spend much of
my time on projects and I will soon be starting on my Ph.D. thesis.

:p.The current plan is for the SimBlob project to include two (or
more) simpler games instead of one very complex one. The games will
share the basic map structure (a hexagonal grid map), environmental
simulation (water flow, erosion, fires, tree growth, etc.), and user
interface. The first game, BlobCity, is a single player game where you
are playing against the environment. In it you try to build a town in
a world that has fires, floods, volcanos, and so on. The SimBlob
project also includes a second game, which will have less city
building and more resource management. This game has not yet been
designed. I have written several possibilities for games that could
fit into the SimBlob project; they are listed on the web page. The
blobs might live in a large city; they might be building a town to
compete with other towns; they might have towns already, and want you
to link them together with trade; or they might be seeking to take
advantage of the natural resources on Mars. I am starting to write up
a design document for The Silver Kingdoms, which is my current
favorite choice for a second game; I will post that document here
soon. Comments and suggestions are appreciated.

:p.For the latest information about BlobCity, visit the web page
at http&colon.//www-cs-students.stanford.edu/~amitp/games.html

:h1.Playing
:i1.Playing

:p.At present, the game is playable, although it's not complete. (The
main problem is lack of documentation and the inability to save and
load games, IMO.) The play in BlobCity is "Blobs vs.  Nature" (single
player) instead of "Blobs vs. Blobs" (human player vs. computer
players), which is planned for the second SimBlob game. Like SimCity,
there is no predetermined goal; instead you set your own goals. You
are building a town and seek to conquer the elements. Try to build
your towns to withstand floods, yet be near sources of water. Try to
build your towns to withstand fires, yet be near forests. Try to
achieve 600,000 population without auto-build mode. Try to have more
food than people in your town.

:p.Other things to try: 
:ol.

:i1.Irrigation
:li.If you want to create irrigation channels, start at a water source
on a mountain. Build a walled channel that slopes gently
downwards. Along this channel, build gates. Outside the channel,
starting from each gate, build a trench. This trench will receive some
water from the main channel, but due to the gate, will not take all of
the water. Note: This works better if you can put several water
sources inside the walled channel.

:i1.Watchtowers
:li.To protect against fires, put fire watchtowers in spaces that the
blobs aren't going to build in anyway. For example, a common road
pattern is a hexagon with seven spaces inside; with this, only six
will be used by the blobs, and the middle hex can be filled with a
watchtower.

:i1.Floods
:li.To protect against floods, either build flood trenches that will
catch and divert flooding, or place trenches in open spots that blobs
don't build in. The first solution will keep flooding from occuring;
the second solution will drain water away if your town does get
flooding.

:li.To get the most from a water source, force the water go go down a
gentle slope. On a steep slope, water will flow faster, and your river
or channel will be shorter, so fewer farms will benefit.

:i1.Auto-Build
:li.Build a village and turn on Auto-Build mode. You can watch the
builder blobs go crazy trying to extend the village.

:eol.

:h1.Simulation 

:p.Farms and houses will be created automatically around roads; the
blobs will only move into a hex adjacent to a road. (They will not
move in next to a bridge.) If you have Auto Build Mode on, then roads
and bridges will be created and extended automatically.

:p.Anything that needs to be built will be marked with a hexagon, and
then a builder blob will go there to build it.

:p.Water will flow downhill. Trenches are 50 feet deep, so water will
flow into them. Walls are 120 feet high, so they block most
water. Gates are also 120 feet high, but they allow a small amount of
water to pass through. You can use gates to build spillways or
controlled release points on dams; or you can use them as openings on
the sides of an irrigation canal, to let out small amounts of water to
side canals.

:p.Economic, military, and political simulation are not yet in
place. I am using cellular automata and network flow algorithms to
determine how many people can "flow" from certain points (houses) to
other points (farms). I am using the same algorithms for making food
"flow" from farms to houses. For now, workers producing food forms the
entire basis for BlobCity's economy. BlobCity will probably not have
any military simulation and is unlikely to have any political
simulation; these two are more likely to be in the second SimBlob
game.

:euserdoc.
