/* Mochi DFPlayer: model kabel bersama untuk pemasangan.html (langkah demi langkah) dan pemasangan-kabel.html (diagram akhir).
 * Pin dicek terhadap firmware/include/MochiRzmong.h dan firmware/include/User_Setup_ST7789.h (firmware 0.7.0).
 * Koordinat dalam unit SVG (viewBox 0 0 1100 745). Rute kabel hanya ilustrasi; yang penting pin ke pin. */
(function (g) {
'use strict';
const NET = {'5V':'#e63946',GND:'#2b2b2b','3V3':'#ff7a00',G4:'#2bb24c',G6:'#2f6fe4',G3:'#8e44ec',G10:'#14b8c4',G7:'#9a5b2c',
  G1:'#c9a400',G20:'#65a30d',G21:'#ff6b6b',G5:'#6b7280',G8:'#127a3a',G9:'#e335b5',SPK:'#ff4fa3'};
const NETNAME = {'5V':'5V / VBUS','GND':'GND bersama','3V3':'3V3',G4:'GPIO4 · SCL (SPI clock)',G6:'GPIO6 · SDA (SPI data)',
  G3:'GPIO3 · DC',G10:'GPIO10 · RES',G7:'GPIO7 · BLK (lampu latar)',G1:'GPIO1 · sentuh',G20:'GPIO20 → RX DFPlayer (lewat 1 kΩ)',
  G21:'GPIO21 ← TX DFPlayer',G5:'GPIO5 ← BUSY (opsional)',G8:'GPIO8 · SDA MPU6050',G9:'GPIO9 · SCL MPU6050',SPK:'Speaker 8 Ω'};

// ---------- pin ----------
const P = {};
const ESPL = [['5V',330],['GND',358],['3V3',386],['G4',414],['G3',442],['G2',470],['G1',498],['G0',526]];
const ESPR = [['G5',330],['G6',358],['G7',386],['G8',414],['G9',442],['G10',470],['G20',498],['G21',526]];
ESPL.forEach(([n, y]) => P['ESP.' + n] = {x: 454, y});
ESPR.forEach(([n, y]) => P['ESP.' + n] = {x: 586, y});
const TFTP = ['GND','VCC','SCL','SDA','RES','DC','BLK'];
TFTP.forEach((n, i) => P['TFT.' + n] = {x: 355 + i * 38, y: 184});
const DFL = ['VCC','RX','TX','DAC_R','DAC_L','SPK_1','GND','SPK_2'];
const DFR = ['BUSY','USB−','USB+','ADKEY2','ADKEY1','IO2','GND ','IO1'];
DFL.forEach((n, i) => P['DF.' + n] = {x: 834, y: 410 + i * 28});
DFR.forEach((n, i) => P['DF.r' + n.trim()] = {x: 986, y: 410 + i * 28});
Object.assign(P, {'TTP.GND': {x: 95, y: 500}, 'TTP.IO': {x: 135, y: 500}, 'TTP.VCC': {x: 175, y: 500}});
const MPUP = ['VCC','GND','SCL','SDA','XDA','XCL','AD0','INT'];
MPUP.forEach((n, i) => P['MPU.' + n] = {x: 488 + i * 22, y: 636});
Object.assign(P, {'SPK.+': {x: 748, y: 655}, 'SPK.-': {x: 748, y: 690}, 'CAP.+': {x: 880, y: 312}, 'CAP.-': {x: 880, y: 352},
  'RAIL.L': {x: 22, y: 358}, 'RAIL.B': {x: 22, y: 725}});
const PINNAME = {'ESP.G4':'GPIO4','ESP.G6':'GPIO6','ESP.G3':'GPIO3','ESP.G10':'GPIO10','ESP.G7':'GPIO7','ESP.G1':'GPIO1','ESP.G20':'GPIO20',
  'ESP.G21':'GPIO21','ESP.G5':'GPIO5','ESP.G8':'GPIO8','ESP.G9':'GPIO9','ESP.5V':'5V','ESP.GND':'GND','ESP.3V3':'3V3'};

// ---------- kabel: step, dari, ke, net, titik, label, opsi ----------
const W = [];
function w(step, a, b, net, pts, label, o) { W.push(Object.assign({step, a, b, net, pts, label}, o || {})); }
// layar ST7789 (7 kabel) + rel GND bersama
w('tft', 'ESP.GND', 'RAIL', 'GND', [[454,358],[22,358],[22,725],[1040,725]], 'ESP GND → rel GND bersama', {rail: 1});
w('tft', 'TFT.GND', 'RAIL', 'GND', [[355,184],[355,198],[22,198],[22,358]], 'Layar GND → GND');
w('tft', 'TFT.VCC', 'ESP.3V3', '3V3', [[454,386],[430,386],[430,214],[393,214],[393,184]], 'Layar VCC → 3V3');
w('tft', 'TFT.SCL', 'ESP.G4', 'G4', [[454,414],[420,414],[420,206],[431,206],[431,184]], 'Layar SCL → GPIO4');
w('tft', 'TFT.SDA', 'ESP.G6', 'G6', [[586,358],[612,358],[612,238],[469,238],[469,184]], 'Layar SDA → GPIO6');
w('tft', 'TFT.RES', 'ESP.G10', 'G10', [[586,470],[632,470],[632,230],[507,230],[507,184]], 'Layar RES → GPIO10');
w('tft', 'TFT.DC', 'ESP.G3', 'G3', [[454,442],[410,442],[410,246],[545,246],[545,184]], 'Layar DC → GPIO3');
w('tft', 'TFT.BLK', 'ESP.G7', 'G7', [[586,386],[622,386],[622,222],[583,222],[583,184]], 'Layar BLK → GPIO7');
// TTP223
w('ttp', 'TTP.GND', 'RAIL', 'GND', [[95,500],[95,440],[22,440]], 'TTP223 GND → GND');
w('ttp', 'TTP.IO', 'ESP.G1', 'G1', [[454,498],[400,498],[400,450],[135,450],[135,500]], 'TTP223 OUT (I/O) → GPIO1');
w('ttp', 'TTP.VCC', 'ESP.3V3', '3V3', [[430,386],[430,470],[175,470],[175,500]], 'TTP223 VCC → 3V3', {j: [[430,386]]});
// DFPlayer data
w('df', 'DF.RX', 'ESP.G20', 'G20', [[586,498],[720,498],[720,438],[834,438]], 'DFPlayer RX ← GPIO20 lewat 1 kΩ', {res: {x: 785, y: 438, t: '1 kΩ'}});
w('df', 'DF.TX', 'ESP.G21', 'G21', [[586,526],[735,526],[735,466],[834,466]], 'DFPlayer TX → GPIO21 (1 kΩ opsional)', {res: {x: 785, y: 466, t: '1 kΩ opsional', opt: 1}});
w('df', 'DF.rBUSY', 'ESP.G5', 'G5', [[586,330],[604,330],[604,254],[1015,254],[1015,410],[986,410]], 'DFPlayer BUSY → GPIO5 (opsional)', {dash: 1});
// DFPlayer daya + kapasitor
w('dfpwr', 'DF.VCC', 'ESP.5V', '5V', [[454,330],[436,330],[436,262],[800,262],[800,410],[834,410]], 'DFPlayer VCC → 5V / VBUS');
w('dfpwr', 'DF.rGND', 'RAIL', 'GND', [[986,578],[1040,578],[1040,725]], 'DFPlayer GND → GND');
w('dfpwr', 'CAP.+', 'DF.VCC', '5V', [[800,300],[880,300],[880,312]], 'Kapasitor (+) → VCC DFPlayer', {j: [[800,300]]});
w('dfpwr', 'CAP.-', 'RAIL', 'GND', [[880,352],[880,362],[1040,362],[1040,578]], 'Kapasitor (−) → GND');
// speaker
w('spk', 'DF.SPK_1', 'SPK.+', 'SPK', [[834,550],[790,550],[790,655],[748,655]], 'SPK_1 → speaker');
w('spk', 'DF.SPK_2', 'SPK.-', 'SPK', [[834,606],[805,606],[805,690],[748,690]], 'SPK_2 → speaker');
// MPU6050 (opsional)
w('mpu', 'MPU.VCC', 'ESP.3V3', '3V3', [[430,470],[430,600],[488,600],[488,636]], 'MPU6050 VCC → 3V3', {j: [[430,470]]});
w('mpu', 'MPU.GND', 'RAIL', 'GND', [[510,636],[510,612],[372,612],[372,725]], 'MPU6050 GND → GND');
w('mpu', 'MPU.SCL', 'ESP.G9', 'G9', [[586,442],[640,442],[640,588],[532,588],[532,636]], 'MPU6050 SCL → GPIO9');
w('mpu', 'MPU.SDA', 'ESP.G8', 'G8', [[586,414],[650,414],[650,598],[554,598],[554,636]], 'MPU6050 SDA → GPIO8');

const PADS = {
  esp: ['ESP.5V','ESP.GND','ESP.3V3','ESP.G4','ESP.G3','ESP.G1','ESP.G5','ESP.G6','ESP.G7','ESP.G8','ESP.G9','ESP.G10','ESP.G20','ESP.G21'],
  tft: TFTP.map(n => 'TFT.' + n).concat(['ESP.GND','ESP.3V3','ESP.G4','ESP.G6','ESP.G10','ESP.G3','ESP.G7']),
  ttp: ['TTP.GND','TTP.IO','TTP.VCC','ESP.G1','ESP.3V3'],
  df: ['DF.RX','DF.TX','DF.rBUSY','ESP.G20','ESP.G21','ESP.G5'],
  dfpwr: ['DF.VCC','DF.rGND','CAP.+','CAP.-','ESP.5V'],
  spk: ['DF.SPK_1','DF.SPK_2','SPK.+','SPK.-'],
  mpu: ['MPU.VCC','MPU.GND','MPU.SCL','MPU.SDA','ESP.3V3','ESP.G8','ESP.G9']
};

// ---------- gambar ----------
function el(t, a, inner) { let s = '<' + t; for (const k in a) s += ` ${k}="${a[k]}"`; return s + (inner !== undefined ? '>' + inner + '</' + t + '>' : '/>'); }
function T(x, y, s, o) { return el('text', Object.assign({x, y, 'font-size': 10, 'text-anchor': 'middle', fill: '#fff', 'font-weight': 700}, o || {}), s); }
function pin(k, on, dim) {
  const p = P[k];
  return el('circle', {cx: p.x, cy: p.y, r: 5, fill: '#e6c35c', stroke: '#7a5c12', 'stroke-width': 1.2, opacity: dim ? .45 : 1}) +
    el('circle', {cx: p.x, cy: p.y, r: 2, fill: '#fff8e7'}) +
    (on ? el('circle', {cx: p.x, cy: p.y, r: 8.5, fill: 'none', stroke: '#ff5c98', 'stroke-width': 2.5, class: 'pad-hi'}) : '');
}
function mods(show, hi) {
  const has = k => show.includes(k), on = k => hi.includes(k);
  let s = '';
  // ESP32-C3 Super Mini (selalu digambar, redup sebelum langkahnya)
  s += `<g opacity="${has('esp') ? 1 : .35}">` + el('rect', {x: 440, y: 300, width: 160, height: 250, rx: 12, fill: '#1d212b'}) +
    el('rect', {x: 492, y: 280, width: 56, height: 34, rx: 8, fill: '#c9ced8', stroke: '#8b93a1'}) +
    el('rect', {x: 502, y: 392, width: 36, height: 36, rx: 3, fill: '#30384a', transform: 'rotate(45 520 410)'}) +
    T(520, 540, 'ESP32-C3 Super Mini', {'font-size': 10}) + T(520, 274, 'USB-C', {fill: '#6b7280', 'font-size': 10});
  const used = PADS.esp;
  ESPL.forEach(([n, y]) => { const k = 'ESP.' + n, u = used.includes(k);
    s += pin(k, on(k), !u) + el('rect', {x: 464, y: y - 7, width: 38, height: 14, rx: 4, fill: n === '5V' ? '#e63946' : n === 'GND' ? '#2b2b2b' : n === '3V3' ? '#ff7a00' : '#f08c00', stroke: n === 'GND' ? '#666' : 'none', opacity: u ? 1 : .3}) +
      T(483, y + 3.5, n.replace(/^G(\d)/, 'GPIO$1'), {'font-size': 7.5}); });
  ESPR.forEach(([n, y]) => { const k = 'ESP.' + n, u = used.includes(k);
    s += pin(k, on(k), !u) + el('rect', {x: 538, y: y - 7, width: 38, height: 14, rx: 4, fill: '#f08c00', opacity: u ? 1 : .3}) +
      T(557, y + 3.5, n.replace(/^G(\d)/, 'GPIO$1'), {'font-size': 7.5}); });
  s += '</g>';
  if (has('tft')) s += '<g class="fadein">' + el('rect', {x: 330, y: 20, width: 280, height: 180, rx: 10, fill: '#1d4fa8'}) +
    el('rect', {x: 395, y: 32, width: 150, height: 120, rx: 6, fill: '#0b1020'}) +
    '<g transform="translate(470 92)"><ellipse cx="-24" cy="-6" rx="9" ry="12" fill="#7ef0c8"/><ellipse cx="24" cy="-6" rx="9" ry="12" fill="#7ef0c8"/><path d="M-10 16q10 9 20 0" stroke="#ff8fb8" stroke-width="4" fill="none" stroke-linecap="round"/></g>' +
    T(470, 166, 'ST7789 1,3" 240×240 (SPI)', {'font-size': 10, fill: '#cfe0ff'}) +
    TFTP.map(n => pin('TFT.' + n, on('TFT.' + n)) + T(P['TFT.' + n].x, 176, n, {'font-size': 7.5, fill: '#e0e7ff'})).join('') +
    T(345, 40, 'CS → GND', {'font-size': 8, fill: '#ffd3e4', 'text-anchor': 'start'}) + T(345, 50, '(modul 8 pin)', {'font-size': 7, fill: '#cfe0ff', 'text-anchor': 'start'}) + '</g>';
  if (has('ttp')) s += '<g class="fadein">' + el('rect', {x: 60, y: 488, width: 150, height: 100, rx: 8, fill: '#e11d48'}) +
    el('circle', {cx: 135, cy: 552, r: 26, fill: 'none', stroke: '#fecdd3', 'stroke-width': 3}) + T(135, 556, 'TTP223', {'font-size': 13, 'font-weight': 800}) +
    [['GND', 'TTP.GND'], ['I/O', 'TTP.IO'], ['VCC', 'TTP.VCC']].map(([n, k]) => pin(k, on(k)) + T(P[k].x, 518, n, {'font-size': 8})).join('') +
    el('rect', {x: 176, y: 570, width: 26, height: 12, rx: 3, fill: '#fde68a', stroke: '#92400e'}) + T(189, 579, 'A', {'font-size': 8, fill: '#92400e', 'font-weight': 800}) +
    T(170, 579, 'solder', {'font-size': 7, 'text-anchor': 'end', fill: '#ffe4e6'}) + '</g>';
  if (has('df')) s += '<g class="fadein">' + el('rect', {x: 820, y: 380, width: 180, height: 250, rx: 8, fill: '#0f172a'}) +
    el('rect', {x: 862, y: 395, width: 96, height: 64, rx: 5, fill: '#cbd5e1'}) + el('rect', {x: 872, y: 402, width: 76, height: 50, rx: 3, fill: '#94a3b8'}) +
    T(910, 432, 'microSD', {'font-size': 10, fill: '#1e293b'}) + T(910, 500, 'DFPlayer', {'font-size': 14, 'font-weight': 800}) + T(910, 516, 'Mini', {'font-size': 11}) +
    el('rect', {x: 888, y: 532, width: 44, height: 44, rx: 4, fill: '#1f2430', stroke: '#334155'}) +
    DFL.map(n => pin('DF.' + n, on('DF.' + n), !['VCC','RX','TX','SPK_1','SPK_2'].includes(n)) + T(846, P['DF.' + n].y + 3, n, {'font-size': 7.5, 'text-anchor': 'start', opacity: ['DAC_R','DAC_L','GND'].includes(n) ? .5 : 1})).join('') +
    DFR.map(n => { const k = 'DF.r' + n.trim(); return pin(k, on(k), !['BUSY','GND'].includes(n.trim())) + T(974, P[k].y + 3, n.trim(), {'font-size': 7.5, 'text-anchor': 'end', opacity: ['BUSY','GND'].includes(n.trim()) ? 1 : .5}); }).join('') + '</g>';
  if (has('cap')) s += '<g class="fadein">' + el('rect', {x: 866, y: 312, width: 28, height: 40, rx: 6, fill: '#2d3a8c'}) + el('rect', {x: 886, y: 312, width: 6, height: 40, fill: '#94a3b8'}) +
    T(900, 326, '100–470 µF', {'font-size': 9, fill: '#2d3a8c', 'text-anchor': 'start'}) + T(900, 338, '≥ 10 V', {'font-size': 8, fill: '#2d3a8c', 'text-anchor': 'start'}) +
    T(860, 322, '+', {'font-size': 11, fill: '#b91c1c', 'text-anchor': 'end'}) + pin('CAP.+', on('CAP.+')) + pin('CAP.-', on('CAP.-')) + '</g>';
  if (has('spk')) s += '<g class="fadein">' + el('circle', {cx: 700, cy: 672, r: 40, fill: '#1f1f1f'}) + el('circle', {cx: 700, cy: 672, r: 27, fill: '#3a3a3a'}) +
    el('circle', {cx: 700, cy: 672, r: 9, fill: '#111'}) + T(700, 624, 'Speaker 8 Ω 1–3 W', {fill: '#333', 'font-size': 10}) +
    pin('SPK.+', on('SPK.+')) + pin('SPK.-', on('SPK.-')) + '</g>';
  if (has('mpu')) s += '<g class="fadein">' + el('rect', {x: 470, y: 624, width: 186, height: 86, rx: 8, fill: '#1e3a8a'}) +
    el('rect', {x: 545, y: 662, width: 30, height: 30, rx: 3, fill: '#111827'}) + T(512, 690, 'MPU6050', {'font-size': 11, 'font-weight': 800}) + T(512, 702, 'opsional', {'font-size': 8, fill: '#bfdbfe'}) +
    MPUP.map((n, i) => pin('MPU.' + n, on('MPU.' + n), i > 3) + T(P['MPU.' + n].x, 652, n, {'font-size': 6.5, opacity: i > 3 ? .5 : 1})).join('') + '</g>';
  return s;
}
function dpath(c) { return 'M' + c.pts.map(p => p.join(' ')).join(' L'); }
function plen(c) { let L = 0; for (let i = 1; i < c.pts.length; i++) L += Math.hypot(c.pts[i][0] - c.pts[i - 1][0], c.pts[i][1] - c.pts[i - 1][1]); return Math.round(L); }
function resistor(r, col) {
  return '<g>' + el('rect', {x: r.x - 22, y: r.y - 7, width: 44, height: 14, rx: 6, fill: '#f5deb3', stroke: r.opt ? '#9ca3af' : '#8a6d1d', 'stroke-width': 1.2, 'stroke-dasharray': r.opt ? '3 2' : 'none'}) +
    [-12, -5, 2, 9].map((dx, i) => el('rect', {x: r.x + dx, y: r.y - 7, width: 3.5, height: 14, fill: ['#8b4513', '#111', '#e63946', '#d4a017'][i], opacity: r.opt ? .55 : 1})).join('') +
    T(r.x, r.opt ? r.y + 20 : r.y - 11, r.t, {'font-size': 8.5, fill: r.opt ? '#6b7280' : '#7a5c12'}) + '</g>';
}
function wires(list, anim) {
  let s = '';
  list.forEach((c, i) => {
    const d = dpath(c), col = NET[c.net], dash = c.dash ? '7 5' : 'none';
    s += el('path', {d, class: 'wire', stroke: '#fff', 'stroke-width': anim ? 7 : 6});
    if (anim) {
      const len = plen(c);
      s += el('path', {d, class: 'halo', stroke: col, 'stroke-width': 11});
      s += el('path', {d, class: 'wire draw', stroke: col, 'stroke-width': 3.4, style: `--len:${len};stroke-dasharray:${len};animation-delay:${(i * .15).toFixed(2)}s`});
      s += el('path', {d, class: 'wire flow2', stroke: '#fff', 'stroke-width': 1.4, opacity: .8, style: `animation-delay:${(i * .15).toFixed(2)}s`});
    } else s += el('path', {d, class: 'wire', stroke: col, 'stroke-width': 3, 'stroke-dasharray': dash});
    (c.j || []).forEach(p => s += el('circle', {cx: p[0], cy: p[1], r: 4.5, fill: col, stroke: '#fff', 'stroke-width': 1.5}));
    if (c.res) s += resistor(c.res, col);
  });
  return s;
}
// titik sambung GND di rel
const RAILJ = {tft: [[22,358]], ttp: [[22,440]], dfpwr: [[1040,578],[1040,725]], mpu: [[372,725]]};
function railDots(steps) {
  let s = '';
  steps.forEach(k => (RAILJ[k] || []).forEach(p => s += el('circle', {cx: p[0], cy: p[1], r: 4.5, fill: NET.GND, stroke: '#fff', 'stroke-width': 1.5})));
  return s;
}
const MB = {esp: [425, 265, 190, 300], tft: [320, 10, 300, 200], ttp: [50, 430, 170, 165], df: [810, 245, 260, 395], spk: [650, 625, 160, 115],
  mpu: [360, 575, 305, 145], cap: [790, 290, 150, 70]};
function fit(show, ws) {
  let x0 = 425, y0 = 265, x1 = 615, y1 = 565;
  const inc = (x, y) => { x0 = Math.min(x0, x); y0 = Math.min(y0, y); x1 = Math.max(x1, x); y1 = Math.max(y1, y); };
  show.forEach(k => { const b = MB[k]; if (b) { inc(b[0], b[1]); inc(b[0] + b[2], b[1] + b[3]); } });
  ws.forEach(c => c.pts.forEach(q => inc(q[0], q[1])));
  const m = 16; x0 -= m; y0 -= m; x1 += m; y1 += m;
  let W_ = x1 - x0, H = y1 - y0;
  if (W_ < H * 1.3) { const d = (H * 1.3 - W_) / 2; x0 -= d; W_ = H * 1.3; }
  return [x0, y0, W_, H].map(v => Math.round(v));
}
/* render(target, {show, cur:[stepId], prev:[stepIds], all, hi, full, title}) */
function svg(o) {
  const cur = W.filter(c => (o.cur || []).includes(c.step));
  const prev = o.all ? W : W.filter(c => (o.prev || []).includes(c.step));
  const hi = o.hi || [];
  const vb = o.full ? [0, 0, 1100, 745] : fit(o.show, cur.concat(prev));
  let s = `<svg viewBox="${vb.join(' ')}" xmlns="http://www.w3.org/2000/svg" font-family="Nunito,system-ui,sans-serif" role="img" aria-label="${o.title || 'Diagram kabel Mochi DFPlayer'}">` +
    '<style>.wire{fill:none;stroke-linecap:round;stroke-linejoin:round}.halo{fill:none;stroke-opacity:.25;stroke-linecap:round;stroke-linejoin:round}' +
    '.wire.draw{animation:drawin 1.1s ease-out both}.wire.flow2{stroke-dasharray:8 10;animation:flow2 1.2s linear infinite}' +
    '@keyframes drawin{from{stroke-dashoffset:var(--len)}to{stroke-dashoffset:0}}@keyframes flow2{to{stroke-dashoffset:-36}}' +
    '.pad-hi{animation:pulse 1.1s ease-in-out infinite}@keyframes pulse{0%,100%{stroke-opacity:1}50%{stroke-opacity:.3}}' +
    '.fadein{animation:fadein .6s ease both}@keyframes fadein{from{opacity:0}to{opacity:1}}' +
    '@media (prefers-reduced-motion:reduce){.wire.draw,.wire.flow2,.pad-hi,.fadein{animation:none}}</style>';
  s += el('rect', {x: vb[0], y: vb[1], width: vb[2], height: vb[3], fill: '#fffafc', rx: 14});
  s += `<g opacity="${o.all ? 1 : .33}">` + wires(prev, false) + railDots(o.all ? Object.keys(RAILJ) : (o.prev || [])) + '</g>';
  s += mods(o.show, hi);
  s += wires(cur, !o.static) + railDots(o.cur || []);
  if (o.all || (o.cur || []).includes('tft') || (o.prev || []).includes('tft')) s += T(30, 718, 'GND bersama', {fill: '#2b2b2b', 'font-size': 10, 'text-anchor': 'start'});
  if (o.caption) s += T(1090, 20, o.caption, {fill: '#8a7488', 'font-size': 11, 'text-anchor': 'end'});
  return s + '</svg>';
}
function nice(n) {
  if (n === 'RAIL') return 'GND bersama';
  const m = {'SPK.+': 'Speaker (+)', 'SPK.-': 'Speaker (−)', 'CAP.+': 'Kapasitor (+)', 'CAP.-': 'Kapasitor (−)', 'DF.rBUSY': 'DFPlayer BUSY', 'DF.rGND': 'DFPlayer GND', 'TTP.IO': 'TTP223 OUT (I/O)'};
  if (m[n]) return m[n];
  if (PINNAME[n]) return 'ESP ' + PINNAME[n];
  const [mod, p] = n.split('.');
  return ({TFT: 'Layar', DF: 'DFPlayer', TTP: 'TTP223', MPU: 'MPU6050'}[mod] || mod) + ' ' + p;
}
function connTable(step) {
  const l = W.filter(c => c.step === step);
  if (!l.length) return '';
  return '<table class="conn"><tr><th>Dari</th><th>Ke</th><th>Net</th></tr>' + l.map(c => {
    const a = c.b.startsWith('ESP') ? c.b : c.a, b = c.b.startsWith('ESP') ? c.a : c.b;
    return `<tr><td>${nice(a)}</td><td>${nice(b)}${c.res ? ` <small>(${c.res.t})</small>` : ''}${c.dash ? ' <small>(opsional)</small>' : ''}</td><td><span class="sw" style="background:${NET[c.net]}"></span>${NETNAME[c.net]}</td></tr>`;
  }).join('') + '</table>';
}
function legend(nets) { return nets.map(n => `<span><span class="sw" style="background:${NET[n]}"></span>${NETNAME[n]}</span>`).join(''); }
g.MochiWiring = {NET, NETNAME, W, PADS, svg, connTable, legend, ALL: ['esp', 'tft', 'ttp', 'df', 'cap', 'spk', 'mpu']};
})(window);
