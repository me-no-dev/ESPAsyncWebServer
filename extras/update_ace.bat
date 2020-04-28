REM https://www.npmjs.com/package/github-files-fetcher   
REM npm install -g github-files-fetcher
REM fetcher --url=resource_url  --out=output_directory

call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/ace.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/mode-html.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/theme-monokai.js" --out=tmp1

cd tmp1
type ace.js mode-html.js theme-monokai.js > acefull.js
"C:\Program Files\7-Zip\7z.exe" a -tgzip -mx9 acefull.js.gz acefull.js
REM update:
pause
copy acefull.js.gz ..\..\examples\SmartSwitch\data\acefull.js.gz
del *.js *.gz

