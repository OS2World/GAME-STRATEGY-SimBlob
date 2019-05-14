/* Rexx script to create SIMBLOB.DEF */
ECHO "; Do not edit this file" ">Simblob.def"
ECHO "NAME SimBlob WINDOWAPI" ">>Simblob.def"
ECHO "DESCRIPTION 'SimBlob - " DATE() TIME('Civil') "PST'" ">>Simblob.def"
ECHO "DATA MULTIPLE" ">>Simblob.def"
ECHO "STACKSIZE 65536" ">>Simblob.def"
ECHO "HEAPSIZE 8192" ">>Simblob.def"
ECHO "PROTMODE" ">>Simblob.def"

ECHO '"' DATE() '"' ">build.h"
ECHO '   LTEXT "Build time: ' DATE() TIME('Civil') "PST" '",' ">sb_time.rc"
ECHO '   ID_WINDOW "@#Amit Patel:' DATE() '#@SimBlob" >sb_version.rc"'
'TOUCH simblob.rc'

/* This script just puts the date and time into the resource file */