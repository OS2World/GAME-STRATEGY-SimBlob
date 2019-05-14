# Amit uses something like this to put all his OBJ, D files on a ramdisk (H:)
# tvmount X:
# tvlink -rw X:\SimBlob H:\Obj
# tvlink -r  X:\SimBlob E:\Data\SimBlob

# -fcheck-memory-usage doesn't seem to work

# -W                enables many warnings
# -Weffc++          for C++ style guide
# -Wold-style-cast  for warning about old style casts
# -Wshadow          for variables shadowing outer scope variables
# -Wcast-align -Wcast-qual -Wconversion -Wimplicit 
# -Wbad-function-cast 

FLAGS = -MMD -O1 -Zomf -Zsys -Zmt -mstack-arg-probe -fstack-check -fno-exceptions -fvtable-thunks -ffor-scope -Woverloaded-virtual -Wtemplate-debugging -Wformat -Wpointer-arith -Wreturn-type -Wunused -mpentium -D__ST_MT_ERRNO__

OBJS = bitmaps.obj blitter.obj bmpformat.obj bufferwin.obj control.obj figment.obj gamewin.obj gameinit.obj glyph.obj glyphlib.obj images.obj initbitmaps.obj initmap.obj layer.obj layout.obj mainwin.obj map.obj mapcmd.obj menu.obj military.obj notion.obj paint.obj palette.obj path.obj rgbtable.obj simblob.obj simulate.obj sprites.obj statusbar.obj textglyph.obj terrain.obj tools.obj ui.obj unit.obj view.obj viewwin.obj water.obj worldmap.obj

all: simblob.exe

distrib:
	call mk.cmd

simblob.exe: simblob.res simblob.def $(OBJS)
	link386 /A:2 /M:full /F /PACKC /BATCH /NOO /BASE:0x10000 /EXEPACK:2 @build.rsp
	rc -x2 -n -w2 simblob.res simblob.exe

simblob.res: simblob.rc simblob.h initcode.h sb_time.rc sb_version.rc
	rc -x2 -r simblob.rc simblob.res

simblob.hlp: simblob.ipf
	ipfc simblob.ipf simblob.hlp /W3

clean:
	del *.obj 
	del *.d 
	del simblob.res
	del simblob.map	

%.obj: %.cpp
	gcc -c $(FLAGS) $< -o $@

include $(wildcard *.d)
