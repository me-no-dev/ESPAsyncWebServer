copy ..\src\edit.htm edit.htm
"C:\Program Files\7-Zip\7z.exe" a -tgzip -mx9 edit.htm.gz edit.htm
ehg edit.htm.gz PROGMEM
copy edit.htm.gz.h ..\src\edit.htm.gz.h
pause
