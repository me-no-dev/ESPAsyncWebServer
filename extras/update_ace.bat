REM https://www.npmjs.com/package/github-files-fetcher   
REM npm install -g github-files-fetcher
REM fetcher --url=resource_url  --out=output_directory

call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/ace.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/mode-html.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/mode-json.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/theme-monokai.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/ext-searchbox.js" --out=tmp1

REM if you don't need worker(s), modify line#446 of edit.htm .setUseWorker(!0) to (!1) (true to false)
REM and do not take and include them below
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/worker-html.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/worker-css.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/worker-javascript.js" --out=tmp1
call fetcher --url="https://github.com/ajaxorg/ace-builds/blob/master/src-min-noconflict/worker-json.js" --out=tmp1

cd tmp1
type ace.js mode-html.js mode-json.js theme-monokai.js ext-searchbox.js > acefull.js
"C:\Program Files\7-Zip\7z.exe" a -tgzip -mx9 acefull.js.gz acefull.js
"C:\Program Files\7-Zip\7z.exe" a -tgzip -mx9 worker-html.js.gz worker-html.js
"C:\Program Files\7-Zip\7z.exe" a -tgzip -mx9 worker-javascript.js.gz worker-javascript.js
"C:\Program Files\7-Zip\7z.exe" a -tgzip -mx9 worker-json.js.gz worker-json.js
"C:\Program Files\7-Zip\7z.exe" a -tgzip -mx9 worker-css.js.gz worker-css.js

REM update SmartSwitch /data:
pause
copy acefull.js.gz ..\..\examples\SmartSwitch\data\acefull.js.gz
copy worker-html.js.gz ..\..\examples\SmartSwitch\data\worker-html.js.gz
copy worker-javascript.js.gz ..\..\examples\SmartSwitch\data\worker-javascript.js.gz
copy worker-json.js.gz ..\..\examples\SmartSwitch\data\worker-json.js.gz
copy worker-css.js.gz ..\..\examples\SmartSwitch\data\worker-css.js.gz

REM update ESP_AsyncFSBrowser /data:
pause
copy acefull.js.gz ..\..\examples\ESP_AsyncFSBrowser\data\acefull.js.gz
copy worker-html.js.gz ..\..\examples\ESP_AsyncFSBrowser\data\worker-html.js.gz
copy worker-javascript.js.gz ..\..\examples\ESP_AsyncFSBrowser\data\worker-javascript.js.gz
copy worker-json.js.gz ..\..\examples\ESP_AsyncFSBrowser\data\worker-json.js.gz
copy worker-css.js.gz ..\..\examples\ESP_AsyncFSBrowser\data\worker-css.js.gz

REM delete temporary stuff
pause
del *.js *.gz
