#include "WebServerModule.h"

#include "Config.h"
#include "Types.h"
#include "AlarmManager.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "SoundManager.h"
#include "RFIDManager.h"

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>



// =====================================================
// WEB SERVER
// =====================================================

WebServer server(WEB_SERVER_PORT);

static File uploadFile;

// Name the current upload is stored as, and why it
// failed ("" = OK). Reported by the completion handler.

static String uploadName;

static String uploadError;



// =====================================================
// HTML PAGE
// =====================================================

static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#f5f4f0" media="(prefers-color-scheme: light)">
<meta name="theme-color" content="#0f0f10" media="(prefers-color-scheme: dark)">
<title>Alarm Clock</title>
<style>
:root{--bg:#f5f4f0;--surface:#fff;--surface-2:#efede8;--text:#1c1c1e;--muted:#6f6e6b;--line:#e7e4de;--accent:#c4651f;--accent-soft:#f7e9dd;--on-accent:#fff;--ok:#2f8a57;--bad:#c4473a;--shadow:0 1px 2px rgba(0,0,0,.04),0 6px 20px rgba(0,0,0,.04);color-scheme:light}
@media (prefers-color-scheme:dark){:root{--bg:#0f0f10;--surface:#19191b;--surface-2:#242427;--text:#f2f2f0;--muted:#9a9995;--line:#2a2a2d;--accent:#f0a360;--accent-soft:#33261a;--on-accent:#1b1206;--ok:#5cc28a;--bad:#ef7363;--shadow:none;color-scheme:dark}}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
html{-webkit-text-size-adjust:100%}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,system-ui,sans-serif;background:var(--bg);color:var(--text);line-height:1.4;padding:max(16px,env(safe-area-inset-top)) max(16px,env(safe-area-inset-right)) calc(32px + env(safe-area-inset-bottom)) max(16px,env(safe-area-inset-left));min-height:100vh}
button,input{font:inherit;color:inherit}
button{background:none;border:0;cursor:pointer}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px;border-radius:8px}
.wrap{max-width:460px;margin:0 auto}
.now{text-align:center;padding:28px 0 22px}
.now-time{font-size:clamp(60px,19vw,88px);font-weight:300;letter-spacing:-.03em;line-height:1;font-variant-numeric:tabular-nums}
.now-time small{font-size:.28em;font-weight:500;letter-spacing:0;color:var(--muted);margin-left:6px}
.now-date{color:var(--muted);font-size:15px;margin-top:10px}
.chips{display:flex;justify-content:center;gap:6px;flex-wrap:wrap;margin-top:16px}
.chip{display:inline-flex;align-items:center;gap:6px;font-size:12px;font-weight:500;color:var(--muted);background:var(--surface-2);padding:5px 10px;border-radius:999px}
.chip i{width:6px;height:6px;border-radius:50%;background:var(--bad)}
.chip.ok i{background:var(--ok)}
.ring{display:none;background:var(--accent);color:var(--on-accent);border-radius:20px;padding:16px 20px;margin-bottom:12px;animation:glow 1.6s ease-in-out infinite}
.ring.on{display:block}
.ring b{display:block;font-size:17px}
.ring span{font-size:14px;opacity:.85}
@keyframes glow{50%{opacity:.82}}
.card{background:var(--surface);border:1px solid var(--line);border-radius:20px;padding:20px;margin-bottom:12px;box-shadow:var(--shadow)}
.head{display:flex;align-items:center;justify-content:space-between;gap:12px}
.label{font-size:13px;font-weight:600;color:var(--muted);letter-spacing:.02em}
.alarm-time{display:block;width:100%;border:0;background:none;font-size:clamp(52px,16vw,68px);font-weight:300;letter-spacing:-.02em;font-variant-numeric:tabular-nums;padding:6px 0 2px;cursor:pointer;transition:opacity .2s}
.alarm-time::-webkit-calendar-picker-indicator{display:none}
.alarm-time:focus{outline:none}
.off .alarm-time{opacity:.35}
.hint{font-size:14px;color:var(--muted)}
.hint b{color:var(--text);font-weight:600}
.off .hint b{color:var(--muted)}
.switch{position:relative;width:51px;height:31px;flex:none}
.switch input{position:absolute;opacity:0;inset:0;cursor:pointer;z-index:1}
.switch span{position:absolute;inset:0;background:var(--line);border-radius:31px;transition:background .2s}
.switch span:after{content:"";position:absolute;width:27px;height:27px;left:2px;top:2px;background:#fff;border-radius:50%;box-shadow:0 1px 3px rgba(0,0,0,.25);transition:transform .2s}
.switch input:checked+span{background:var(--accent)}
.switch input:checked+span:after{transform:translateX(20px)}
.vol{display:flex;align-items:center;gap:14px;margin-top:14px}
.vol output{font-size:14px;font-variant-numeric:tabular-nums;color:var(--muted);min-width:38px;text-align:right}
input[type=range]{-webkit-appearance:none;appearance:none;flex:1;height:32px;background:none;cursor:pointer}
input[type=range]::-webkit-slider-runnable-track{height:6px;border-radius:3px;background:linear-gradient(to right,var(--accent) var(--p,100%),var(--line) var(--p,100%))}
input[type=range]::-moz-range-track{height:6px;border-radius:3px;background:var(--line)}
input[type=range]::-moz-range-progress{height:6px;border-radius:3px;background:var(--accent)}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;margin-top:-9px;border-radius:50%;background:#fff;border:0;box-shadow:0 1px 4px rgba(0,0,0,.3)}
input[type=range]::-moz-range-thumb{width:24px;height:24px;border-radius:50%;background:#fff;border:0;box-shadow:0 1px 4px rgba(0,0,0,.3)}
.list{margin:6px -8px 0}
.snd{display:flex;align-items:center;gap:12px;min-height:56px;padding:4px 8px;border-radius:14px;cursor:pointer;transition:background .15s}
.snd:hover{background:var(--surface-2)}
.snd.sel{background:var(--accent-soft)}
.radio{width:22px;height:22px;flex:none;border-radius:50%;border:2px solid var(--line);display:grid;place-items:center;transition:border-color .15s}
.sel .radio{border-color:var(--accent)}
.sel .radio:after{content:"";width:10px;height:10px;border-radius:50%;background:var(--accent)}
.snd .name{flex:1;min-width:0;font-size:15px;line-height:1.3;overflow-wrap:anywhere;display:-webkit-box;-webkit-line-clamp:2;-webkit-box-orient:vertical;overflow:hidden;margin:6px 0}
.snd .name small{display:block;font-size:12px;color:var(--muted)}
.icon{width:40px;height:40px;flex:none;display:grid;place-items:center;border-radius:12px;color:var(--muted);transition:background .15s,color .15s}
.icon:hover{background:var(--surface);color:var(--text)}
.icon svg{width:20px;height:20px;fill:currentColor}
.icon.playing{color:var(--accent)}
.del.confirm{width:auto;padding:0 12px;color:var(--bad);font-size:13px;font-weight:600}
.add{display:flex;align-items:center;justify-content:center;gap:8px;width:100%;min-height:52px;margin-top:10px;border:1.5px dashed var(--line);border-radius:14px;color:var(--accent);font-weight:600;font-size:15px;transition:background .15s}
.add:hover{background:var(--surface-2)}
.add:disabled{opacity:.5;cursor:default}
.bar{height:4px;border-radius:2px;background:var(--line);overflow:hidden;margin-top:10px;display:none}
.bar div{height:100%;width:0;background:var(--accent);transition:width .2s}
.foot{font-size:12px;color:var(--muted);text-align:center;margin-top:10px}
.toast{position:fixed;left:50%;bottom:calc(24px + env(safe-area-inset-bottom));transform:translate(-50%,20px);background:var(--text);color:var(--bg);font-size:14px;font-weight:500;padding:10px 18px;border-radius:999px;opacity:0;pointer-events:none;transition:opacity .2s,transform .2s;max-width:calc(100% - 32px);text-align:center}
.toast.show{opacity:1;transform:translate(-50%,0)}
.toast.err{background:var(--bad);color:#fff}
</style>
</head>
<body>
<div class="wrap">

<header class="now">
<div class="now-time" id="now">--:--</div>
<div class="now-date" id="date">Connecting&hellip;</div>
<div class="chips">
<span class="chip" id="c-wifi"><i></i>Wi-Fi</span>
<span class="chip" id="c-ntp"><i></i>Time</span>
<span class="chip" id="c-rfid"><i></i>Reader</span>
</div>
</header>

<div class="ring" id="ring"><b>Alarm ringing</b><span>Scan your RFID tag to stop it</span></div>

<section class="card" id="alarm">
<div class="head">
<span class="label">ALARM</span>
<label class="switch"><input type="checkbox" id="enabled" aria-label="Alarm on or off"><span></span></label>
</div>
<input type="time" class="alarm-time" id="time" aria-label="Alarm time">
<div class="hint" id="until">&nbsp;</div>
<div class="vol">
<svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor" style="color:var(--muted);flex:none" aria-hidden="true"><path d="M3 10v4h4l5 5V5L7 10H3zm13.5 2A4.5 4.5 0 0 0 14 8v8a4.5 4.5 0 0 0 2.5-4z"/></svg>
<input type="range" id="vol" min="0" max="100" step="5" aria-label="Volume">
<output id="vol-val">100%</output>
</div>
</section>

<section class="card">
<span class="label">SOUND</span>
<div class="list" id="list"></div>
<button class="add" id="add"><svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M11 5h2v6h6v2h-6v6h-2v-6H5v-2h6z"/></svg><span id="add-txt">Add sound</span></button>
<div class="bar" id="bar"><div id="bar-fill"></div></div>
<input type="file" id="file" accept=".wav,audio/wav" hidden>
<p class="foot">WAV &middot; 8 or 16-bit &middot; up to 500 KB &middot; 5 sounds max</p>
</section>

</div>
<div class="toast" id="toast"></div>

<script>
const $=id=>document.getElementById(id);
const PLAY='<svg viewBox="0 0 24 24"><path d="M8 5v14l11-7z"/></svg>';
const STOP='<svg viewBox="0 0 24 24"><path d="M7 7h10v10H7z"/></svg>';
const TRASH='<svg viewBox="0 0 24 24"><path d="M9 3h6l1 2h4v2H4V5h4zm-3 6h12l-1 12H7z"/></svg>';
let S=null,sounds=[],playing=null,playTimer=null,nowMin=null;

function toast(msg,err){const t=$('toast');t.textContent=msg;t.className='toast show'+(err?' err':'');clearTimeout(t._h);t._h=setTimeout(()=>t.className='toast'+(err?' err':''),2200)}
function api(url,opt){return fetch(url,opt).then(r=>r.json().catch(()=>({})).then(d=>{if(!r.ok||d.ok===false)throw new Error(d.msg||'Something went wrong');return d}))}
const pad=n=>String(n).padStart(2,'0');
const pretty=s=>s==='builtin'?'Classic chime':s.replace(/\.wav$/i,'').replace(/[_]+/g,' ').trim();

// ---------- status ----------
function poll(){
fetch('/api/status').then(r=>r.json()).then(d=>{
const m=/(\d+):(\d+):(\d+) (AM|PM)/.exec(d.time||'');
if(m&&d.ntp){
$('now').innerHTML=(+m[1])+':'+m[2]+'<small>'+m[4]+'</small>';
nowMin=(+m[1]%12+(m[4]==='PM'?12:0))*60+(+m[2]);
const dm=/(\d+)\/(\d+)\/(\d+)/.exec(d.date||'');
if(dm)$('date').textContent=new Date(+dm[3],+dm[2]-1,+dm[1]).toLocaleDateString(undefined,{weekday:'long',day:'numeric',month:'long'});
}else{$('now').textContent='--:--';$('date').textContent='Waiting for time sync';nowMin=null}
$('c-wifi').className='chip'+(d.wifi?' ok':'');
$('c-ntp').className='chip'+(d.ntp?' ok':'');
$('c-rfid').className='chip'+(d.rfid?' ok':'');
$('ring').className='ring'+(d.alarmActive?' on':'');
until();
}).catch(()=>{$('date').textContent='Clock offline';['c-wifi','c-ntp','c-rfid'].forEach(i=>$(i).className='chip')});
}

// ---------- alarm ----------
function until(){
if(!S)return;
$('alarm').className='card'+(S.enabled?'':' off');
if(!S.enabled){$('until').innerHTML='Alarm is <b>off</b>';return}
if(nowMin===null){$('until').innerHTML='<b>On</b>';return}
const a=(S.hour%12+(S.pm?12:0))*60+S.minute;
let diff=(a-nowMin+1440)%1440;if(diff===0)diff=1440;
const h=Math.floor(diff/60),mi=diff%60;
$('until').innerHTML='Rings in <b>'+(h?h+' h ':'')+(mi||!h?mi+' min':'')+'</b>';
}
function renderAlarm(){
if(document.activeElement!==$('time'))$('time').value=pad(S.hour%12+(S.pm?12:0))+':'+pad(S.minute);
$('enabled').checked=S.enabled;
setVol(S.volume);
until();
}
function setVol(v){$('vol').value=v;$('vol').style.setProperty('--p',v+'%');$('vol-val').textContent=v+'%'}
function save(patch,msg){
Object.assign(S,patch);until();
return api('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(patch)})
.then(()=>{if(msg)toast(msg);if(playing){playing=null;renderList()}}).catch(e=>{toast(e.message,1);load()});
}
$('time').addEventListener('change',e=>{
const v=e.target.value;if(!v)return;
const [h,m]=v.split(':').map(Number);
save({hour:h%12||12,minute:m,pm:h>=12,enabled:true},'Alarm set for '+((h%12)||12)+':'+pad(m)+(h>=12?' PM':' AM'));
});
$('time').addEventListener('click',e=>{try{e.target.showPicker()}catch(_){}});
$('enabled').addEventListener('change',e=>save({enabled:e.target.checked},e.target.checked?'Alarm on':'Alarm off'));
$('vol').addEventListener('input',e=>setVol(e.target.value));
$('vol').addEventListener('change',e=>save({volume:+e.target.value},'Volume '+e.target.value+'%'));

// ---------- sounds ----------
function renderList(){
const L=$('list');L.innerHTML='';
sounds.forEach(s=>{
const row=document.createElement('div');
row.className='snd'+(S&&S.sound===s?' sel':'');
row.setAttribute('role','radio');row.setAttribute('aria-checked',S&&S.sound===s);row.tabIndex=0;
row.innerHTML='<span class="radio"></span><span class="name"></span>';
row.querySelector('.name').textContent=pretty(s);
if(s==='builtin')row.querySelector('.name').insertAdjacentHTML('beforeend','<small>Built in</small>');
const p=document.createElement('button');
p.className='icon'+(playing===s?' playing':'');p.innerHTML=playing===s?STOP:PLAY;p.setAttribute('aria-label',playing===s?'Stop preview':'Preview');
p.onclick=e=>{e.stopPropagation();preview(s)};
row.appendChild(p);
if(s!=='builtin'){
const d=document.createElement('button');
d.className='icon del';d.innerHTML=TRASH;d.setAttribute('aria-label','Delete');
d.onclick=e=>{e.stopPropagation();
if(!d.classList.contains('confirm')){d.classList.add('confirm');d.textContent='Delete?';setTimeout(()=>{if(d.isConnected){d.classList.remove('confirm');d.innerHTML=TRASH}},3000);return}
api('/api/delete?name='+encodeURIComponent(s),{method:'DELETE'}).then(()=>{toast('Sound deleted');if(playing===s)playing=null;load()}).catch(e=>toast(e.message,1));
};
row.appendChild(d);
}
const pick=()=>{if(S.sound!==s){save({sound:s},pretty(s)+' selected');renderList()}};
row.onclick=pick;row.onkeydown=e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();pick()}};
L.appendChild(row);
});
$('add').disabled=sounds.length>5;
$('add-txt').textContent=sounds.length>5?'Sound limit reached':'Add sound';
}
function preview(s){
clearTimeout(playTimer);
if(playing===s){playing=null;renderList();api('/api/test/stop',{method:'POST'}).catch(()=>{});return}
api('/api/test?sound='+encodeURIComponent(s),{method:'POST'}).then(()=>{
playing=s;renderList();
playTimer=setTimeout(()=>{playing=null;renderList()},15000);
}).catch(e=>toast(e.message,1));
}

// ---------- upload ----------
$('add').onclick=()=>$('file').click();
$('file').onchange=()=>{
const f=$('file').files[0];$('file').value='';
if(!f)return;
if(!/\.wav$/i.test(f.name)){toast('Please choose a .wav file',1);return}
if(f.size>500*1024){toast('File is too large (max 500 KB)',1);return}
const fd=new FormData();fd.append('file',f);
const x=new XMLHttpRequest();
$('bar').style.display='block';$('bar-fill').style.width='0';$('add').disabled=true;$('add-txt').textContent='Uploading...';
x.upload.onprogress=e=>{if(e.lengthComputable)$('bar-fill').style.width=(e.loaded/e.total*100)+'%'};
x.onloadend=()=>{
$('bar').style.display='none';$('add').disabled=false;
let d={};try{d=JSON.parse(x.responseText)}catch(_){}
if(x.status===200&&d.ok){S.sound=d.name;save({sound:d.name},'Added and selected').then(load)}
else{toast(d.msg||'Upload failed',1);load()}
};
x.open('POST','/api/upload');x.send(fd);
};

// ---------- load ----------
function load(){
Promise.all([fetch('/api/settings').then(r=>r.json()),fetch('/api/sounds').then(r=>r.json())])
.then(([s,l])=>{S=s;sounds=l;renderAlarm();renderList()}).catch(()=>{});
}
load();poll();setInterval(poll,1000);
</script>
</body>
</html>
)rawliteral";



// =====================================================
// BEGIN
// =====================================================

void WebServerModule::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("STARTING WEB SERVER");
    Serial.println("==============================");

    setupRoutes();

    server.begin();

    Serial.printf(
        "Web server on port %d\n",
        WEB_SERVER_PORT
    );
}



// =====================================================
// SETUP ROUTES
// =====================================================

void WebServerModule::setupRoutes()
{
    // -------------------------------------------------
    // MAIN PAGE
    // -------------------------------------------------

    server.on("/", HTTP_GET, []()
    {
        server.send_P(
            200,
            "text/html",
            PAGE_HTML
        );
    });


    // -------------------------------------------------
    // STATUS API
    // -------------------------------------------------

    server.on("/api/status", HTTP_GET, []()
    {
        CurrentTime t =
            TimeManager::getCurrentTime();

        char json[256];

        snprintf(
            json, sizeof(json),

            "{"
            "\"time\":\"%02d:%02d:%02d %s\","
            "\"date\":\"%02d/%02d/%04d\","
            "\"wifi\":%s,"
            "\"ntp\":%s,"
            "\"rfid\":%s,"
            "\"alarmActive\":%s"
            "}",

            t.hour12, t.minute, t.second,
            t.pm ? "PM" : "AM",

            t.day, t.month, t.year,

            WiFi.status() == WL_CONNECTED
                ? "true" : "false",

            TimeManager::isSynced()
                ? "true" : "false",

            RFIDManager::isConnected()
                ? "true" : "false",

            AlarmManager::isActive()
                ? "true" : "false"
        );

        server.send(
            200,
            "application/json",
            json
        );
    });


    // -------------------------------------------------
    // GET SETTINGS
    // -------------------------------------------------

    server.on("/api/settings", HTTP_GET, []()
    {
        AlarmSettings s =
            AlarmManager::getSettings();

        char json[256];

        snprintf(
            json, sizeof(json),

            "{"
            "\"hour\":%d,"
            "\"minute\":%d,"
            "\"pm\":%s,"
            "\"enabled\":%s,"
            "\"volume\":%d,"
            "\"sound\":\"%s\""
            "}",

            s.hour,
            s.minute,
            s.pm ? "true" : "false",
            s.enabled ? "true" : "false",
            s.volume,
            s.soundName
        );

        server.send(
            200,
            "application/json",
            json
        );
    });


    // -------------------------------------------------
    // SAVE SETTINGS
    // -------------------------------------------------

    server.on("/api/settings", HTTP_POST, []()
    {
        String body = server.arg("plain");

        AlarmSettings s =
            AlarmManager::getSettings();


        // Parse JSON manually
        // (avoid ArduinoJson dependency)

        int idx;

        // hour
        idx = body.indexOf("\"hour\":");
        if (idx >= 0)
        {
            s.hour = body.substring(
                idx + 7
            ).toInt();
        }

        // minute
        idx = body.indexOf("\"minute\":");
        if (idx >= 0)
        {
            s.minute = body.substring(
                idx + 9
            ).toInt();
        }

        // pm
        idx = body.indexOf("\"pm\":");
        if (idx >= 0)
        {
            s.pm = body.substring(
                idx + 5
            ).startsWith("true");
        }

        // enabled
        idx = body.indexOf("\"enabled\":");
        if (idx >= 0)
        {
            s.enabled = body.substring(
                idx + 10
            ).startsWith("true");
        }

        // volume
        idx = body.indexOf("\"volume\":");
        if (idx >= 0)
        {
            s.volume = body.substring(
                idx + 9
            ).toInt();
        }

        // sound
        idx = body.indexOf("\"sound\":\"");
        if (idx >= 0)
        {
            int start = idx + 9;

            int end = body.indexOf(
                '"', start
            );

            if (end > start)
            {
                String snd =
                    body.substring(start, end);

                if (
                    !SoundManager::isBuiltin(snd) &&
                    !SoundManager::wavExists(snd)
                )
                {
                    server.send(
                        400,
                        "application/json",
                        "{\"ok\":false,"
                        "\"msg\":\"Sound file not found\"}"
                    );

                    return;
                }

                strncpy(
                    s.soundName,
                    snd.c_str(),
                    sizeof(s.soundName) - 1
                );

                s.soundName[
                    sizeof(s.soundName) - 1
                ] = '\0';
            }
        }


        AlarmManager::setSettings(s);

        // Stop audio if playing
        // (settings changed)

        if (AudioPlayer::isPlaying())
        {
            AudioPlayer::stop();
        }


        server.send(
            200,
            "application/json",
            "{\"ok\":true,\"msg\":\"Settings saved!\"}"
        );
    });


    // -------------------------------------------------
    // SOUNDS LIST
    // -------------------------------------------------

    server.on("/api/sounds", HTTP_GET, []()
    {
        String json = "[";

        int count =
            SoundManager::getSoundCount();

        for (int i = 0; i < count; i++)
        {
            if (i > 0) json += ",";

            json += "\"";
            json += SoundManager::getSoundName(i);
            json += "\"";
        }

        json += "]";

        server.send(
            200,
            "application/json",
            json
        );
    });


    // -------------------------------------------------
    // UPLOAD WAV
    // -------------------------------------------------

    server.on(
        "/api/upload",
        HTTP_POST,

        // Completion handler
        []()
        {
            if (uploadError.length() > 0)
            {
                server.send(
                    400,
                    "application/json",
                    "{\"ok\":false,\"msg\":\"" +
                        uploadError + "\"}"
                );

                return;
            }

            server.send(
                200,
                "application/json",
                "{\"ok\":true,\"name\":\"" +
                    uploadName + "\"}"
            );
        },

        // Upload handler
        []()
        {
            HTTPUpload &upload =
                server.upload();


            if (
                upload.status ==
                UPLOAD_FILE_START
            )
            {
                uploadError = "";

                uploadName =
                    SoundManager::sanitizeName(
                        upload.filename
                    );


                // Validate extension

                if (uploadName.length() == 0)
                {
                    uploadError = "Only .wav files";

                    Serial.println(
                        "Upload rejected: "
                        "not a .wav file"
                    );

                    return;
                }


                // Check file count

                if (
                    !SoundManager::hasRoomFor(
                        uploadName
                    )
                )
                {
                    uploadError =
                        "Max " + String(MAX_WAV_FILES) +
                        " sounds - delete one first";

                    Serial.println(
                        "Upload rejected: "
                        "max files reached"
                    );

                    return;
                }


                // Don't overwrite a file that is
                // being played right now

                if (
                    AudioPlayer::isPlaying() &&
                    !AlarmManager::isActive()
                )
                {
                    AudioPlayer::stop();
                }


                String path =
                    SoundManager::getWavPath(
                        uploadName
                    );

                Serial.printf(
                    "Upload start: %s\n",
                    path.c_str()
                );


                uploadFile =
                    LittleFS.open(
                        path,
                        FILE_WRITE
                    );

                if (!uploadFile)
                {
                    uploadError = "Could not create file";
                }
            }


            if (
                upload.status ==
                UPLOAD_FILE_WRITE
            )
            {
                if (uploadFile)
                {
                    bool tooBig =
                        upload.totalSize >
                        MAX_WAV_FILE_SIZE;

                    size_t written = 0;

                    if (!tooBig)
                    {
                        written = uploadFile.write(
                            upload.buf,
                            upload.currentSize
                        );
                    }


                    if (
                        tooBig ||
                        written != upload.currentSize
                    )
                    {
                        uploadError =
                            tooBig
                                ? "File too large (max 500KB)"
                                : "Storage full";

                        uploadFile.close();

                        LittleFS.remove(
                            SoundManager::getWavPath(
                                uploadName
                            )
                        );
                    }
                }
            }


            if (
                upload.status ==
                UPLOAD_FILE_END
            )
            {
                if (uploadFile)
                {
                    uploadFile.close();

                    Serial.printf(
                        "Upload done: %s "
                        "(%u bytes)\n",
                        uploadName.c_str(),
                        (unsigned)upload.totalSize
                    );


                    // Make sure we can actually
                    // play it, otherwise the alarm
                    // would silently fall back to
                    // the built-in tone

                    String path =
                        SoundManager::getWavPath(
                            uploadName
                        );

                    String error;

                    if (
                        !AudioPlayer::checkWavFile(
                            path,
                            error
                        )
                    )
                    {
                        uploadError =
                            "Unsupported WAV: " + error;

                        Serial.printf(
                            "Upload rejected: %s\n",
                            error.c_str()
                        );

                        LittleFS.remove(path);
                    }
                }

                SoundManager::rescan();
            }


            if (
                upload.status ==
                UPLOAD_FILE_ABORTED
            )
            {
                uploadError = "Upload aborted";

                if (uploadFile)
                {
                    uploadFile.close();

                    LittleFS.remove(
                        SoundManager::getWavPath(
                            uploadName
                        )
                    );
                }

                SoundManager::rescan();
            }
        }
    );


    // -------------------------------------------------
    // TEST SOUND (15 second preview)
    // -------------------------------------------------

    server.on("/api/test", HTTP_POST, []()
    {
        if (AlarmManager::isActive())
        {
            server.send(
                409,
                "application/json",
                "{\"ok\":false,"
                "\"msg\":\"Alarm is ringing\"}"
            );

            return;
        }


        String name = server.arg("sound");

        if (
            !SoundManager::isBuiltin(name) &&
            !SoundManager::wavExists(name)
        )
        {
            server.send(
                404,
                "application/json",
                "{\"ok\":false,"
                "\"msg\":\"Sound not found\"}"
            );

            return;
        }


        AudioPlayer::stop();

        AudioPlayer::start(name, 15000);

        server.send(
            200,
            "application/json",
            "{\"ok\":true}"
        );
    });


    server.on("/api/test/stop", HTTP_POST, []()
    {
        if (!AlarmManager::isActive())
        {
            AudioPlayer::stop();
        }

        server.send(
            200,
            "application/json",
            "{\"ok\":true}"
        );
    });


    // -------------------------------------------------
    // DELETE SOUND
    // -------------------------------------------------

    server.on("/api/delete", HTTP_DELETE, []()
    {
        String name = server.arg("name");

        if (name.length() == 0)
        {
            server.send(
                400,
                "application/json",
                "{\"ok\":false,"
                "\"msg\":\"No name\"}"
            );

            return;
        }


        if (name == "builtin")
        {
            server.send(
                400,
                "application/json",
                "{\"ok\":false,"
                "\"msg\":\"Cannot delete "
                "built-in tone\"}"
            );

            return;
        }


        // Don't delete a file while it's playing.
        // If the alarm is ringing, the main loop
        // restarts it with the built-in tone.

        AudioPlayer::stop();


        bool ok =
            SoundManager::removeSound(name);

        if (ok)
        {
            server.send(
                200,
                "application/json",
                "{\"ok\":true,"
                "\"msg\":\"Deleted!\"}"
            );
        }
        else
        {
            server.send(
                500,
                "application/json",
                "{\"ok\":false,"
                "\"msg\":\"Delete failed\"}"
            );
        }
    });
}
