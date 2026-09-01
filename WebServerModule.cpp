#include "WebServerModule.h"

#include "Config.h"
#include "Types.h"
#include "AlarmManager.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "SoundManager.h"

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>



// =====================================================
// WEB SERVER
// =====================================================

WebServer server(WEB_SERVER_PORT);

static File uploadFile;



// =====================================================
// HTML PAGE
// =====================================================

static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart RFID Alarm Clock</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#0f0f1a;color:#e0e0e0;min-height:100vh;display:flex;justify-content:center;align-items:flex-start;padding:20px}
.container{max-width:420px;width:100%}
.card{background:linear-gradient(135deg,#1a1a2e 0%,#16213e 100%);border-radius:16px;padding:24px;margin-bottom:16px;border:1px solid rgba(255,255,255,0.06);box-shadow:0 8px 32px rgba(0,0,0,0.4)}
h1{text-align:center;font-size:1.4em;color:#64ffda;margin-bottom:4px;letter-spacing:1px}
.subtitle{text-align:center;font-size:0.75em;color:#888;margin-bottom:16px}
.clock{text-align:center;font-size:3em;font-weight:700;color:#fff;letter-spacing:2px;margin:8px 0;font-variant-numeric:tabular-nums}
.date{text-align:center;color:#888;font-size:0.85em;margin-bottom:4px}
.status-bar{display:flex;gap:8px;justify-content:center;flex-wrap:wrap;margin-top:12px}
.status-dot{display:flex;align-items:center;gap:4px;font-size:0.7em;color:#aaa}
.status-dot .dot{width:8px;height:8px;border-radius:50%;background:#555}
.status-dot .dot.on{background:#4caf50;box-shadow:0 0 6px #4caf50}
.status-dot .dot.alarm{background:#ff5252;box-shadow:0 0 8px #ff5252;animation:pulse 1s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.4}}
.section-title{font-size:0.85em;color:#64ffda;margin-bottom:12px;text-transform:uppercase;letter-spacing:1px}
.row{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
.row label{font-size:0.9em;color:#ccc}
select,input[type=range]{background:#0d1117;color:#e0e0e0;border:1px solid #333;border-radius:8px;padding:6px 10px;font-size:0.9em}
select{min-width:70px;cursor:pointer}
select:focus{outline:none;border-color:#64ffda}
.toggle{position:relative;width:48px;height:26px;cursor:pointer}
.toggle input{opacity:0;width:0;height:0}
.toggle .slider{position:absolute;top:0;left:0;right:0;bottom:0;background:#333;border-radius:26px;transition:.3s}
.toggle .slider:before{content:'';position:absolute;width:20px;height:20px;left:3px;bottom:3px;background:#888;border-radius:50%;transition:.3s}
.toggle input:checked+.slider{background:#1b5e20}
.toggle input:checked+.slider:before{transform:translateX(22px);background:#64ffda}
input[type=range]{-webkit-appearance:none;width:140px;height:6px;border:none;border-radius:3px;background:#333;padding:0}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:#64ffda;cursor:pointer}
.vol-val{font-size:0.85em;color:#64ffda;min-width:32px;text-align:right}
.btn{display:block;width:100%;padding:12px;border:none;border-radius:10px;font-size:0.95em;font-weight:600;cursor:pointer;transition:all .2s;margin-top:8px}
.btn-primary{background:linear-gradient(135deg,#64ffda 0%,#00bfa5 100%);color:#0f0f1a}
.btn-primary:hover{transform:translateY(-1px);box-shadow:0 4px 16px rgba(100,255,218,0.3)}
.btn-danger{background:linear-gradient(135deg,#ff5252 0%,#d32f2f 100%);color:#fff;font-size:0.8em;padding:8px;margin-top:4px}
.btn-danger:hover{box-shadow:0 4px 12px rgba(255,82,82,0.3)}
.upload-zone{border:2px dashed #333;border-radius:12px;padding:20px;text-align:center;cursor:pointer;transition:all .3s;margin-top:8px}
.upload-zone:hover{border-color:#64ffda;background:rgba(100,255,218,0.03)}
.upload-zone.dragover{border-color:#64ffda;background:rgba(100,255,218,0.08)}
.upload-zone input{display:none}
.upload-zone p{color:#888;font-size:0.85em;margin-top:8px}
.upload-icon{font-size:2em;color:#64ffda}
.sound-item{display:flex;align-items:center;justify-content:space-between;padding:8px 12px;background:rgba(255,255,255,0.03);border-radius:8px;margin-bottom:6px}
.sound-item .name{font-size:0.85em;color:#ccc;flex:1}
.sound-item .del-btn{background:none;border:none;color:#ff5252;cursor:pointer;font-size:0.8em;padding:4px 8px;border-radius:4px}
.sound-item .del-btn:hover{background:rgba(255,82,82,0.15)}
.msg{text-align:center;font-size:0.8em;padding:8px;border-radius:6px;margin-top:8px;display:none}
.msg.ok{display:block;background:rgba(76,175,80,0.15);color:#4caf50}
.msg.err{display:block;background:rgba(255,82,82,0.15);color:#ff5252}
.progress{width:100%;height:4px;background:#333;border-radius:2px;margin-top:8px;display:none;overflow:hidden}
.progress .bar{height:100%;background:#64ffda;border-radius:2px;width:0%;transition:width .3s}
</style>
</head>
<body>
<div class="container">

<div class="card">
<h1>&#9200; Smart Alarm Clock</h1>
<p class="subtitle">RFID Dismiss &bull; v2.0</p>
<div class="clock" id="clock">--:--:--</div>
<div class="date" id="date">---</div>
<div class="status-bar">
<div class="status-dot"><span class="dot" id="st-wifi"></span>WiFi</div>
<div class="status-dot"><span class="dot" id="st-ntp"></span>NTP</div>
<div class="status-dot"><span class="dot" id="st-rfid"></span>RFID</div>
<div class="status-dot"><span class="dot" id="st-alarm"></span>Alarm</div>
</div>
</div>

<div class="card">
<div class="section-title">Alarm Settings</div>
<div class="row">
<label>Hour</label>
<select id="hour"></select>
</div>
<div class="row">
<label>Minute</label>
<select id="minute"></select>
</div>
<div class="row">
<label>AM / PM</label>
<select id="ampm"><option value="0">AM</option><option value="1">PM</option></select>
</div>
<div class="row">
<label>Enabled</label>
<label class="toggle"><input type="checkbox" id="enabled"><span class="slider"></span></label>
</div>
<div class="row">
<label>Volume</label>
<input type="range" id="volume" min="0" max="100" value="100">
<span class="vol-val" id="vol-val">100</span>
</div>
<div class="row">
<label>Sound</label>
<select id="sound"></select>
</div>
<button class="btn btn-primary" onclick="saveSettings()">Save Settings</button>
<div class="msg" id="save-msg"></div>
</div>

<div class="card">
<div class="section-title">Upload WAV Sound</div>
<div class="upload-zone" id="drop-zone">
<div class="upload-icon">&#127925;</div>
<p>Tap to select or drag a .wav file</p>
<p style="font-size:0.7em;color:#666">16-bit PCM &bull; Max 500KB</p>
<input type="file" id="wav-file" accept=".wav">
</div>
<div class="progress" id="progress"><div class="bar" id="progress-bar"></div></div>
<div class="msg" id="upload-msg"></div>
</div>

<div class="card">
<div class="section-title">Uploaded Sounds</div>
<div id="sound-list"></div>
</div>

</div>

<script>
// Populate hour/minute dropdowns
const hSel=document.getElementById('hour');
const mSel=document.getElementById('minute');
for(let i=1;i<=12;i++){let o=document.createElement('option');o.value=i;o.text=i<10?'0'+i:''+i;hSel.appendChild(o)}
for(let i=0;i<60;i++){let o=document.createElement('option');o.value=i;o.text=i<10?'0'+i:''+i;mSel.appendChild(o)}

const volSlider=document.getElementById('volume');
const volVal=document.getElementById('vol-val');
volSlider.oninput=()=>volVal.textContent=volSlider.value;

// Poll status every 1s
function poll(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('clock').textContent=d.time||'--:--:--';
document.getElementById('date').textContent=d.date||'---';
const w=d.wifi,n=d.ntp,r2=d.rfid,a=d.alarmActive;
document.getElementById('st-wifi').className='dot'+(w?' on':'');
document.getElementById('st-ntp').className='dot'+(n?' on':'');
document.getElementById('st-rfid').className='dot'+(r2?' on':'');
document.getElementById('st-alarm').className='dot'+(a?' on alarm':'');
}).catch(()=>{});
}
setInterval(poll,1000);poll();

// Load settings
function loadSettings(){
fetch('/api/settings').then(r=>r.json()).then(d=>{
hSel.value=d.hour;
mSel.value=d.minute;
document.getElementById('ampm').value=d.pm?'1':'0';
document.getElementById('enabled').checked=d.enabled;
volSlider.value=d.volume;volVal.textContent=d.volume;
loadSounds(d.sound);
}).catch(()=>{});
}

function loadSounds(selected){
fetch('/api/sounds').then(r=>r.json()).then(list=>{
const sel=document.getElementById('sound');
sel.innerHTML='';
list.forEach(s=>{
let o=document.createElement('option');
o.value=s;o.text=s==='builtin'?'Built-in Tone':s;
if(s===selected)o.selected=true;
sel.appendChild(o);
});
// Sound list
const div=document.getElementById('sound-list');
div.innerHTML='';
list.forEach(s=>{
if(s==='builtin')return;
let item=document.createElement('div');
item.className='sound-item';
item.innerHTML='<span class="name">'+s+'</span><button class="del-btn" onclick="delSound(\''+s+'\')">Delete</button>';
div.appendChild(item);
});
if(list.length<=1)div.innerHTML='<p style="color:#666;font-size:0.8em;text-align:center">No custom sounds uploaded</p>';
}).catch(()=>{});
}
loadSettings();

// Save
function saveSettings(){
const body={
hour:parseInt(hSel.value),
minute:parseInt(mSel.value),
pm:document.getElementById('ampm').value==='1',
enabled:document.getElementById('enabled').checked,
volume:parseInt(volSlider.value),
sound:document.getElementById('sound').value
};
fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)})
.then(r=>r.json()).then(d=>{
showMsg('save-msg',d.ok?'ok':'err',d.msg||'Saved!');
}).catch(()=>showMsg('save-msg','err','Connection error'));
}

function showMsg(id,type,text){
const el=document.getElementById(id);
el.className='msg '+type;el.textContent=text;
setTimeout(()=>el.style.display='none',3000);
el.style.display='block';
}

// Upload
const dropZone=document.getElementById('drop-zone');
const fileInput=document.getElementById('wav-file');
dropZone.onclick=()=>fileInput.click();
dropZone.ondragover=e=>{e.preventDefault();dropZone.classList.add('dragover')};
dropZone.ondragleave=()=>dropZone.classList.remove('dragover');
dropZone.ondrop=e=>{e.preventDefault();dropZone.classList.remove('dragover');if(e.dataTransfer.files.length)uploadFile(e.dataTransfer.files[0])};
fileInput.onchange=()=>{if(fileInput.files.length)uploadFile(fileInput.files[0])};

function uploadFile(file){
if(!file.name.toLowerCase().endsWith('.wav')){showMsg('upload-msg','err','Only .wav files');return}
if(file.size>500*1024){showMsg('upload-msg','err','File too large (max 500KB)');return}
const formData=new FormData();
formData.append('file',file);
const xhr=new XMLHttpRequest();
const prog=document.getElementById('progress');
const bar=document.getElementById('progress-bar');
prog.style.display='block';bar.style.width='0%';
xhr.upload.onprogress=e=>{if(e.lengthComputable)bar.style.width=Math.round(e.loaded/e.total*100)+'%'};
xhr.onload=()=>{
prog.style.display='none';
if(xhr.status===200){showMsg('upload-msg','ok','Uploaded!');loadSounds('');loadSettings()}
else showMsg('upload-msg','err',xhr.responseText||'Upload failed');
};
xhr.onerror=()=>{prog.style.display='none';showMsg('upload-msg','err','Upload error')};
xhr.open('POST','/api/upload');
xhr.send(formData);
}

function delSound(name){
if(!confirm('Delete '+name+'?'))return;
fetch('/api/delete?name='+encodeURIComponent(name),{method:'DELETE'})
.then(r=>r.json()).then(d=>{
showMsg('upload-msg',d.ok?'ok':'err',d.msg);
loadSettings();
}).catch(()=>{});
}
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
            "\"rfid\":true,"
            "\"alarmActive\":%s"
            "}",

            t.hour12, t.minute, t.second,
            t.pm ? "PM" : "AM",

            t.day, t.month, t.year,

            WiFi.status() == WL_CONNECTED
                ? "true" : "false",

            TimeManager::isSynced()
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
            server.send(
                200,
                "text/plain",
                "OK"
            );
        },

        // Upload handler
        []()
        {
            HTTPUpload &upload =
                server.upload();

            String filename =
                upload.filename;


            if (
                upload.status ==
                UPLOAD_FILE_START
            )
            {
                // Validate extension

                if (
                    !filename.endsWith(".wav") &&
                    !filename.endsWith(".WAV")
                )
                {
                    Serial.println(
                        "Upload rejected: "
                        "not a .wav file"
                    );

                    return;
                }


                // Check file count

                if (
                    SoundManager::getSoundCount()
                    >= (MAX_WAV_FILES + 1)
                )
                {
                    Serial.println(
                        "Upload rejected: "
                        "max files reached"
                    );

                    return;
                }


                String path =
                    SoundManager::getWavPath(
                        filename
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
            }


            if (
                upload.status ==
                UPLOAD_FILE_WRITE
            )
            {
                if (uploadFile)
                {
                    uploadFile.write(
                        upload.buf,
                        upload.currentSize
                    );
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
                        "(%d bytes)\n",
                        filename.c_str(),
                        upload.totalSize
                    );

                    SoundManager::addSound(
                        filename
                    );
                }
            }
        }
    );


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
