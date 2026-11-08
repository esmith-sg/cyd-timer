// Small web UI + JSON API so timers/reminders can be managed from a phone.
#include <WebServer.h>

#include "app.h"
#include "config.h"

static WebServer server(80);

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>CYD Timer</title><style>
body{font-family:system-ui,sans-serif;background:#121212;color:#eee;max-width:560px;margin:auto;padding:12px}
h1{font-size:1.4em}h2{margin:22px 0 8px;font-size:1.15em}
.card{background:#1f1f1f;border-radius:10px;padding:10px 12px;margin:6px 0;display:flex;align-items:center;gap:10px}
.grow{flex:1;min-width:0}.big{font-size:1.4em;font-variant-numeric:tabular-nums}.dim{color:#999;font-size:.85em}
.off{opacity:.45}
button{background:#2962ff;color:#fff;border:0;border-radius:8px;padding:8px 12px;font-size:1em}
button.del{background:#c62828}button.gray{background:#444}
input{background:#222;color:#eee;border:1px solid #444;border-radius:6px;padding:7px;font-size:1em}
form{display:flex;flex-wrap:wrap;gap:8px;align-items:center;background:#1a1a1a;padding:10px;border-radius:10px}
.days label{margin-right:8px;white-space:nowrap}
</style></head><body>
<h1>&#9200; CYD Timer</h1><div id=now class=dim></div>
<h2>Timers</h2><div id=timers></div>
<form id=tf><input id=th type=number min=0 max=23 placeholder=h style="width:56px">h
<input id=tm type=number min=0 max=59 placeholder=min style="width:64px">m
<input id=ts type=number min=0 max=59 placeholder=sec style="width:64px">s
<input id=tl placeholder="Label (optional)" maxlength=31 class=grow><button>Start</button></form>
<h2>Reminders</h2><div id=rems></div>
<form id=rf><b id=rh style="width:100%">New reminder</b><input type=hidden id=ri value=-1>
<input id=rt type=time required><input id=rx placeholder="What to remind you about" maxlength=47 class=grow required>
<div class=days id=rd style="width:100%"></div><button>Save</button>
<button type=button class=gray id=rc style="display:none">Cancel edit</button></form>
<script>
const D=['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
rd.innerHTML=D.map((d,i)=>`<label><input type=checkbox value=${i}> ${d}</label>`).join('')+'<span class=dim>(none = one time)</span>';
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const p2=n=>String(n).padStart(2,'0');
const fmt=s=>{const h=Math.floor(s/3600),m=Math.floor(s%3600/60),x=s%60;return(h?h+':'+p2(m):m)+':'+p2(x)};
const clk=(h,m)=>p2(h)+':'+p2(m);
const dname=d=>d==0?'One time':d==127?'Every day':d==62?'Weekdays':d==65?'Weekends':D.filter((_,i)=>d>>i&1).join(' ');
let S={timers:[],reminders:[]},t0=0;
async function post(u,p){const r=await fetch(u,{method:'POST',body:new URLSearchParams(p)});if(!r.ok)alert(await r.text());await load()}
async function load(){try{S=await(await fetch('/api/state')).json();t0=Date.now();drawR();drawT()}catch(e){now.textContent='Device not reachable'}}
function drawT(){
 const el=(Date.now()-t0)/1000;now.textContent=S.time||'';
 timers.innerHTML=S.timers.map(t=>`<div class=card><div class=grow>${esc(t.label)}</div>
 <div class="big ${t.paused?'off':''}">${fmt(Math.max(0,Math.ceil(t.paused?t.remain:t.remain-el)))}</div>
 <button class=gray onclick="post('/api/timer/pause',{i:${t.i}})">${t.paused?'&#9654;':'&#10074;&#10074;'}</button>
 <button class=del onclick="post('/api/timer/cancel',{i:${t.i}})">&#10005;</button></div>`).join('')||'<div class=dim>No timers running</div>';
}
function drawR(){
 const r=[...S.reminders].sort((a,b)=>a.h*60+a.m-(b.h*60+b.m));
 rems.innerHTML=r.map(x=>`<div class="card ${x.on?'':'off'}"><div class=big style="width:6.5em">${clk(x.h,x.m)}</div>
 <div class=grow><div>${esc(x.text)}</div><div class=dim>${dname(x.days)}</div></div>
 <input type=checkbox ${x.on?'checked':''} onchange="post('/api/reminder/enable',{i:${x.i},on:this.checked?1:0})">
 <button class=gray onclick="edit(${x.i})">Edit</button>
 <button class=del onclick="if(confirm('Delete this reminder?'))post('/api/reminder/delete',{i:${x.i}})">&#10005;</button></div>`).join('')||'<div class=dim>No reminders yet</div>';
}
function edit(i){const x=S.reminders.find(r=>r.i==i);ri.value=i;rt.value=p2(x.h)+':'+p2(x.m);rx.value=x.text;
 rd.querySelectorAll('input').forEach(c=>c.checked=x.days>>c.value&1);rh.textContent='Edit reminder';rc.style.display='';rf.scrollIntoView()}
function resetR(){rf.reset();ri.value=-1;rh.textContent='New reminder';rc.style.display='none'}
rc.onclick=resetR;
rf.onsubmit=async e=>{e.preventDefault();const[h,m]=rt.value.split(':');let d=0;rd.querySelectorAll('input:checked').forEach(c=>d|=1<<c.value);
 await post('/api/reminder',{i:ri.value,h:+h,m:+m,days:d,text:rx.value});resetR()};
tf.onsubmit=async e=>{e.preventDefault();const s=(+th.value||0)*3600+(+tm.value||0)*60+(+ts.value||0);
 if(!s)return alert('Enter a duration');await post('/api/timer',{s,label:tl.value});tf.reset()};
load();setInterval(drawT,500);setInterval(load,10000);
</script></body></html>)HTML";

static void jsonStr(String &out, const char *s) {
  out += '"';
  for (; *s; s++) {
    char c = *s;
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if ((uint8_t)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
    else out += c;
  }
  out += '"';
}

static int argInt(const char *name, int def) {
  return server.hasArg(name) && server.arg(name).length() ? server.arg(name).toInt() : def;
}

static void ok() { server.send(200, "text/plain", "ok"); }

static void handleState() {
  String j;
  j.reserve(2048);
  j += "{\"time\":";
  char buf[64] = "Clock not set yet (waiting for WiFi time)";
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "Device time: %a %b %e, %H:%M", &tm);
  }
  jsonStr(j, buf);
  j += ",\"timers\":[";
  bool first = true;
  for (int i = 0; i < MAX_TIMERS; i++) {
    if (!timers[i].active) continue;
    char lbl[40];
    timerDisplayLabel(i, lbl, sizeof(lbl));
    if (!first) j += ',';
    first = false;
    j += "{\"i\":" + String(i) + ",\"label\":";
    jsonStr(j, lbl);
    j += ",\"remain\":" + String(timerRemainingMs(timers[i]) / 1000.0f, 1);
    j += ",\"paused\":" + String(timers[i].paused ? "true" : "false") + "}";
  }
  j += "],\"reminders\":[";
  first = true;
  for (int i = 0; i < MAX_REMINDERS; i++) {
    const Reminder &r = reminders[i];
    if (!r.used) continue;
    if (!first) j += ',';
    first = false;
    j += "{\"i\":" + String(i) + ",\"h\":" + String(r.hour) + ",\"m\":" + String(r.minute) +
         ",\"days\":" + String(r.days) + ",\"on\":" + String(r.enabled ? "true" : "false") + ",\"text\":";
    jsonStr(j, r.text);
    j += "}";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

static void handleTimer() {
  int s = argInt("s", 0);
  if (s <= 0 || s > 24 * 3600) { server.send(400, "text/plain", "Duration must be 1 second to 24 hours"); return; }
  if (timerStart(s, server.arg("label").c_str()) < 0) { server.send(409, "text/plain", "All timer slots are in use"); return; }
  ok();
}

static bool validTimer(int i) { return i >= 0 && i < MAX_TIMERS && timers[i].active; }
static bool validReminder(int i) { return i >= 0 && i < MAX_REMINDERS && reminders[i].used; }

static void handleTimerCancel() {
  int i = argInt("i", -1);
  if (!validTimer(i)) { server.send(404, "text/plain", "No such timer"); return; }
  timerCancel(i);
  ok();
}

static void handleTimerPause() {
  int i = argInt("i", -1);
  if (!validTimer(i)) { server.send(404, "text/plain", "No such timer"); return; }
  timerTogglePause(i);
  ok();
}

static void handleReminder() {
  int i = argInt("i", -1);
  int h = argInt("h", -1), m = argInt("m", -1), days = argInt("days", 0);
  if (h < 0 || h > 23 || m < 0 || m > 59) { server.send(400, "text/plain", "Bad time"); return; }
  if (i >= 0 && !validReminder(i)) { server.send(404, "text/plain", "No such reminder"); return; }
  Reminder r = {};
  r.hour = h;
  r.minute = m;
  r.days = days & 0x7F;
  r.enabled = true;
  strlcpy(r.text, server.arg("text").c_str(), sizeof(r.text));
  if (reminderSave(i, r) < 0) { server.send(409, "text/plain", "Reminder list is full"); return; }
  ok();
}

static void handleReminderDelete() {
  int i = argInt("i", -1);
  if (!validReminder(i)) { server.send(404, "text/plain", "No such reminder"); return; }
  reminderDelete(i);
  ok();
}

static void handleReminderEnable() {
  int i = argInt("i", -1);
  if (!validReminder(i)) { server.send(404, "text/plain", "No such reminder"); return; }
  reminderSetEnabled(i, argInt("on", 1) != 0);
  ok();
}

void webInit() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/timer", HTTP_POST, handleTimer);
  server.on("/api/timer/cancel", HTTP_POST, handleTimerCancel);
  server.on("/api/timer/pause", HTTP_POST, handleTimerPause);
  server.on("/api/reminder", HTTP_POST, handleReminder);
  server.on("/api/reminder/delete", HTTP_POST, handleReminderDelete);
  server.on("/api/reminder/enable", HTTP_POST, handleReminderEnable);
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void webTick() { server.handleClient(); }
