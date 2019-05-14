REM This is Amit's file for producing a ZIP file... 
e:
cd \data\simblob\distrib
copy \data\simblob\data\*.* Data
del Data\*~
REM copy \data\simblob\tmp\simblob.exe .
copy x:\simblob\simblob.exe .
\utl\lxlite\lxlite /V0O+ simblob.exe
del simblob.zip
zip -z -9 simblob.zip simblob.exe simblob.txt Data\*.* <simblob.dsc
