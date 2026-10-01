#pragma once
// Captive portal — settings + Chronos + SD upload (same-origin)
static const char CAPTIVE_HTML[] PROGMEM = R"MOCHI(
<!DOCTYPE html><html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>Mochi rzmong</title>
<style>
body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:12px;max-width:420px}
h1{font-size:1.15rem;color:#fd0}h2{font-size:.95rem;color:#7ef;margin:14px 0 6px}
label{display:block;margin:8px 0 4px;color:#aaa;font-size:.85rem}
select,input[type=range],input[type=text],input[type=file]{width:100%;padding:8px;border-radius:8px;border:1px solid #333;background:#222;color:#fff;box-sizing:border-box}
.tog{display:flex;justify-content:space-between;align-items:center;padding:10px;background:#1a1a1a;border-radius:10px;margin:6px 0}
button{width:100%;padding:12px;margin:6px 0;border:0;border-radius:10px;font-weight:600}
.p{background:#fd0;color:#000}.s{background:#234;color:#9ef}.u{background:#1a5;color:#fff}
#st{font-size:.8rem;color:#888;margin-top:8px;white-space:pre-wrap}
.box{background:#1a1a22;border:1px solid #333;border-radius:12px;padding:12px;margin:10px 0}
.hint{font-size:.72rem;color:#666;margin:4px 0 8px}a{color:#7ef}
</style></head><body>
<h1>🍡 Mochi · rzmong</h1>
<p class=hint>v0.5.6 · offline · http://192.168.4.1/</p>
<p class=hint>😶 wajah · 🤖 gundam · 🚗 mobil · 🚓 polisi · 🎵 musik · 💜 neon · 🌸 anime · 🍜 makanan · ✨ intro</p>
<div class=box>
<label>Tema</label><select id=theme></select>
<label>Sumber</label><select id=storage><option value=sd>SD</option><option value=flash>Flash</option></select>
<label>Mode putar</label><select id=mode><option value=kategori>Kategori</option><option value=acak>Acak semua tema</option><option value=acak_tema>Acak dalam tema</option></select>
<label>Reaksi</label><select id=rmode><option value=acak>Acak</option><option value=tetap>Tetap</option></select>
<label>Volume <span id=vv></span></label><input type=range id=vol min=0 max=21 value=12>
<div class=tog><span>SFX</span><input type=checkbox id=sound checked></div>
</div>
<div class=box>
<h2>📱 Chronos (BLE)</h2>
<p class=hint>Pair di app Chronos · BLE name <b>rzmong</b></p>
<div class=tog><span>Chronos BLE</span><input type=checkbox id=chronos></div>
<div class=tog><span>Jam HP</span><input type=checkbox id=clock></div>
<div class=tog><span>Tampil navigasi</span><input type=checkbox id=chronos_nav checked></div>
<div id=chinfo class=hint>—</div>
</div>
<div class=box>
<h2>💾 Upload ke SD</h2>
<p class=hint>Convert di Studio (online) dulu, unduh file, lalu upload di sini (same-origin).</p>
<label>Jenis</label><select id=utype><option value=gif>GIF</option><option value=wav>WAV</option></select>
<label>Tema folder</label><select id=utema></select>
<label>Stem (tanpa ekstensi)</label><input type=text id=ustem placeholder=raspberry>
<label>File</label><input type=file id=ufile accept=".gif,.wav,image/gif,audio/wav">
<button class=u type=button onclick=upload()>Upload</button>
</div>
<button class=p onclick=save()>Simpan ke Mochi</button>
<button class=s onclick=load()>Muat status</button>
<div id=st></div>
<script>
const T=['wajah','gundam','mobil','polisi','musik','neon','anime','makanan','intro'];
const ts=document.getElementById('theme'), ut=document.getElementById('utema');
T.forEach(t=>{[ts,ut].forEach(s=>{const o=document.createElement('option');o.value=t;o.textContent=t;s.appendChild(o);});});
document.getElementById('vol').oninput=e=>document.getElementById('vv').textContent=e.target.value;
async function load(){try{const j=await(await fetch('/api/status')).json();
if(j.theme)ts.value=j.theme;if(j.storage)document.getElementById('storage').value=j.storage;
if(j.mode)document.getElementById('mode').value=j.mode;if(j.react_mode)document.getElementById('rmode').value=j.react_mode;
if(typeof j.vol==='number'){document.getElementById('vol').value=j.vol;document.getElementById('vv').textContent=j.vol;}
if(typeof j.sound==='boolean')document.getElementById('sound').checked=j.sound;
if(typeof j.chronos==='boolean')document.getElementById('chronos').checked=j.chronos;
if(typeof j.clock==='boolean')document.getElementById('clock').checked=j.clock;
if(typeof j.chronos_nav==='boolean')document.getElementById('chronos_nav').checked=j.chronos_nav;
let ci='BLE '+(j.chronos_run?'ON':'off')+(j.chronos_conn?' linked':'');
if(j.chronos_mac)ci+='\n'+j.chronos_mac;if(j.nav_active)ci+='\nNav '+(j.nav_title||'');
document.getElementById('chinfo').textContent=ci;
document.getElementById('st').textContent='v'+(j.ver||'?')+' · SSID '+(j.ap_ssid||'');
}catch(e){document.getElementById('st').textContent='Gagal muat';}}
async function save(){const body={theme:ts.value,storage:document.getElementById('storage').value,play_mode:document.getElementById('mode').value,react_mode:document.getElementById('rmode').value,volume:+document.getElementById('vol').value,sound:document.getElementById('sound').checked,chronos:document.getElementById('chronos').checked,chronos_nav:document.getElementById('chronos_nav').checked,clock:document.getElementById('clock').checked};
try{const r=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
document.getElementById('st').textContent=r.ok?'Tersimpan':'Gagal';if(r.ok)setTimeout(load,400);}catch(e){document.getElementById('st').textContent='Offline';}}
async function upload(){
const f=document.getElementById('ufile').files[0];
const stem=(document.getElementById('ustem').value||'').trim()||(f&&f.name.replace(/\.[^.]+$/,''))||'file';
const tema=document.getElementById('utema').value, type=document.getElementById('utype').value;
if(!f){document.getElementById('st').textContent='Pilih file';return;}
const fd=new FormData(); fd.append('file',f,f.name);
document.getElementById('st').textContent='Mengunggah…';
try{
const r=await fetch('/api/upload?tema='+encodeURIComponent(tema)+'&stem='+encodeURIComponent(stem)+'&type='+type,{method:'POST',body:fd});
const t=await r.text();
document.getElementById('st').textContent=r.ok?('OK '+t):('Gagal '+t);
}catch(e){document.getElementById('st').textContent='Upload gagal: '+e;}
}
load();
</script></body></html>
)MOCHI";
