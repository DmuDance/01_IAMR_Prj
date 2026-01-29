/**
 * STM32 RC Car Web Controller - Node.js
 * 
 * STM32 USART1 (블루투스) 설정: 9600 baud
 * 
 * 명령어:
 *   w - 전진, s - 후진, a - 좌회전, d - 우회전
 *   x - 정지, t - 자동모드, r - 서보 리셋
 */

const express = require('express');
const { SerialPort } = require('serialport');
const fs = require('fs');
const path = require('path');
const os = require('os');

const app = express();
app.use(express.json());

// ===== 설정 =====
const CONFIG_PATH = path.join(__dirname, 'config.json');

let config = {
    serial: { port: 'auto', baudRate: 9600 },
    server: { port: 3000 }
};

function loadConfig() {
    try {
        if (fs.existsSync(CONFIG_PATH)) {
            config = JSON.parse(fs.readFileSync(CONFIG_PATH, 'utf8'));
            console.log('📁 설정 로드: config.json');
        }
    } catch (err) {
        console.log('⚠️ 기본 설정 사용');
    }
}

function saveConfig() {
    try {
        fs.writeFileSync(CONFIG_PATH, JSON.stringify(config, null, 2));
    } catch (err) {}
}

function parseArgs() {
    const args = process.argv.slice(2);
    for (let i = 0; i < args.length; i++) {
        if ((args[i] === '--port' || args[i] === '-p') && args[i + 1]) config.serial.port = args[i + 1];
        if ((args[i] === '--baud' || args[i] === '-b') && args[i + 1]) config.serial.baudRate = parseInt(args[i + 1]);
        if ((args[i] === '--server-port' || args[i] === '-s') && args[i + 1]) config.server.port = parseInt(args[i + 1]);
    }
}

loadConfig();
parseArgs();

// ===== 상태 =====
let serialPort = null;
let status = {
    direction: 'STOP',
    mode: 'MANUAL',
    state: 'IDLE',
    connected: false,
    port: '',
    baudRate: config.serial.baudRate
};

// 레이더 데이터 (각도별 거리)
let radarData = {};
let currentAngle = 90;
let currentDistance = 0;
let minAngle = 90;
let minDistance = 999;

// ===== 시리얼 =====
async function listPorts() {
    try { return await SerialPort.list(); } catch (err) { return []; }
}

async function findPort() {
    const ports = await listPorts();
    if (ports.length === 0) return null;
    const target = ports.find(p => {
        const info = ((p.manufacturer || '') + (p.friendlyName || '')).toLowerCase();
        return ['serial', 'uart', 'ch340', 'cp210', 'ftdi', 'usb', 'bluetooth'].some(k => info.includes(k));
    });
    return target ? target.path : ports[0].path;
}

async function connect(portPath, baudRate) {
    if (serialPort) {
        try {
            if (serialPort.isOpen) {
                await new Promise(resolve => serialPort.close(() => resolve()));
                await new Promise(resolve => setTimeout(resolve, 500));
            }
        } catch (e) {}
        serialPort = null;
    }
    
    if (!portPath || portPath === 'auto') {
        portPath = await findPort();
        if (!portPath) { status.connected = false; return false; }
    }
    
    baudRate = baudRate || config.serial.baudRate;
    
    return new Promise(resolve => {
        try {
            serialPort = new SerialPort({ path: portPath, baudRate: baudRate, autoOpen: false });
            
            serialPort.on('error', err => { console.error('시리얼 오류:', err.message); status.connected = false; });
            serialPort.on('close', () => { status.connected = false; });
            
            let buffer = '';
            serialPort.on('data', data => {
                buffer += data.toString();
                const lines = buffer.split('\r\n');
                buffer = lines.pop();
                
                lines.forEach(line => {
                    if (line.trim()) {
                        console.log('📥 STM32:', line);
                        parseSTM32Response(line);
                    }
                });
            });
            
            serialPort.open(err => {
                if (err) {
                    console.error('❌ 연결 실패:', err.message);
                    status.connected = false;
                    serialPort = null;
                    resolve(false);
                } else {
                    console.log(`✅ 연결: ${portPath} @ ${baudRate} baud`);
                    status.connected = true;
                    status.port = portPath;
                    status.baudRate = baudRate;
                    config.serial.port = portPath;
                    config.serial.baudRate = baudRate;
                    saveConfig();
                    resolve(true);
                }
            });
        } catch (err) {
            status.connected = false;
            resolve(false);
        }
    });
}

// STM32 응답 파싱
function parseSTM32Response(line) {
    // STATE:SCAN | angle= 30 | dist= 45 cm
    const stateMatch = line.match(/STATE:(\w+)/);
    if (stateMatch) {
        status.state = stateMatch[1];
    }
    
    // angle 파싱
    const angleMatch = line.match(/angle\s*=\s*(\d+)/);
    if (angleMatch) {
        currentAngle = parseInt(angleMatch[1]);
    }
    
    // distance 파싱
    const distMatch = line.match(/dist\s*=\s*(\d+)/);
    if (distMatch) {
        currentDistance = parseInt(distMatch[1]);
        // 레이더 데이터 저장
        if (currentAngle >= 30 && currentAngle <= 150) {
            radarData[currentAngle] = currentDistance;
        }
    }
    
    // min_angle, min_dist 파싱 (DECIDE 상태)
    const minAngleMatch = line.match(/min_angle\s*=\s*(\d+)/);
    const minDistMatch = line.match(/min_dist\s*=\s*(\d+)/);
    if (minAngleMatch) minAngle = parseInt(minAngleMatch[1]);
    if (minDistMatch) minDistance = parseInt(minDistMatch[1]);
    
    // 모드 파싱
    if (line.includes('AUTO MODE')) {
        status.mode = 'AUTO';
        status.direction = 'AUTO';
    } else if (line.includes('MANUAL:')) {
        status.mode = 'MANUAL';
        if (line.includes('FORWARD')) status.direction = 'FORWARD';
        else if (line.includes('BACKWARD')) status.direction = 'BACKWARD';
        else if (line.includes('LEFT')) status.direction = 'LEFT';
        else if (line.includes('RIGHT')) status.direction = 'RIGHT';
    } else if (line.includes('STOP')) {
        status.direction = 'STOP';
        status.mode = 'MANUAL';
    }
}

async function disconnect() {
    if (serialPort) {
        try { if (serialPort.isOpen) await new Promise(resolve => serialPort.close(() => resolve())); } catch (e) {}
        serialPort = null;
        status.connected = false;
        status.port = '';
    }
}

function send(cmd) {
    if (!serialPort || !serialPort.isOpen) { status.connected = false; return false; }
    const lowerCmd = cmd.toLowerCase();
    console.log('📤 전송:', lowerCmd);
    serialPort.write(lowerCmd, err => { if (err) console.log('전송 오류:', err.message); });
    return true;
}

// ===== HTML =====
const HTML = `
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>RC Car Controller</title>
    <style>
        * { margin:0; padding:0; box-sizing:border-box; user-select:none; -webkit-tap-highlight-color:transparent; }
        :root { --bg:#0f172a; --card:#1e293b; --border:#334155; --text:#f1f5f9; --dim:#64748b; --blue:#3b82f6; --green:#22c55e; --red:#ef4444; --purple:#8b5cf6; --cyan:#06b6d4; --orange:#f97316; --yellow:#eab308; }
        html,body { height:100%; overflow-x:hidden; }
        body { font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif; background:var(--bg); color:var(--text); display:flex; flex-direction:column; align-items:center; padding:10px; gap:10px; }
        
        .header { display:flex; align-items:center; gap:12px; }
        h1 { font-size:1.4rem; background:linear-gradient(135deg,#60a5fa,#a78bfa); -webkit-background-clip:text; -webkit-text-fill-color:transparent; }
        .settings-btn { background:var(--card); border:1px solid var(--border); color:var(--text); padding:6px 10px; border-radius:6px; cursor:pointer; font-size:1rem; }
        
        /* 상태 표시 */
        .status-bar { display:flex; gap:15px; background:var(--card); padding:8px 15px; border-radius:10px; border:1px solid var(--border); flex-wrap:wrap; justify-content:center; }
        .status-item { text-align:center; min-width:50px; }
        .status-label { font-size:0.55rem; color:var(--dim); text-transform:uppercase; }
        .status-value { font-size:0.85rem; font-weight:600; }
        #direction { color:#60a5fa; }
        #mode { color:var(--yellow); }
        #state { color:var(--cyan); }
        #connection.on { color:var(--green); }
        #connection.off { color:var(--red); }
        
        /* 레이더 컨테이너 */
        .radar-container { background:var(--card); border-radius:12px; padding:10px; border:1px solid var(--border); }
        .radar-title { text-align:center; font-size:0.75rem; color:var(--dim); margin-bottom:5px; }
        #radarCanvas { display:block; }
        
        /* 레이더 정보 */
        .radar-info { display:flex; justify-content:center; gap:20px; margin-top:8px; font-size:0.75rem; }
        .radar-info-item { text-align:center; }
        .radar-info-label { color:var(--dim); font-size:0.6rem; }
        .radar-info-value { font-weight:600; }
        .radar-info-value.angle { color:var(--cyan); }
        .radar-info-value.dist { color:var(--green); }
        .radar-info-value.min { color:var(--orange); }
        
        /* 모드 버튼 */
        .mode-btns { display:flex; gap:8px; }
        .mode-btn { padding:8px 16px; border:none; border-radius:8px; font-size:0.85rem; cursor:pointer; font-weight:500; }
        .mode-btn.auto { background:var(--yellow); color:#000; }
        .mode-btn.stop { background:var(--red); color:#fff; }
        
        /* 컨트롤러 */
        .controller { display:flex; flex-direction:column; align-items:center; gap:5px; }
        .row { display:flex; gap:5px; }
        .btn { width:65px; height:65px; border:none; border-radius:12px; font-size:1.4rem; cursor:pointer; display:flex; align-items:center; justify-content:center; background:var(--card); color:var(--text); border:2px solid var(--border); transition:transform 0.1s; }
        .btn:active,.btn.active { transform:scale(0.95); }
        .btn-up { background:linear-gradient(145deg,#3b82f6,#2563eb); border-color:#3b82f6; }
        .btn-down { background:linear-gradient(145deg,#8b5cf6,#7c3aed); border-color:#8b5cf6; }
        .btn-left,.btn-right { background:linear-gradient(145deg,#06b6d4,#0891b2); border-color:#06b6d4; }
        .btn-stop { background:linear-gradient(145deg,#ef4444,#dc2626); border-color:#ef4444; }
        .spacer { width:65px; height:65px; }
        
        .hint { color:var(--dim); font-size:0.6rem; text-align:center; }
        .key { display:inline-block; background:var(--card); padding:1px 4px; border-radius:3px; font-family:monospace; border:1px solid var(--border); margin:0 1px; font-size:0.55rem; }
        
        /* 모달 */
        .modal { display:none; position:fixed; top:0; left:0; width:100%; height:100%; background:rgba(0,0,0,0.7); justify-content:center; align-items:center; z-index:100; }
        .modal.show { display:flex; }
        .modal-content { background:var(--card); border-radius:16px; padding:20px; width:90%; max-width:360px; border:1px solid var(--border); max-height:85vh; overflow-y:auto; }
        .modal-header { display:flex; justify-content:space-between; align-items:center; margin-bottom:15px; }
        .modal-title { font-size:1.1rem; font-weight:600; }
        .close-btn { background:none; border:none; color:var(--dim); font-size:1.4rem; cursor:pointer; }
        .form-group { margin-bottom:12px; }
        .form-label { display:block; color:var(--dim); font-size:0.75rem; margin-bottom:4px; }
        .form-select { width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:var(--bg); color:var(--text); font-size:0.9rem; }
        .btn-row { display:flex; gap:8px; margin-top:14px; }
        .btn-primary { flex:1; padding:10px; border:none; border-radius:8px; background:var(--blue); color:white; font-size:0.9rem; cursor:pointer; }
        .btn-secondary { flex:1; padding:10px; border:1px solid var(--border); border-radius:8px; background:var(--card); color:var(--text); font-size:0.9rem; cursor:pointer; }
        .btn-danger { background:var(--red); }
        .port-list { max-height:120px; overflow-y:auto; margin-bottom:8px; }
        .port-item { padding:8px; background:var(--bg); border-radius:6px; margin-bottom:4px; cursor:pointer; border:1px solid var(--border); font-size:0.85rem; }
        .port-item:hover { border-color:var(--blue); }
        .port-item.selected { border-color:var(--green); background:rgba(34,197,94,0.1); }
        .port-name { font-weight:500; }
        .port-desc { font-size:0.7rem; color:var(--dim); }
        .refresh-btn { background:var(--card); border:1px solid var(--border); color:var(--text); padding:4px 8px; border-radius:5px; cursor:pointer; font-size:0.75rem; }
        .status-badge { display:inline-block; padding:3px 8px; border-radius:10px; font-size:0.7rem; }
        .status-badge.connected { background:rgba(34,197,94,0.2); color:var(--green); }
        .status-badge.disconnected { background:rgba(239,68,68,0.2); color:var(--red); }
        
        /* 반응형 - 큰 화면 */
        @media (min-width:600px) {
            body { padding:15px; gap:12px; }
            h1 { font-size:1.6rem; }
            .btn { width:80px; height:80px; font-size:1.8rem; }
            .spacer { width:80px; height:80px; }
            .row { gap:8px; }
            .controller { gap:8px; }
            #radarCanvas { width:350px; height:200px; }
        }
        
        /* 가로 모드 */
        @media (max-height:450px) and (orientation:landscape) {
            body { flex-direction:row; flex-wrap:wrap; justify-content:space-around; align-content:center; padding:5px 15px; gap:8px; }
            .header { display:none; }
            .status-bar { position:fixed; top:3px; left:50%; transform:translateX(-50%); padding:4px 12px; gap:10px; font-size:0.8rem; }
            .status-label { display:none; }
            .radar-container { order:1; }
            .controller { order:2; }
            .mode-btns { position:fixed; bottom:3px; left:50%; transform:translateX(-50%); }
            .hint { display:none; }
            .btn { width:55px; height:55px; font-size:1.2rem; }
            .spacer { width:55px; height:55px; }
            #radarCanvas { width:280px; height:160px; }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>🚗 RC Car</h1>
        <button class="settings-btn" onclick="openSettings()">⚙️</button>
    </div>
    
    <div class="status-bar">
        <div class="status-item">
            <div class="status-label">State</div>
            <div class="status-value" id="state">IDLE</div>
        </div>
        <div class="status-item">
            <div class="status-label">Direction</div>
            <div class="status-value" id="direction">STOP</div>
        </div>
        <div class="status-item">
            <div class="status-label">Mode</div>
            <div class="status-value" id="mode">MANUAL</div>
        </div>
        <div class="status-item">
            <div class="status-label">Status</div>
            <div class="status-value" id="connection">●</div>
        </div>
    </div>
    
    <div class="radar-container">
        <div class="radar-title">📡 Ultrasonic Radar (30° ~ 150°)</div>
        <canvas id="radarCanvas" width="320" height="180"></canvas>
        <div class="radar-info">
            <div class="radar-info-item">
                <div class="radar-info-label">ANGLE</div>
                <div class="radar-info-value angle" id="currentAngle">90°</div>
            </div>
            <div class="radar-info-item">
                <div class="radar-info-label">DISTANCE</div>
                <div class="radar-info-value dist" id="currentDist">0 cm</div>
            </div>
            <div class="radar-info-item">
                <div class="radar-info-label">MIN DIST</div>
                <div class="radar-info-value min" id="minDist">999 cm</div>
            </div>
            <div class="radar-info-item">
                <div class="radar-info-label">MIN ANGLE</div>
                <div class="radar-info-value min" id="minAngle">90°</div>
            </div>
        </div>
    </div>
    
    <div class="mode-btns">
        <button class="mode-btn auto" onclick="sendCmd('t')">🤖 AUTO</button>
        <button class="mode-btn stop" onclick="sendCmd('x')"> ♻️RESET</button>
    </div>
    
    <div class="controller">
        <div class="row">
            <div class="spacer"></div>
            <button class="btn btn-up" id="btnW" data-cmd="w">▲</button>
            <div class="spacer"></div>
        </div>
        <div class="row">
            <button class="btn btn-left" id="btnA" data-cmd="a">◀</button>
            <button class="btn btn-stop" id="btnX" data-cmd="x">✕</button>
            <button class="btn btn-right" id="btnD" data-cmd="d">▶</button>
        </div>
        <div class="row">
            <div class="spacer"></div>
            <button class="btn btn-down" id="btnS" data-cmd="s">▼</button>
            <div class="spacer"></div>
        </div>
    </div>
    
    <div class="hint">
        <span class="key">W</span><span class="key">A</span><span class="key">S</span><span class="key">D</span> 이동
        <span class="key">X</span> 정지
        <span class="key">T</span> 자동
    </div>

    <div class="modal" id="settingsModal">
        <div class="modal-content">
            <div class="modal-header">
                <span class="modal-title">⚙️ 시리얼 설정</span>
                <button class="close-btn" onclick="closeSettings()">×</button>
            </div>
            <div class="form-group">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:5px;">
                    <label class="form-label" style="margin:0;">포트 목록</label>
                    <button class="refresh-btn" onclick="refreshPorts()">🔄 새로고침</button>
                </div>
                <div class="port-list" id="portList"></div>
            </div>
            <div class="form-group">
                <label class="form-label">Baud Rate</label>
                <select class="form-select" id="baudRate">
                    <option value="9600" selected>9600 (STM32 기본)</option>
                    <option value="115200">115200</option>
                </select>
            </div>
            <div class="form-group">
                <label class="form-label">현재 상태</label>
                <div>
                    <span class="status-badge" id="modalStatus">연결 안됨</span>
                    <span id="modalPort" style="margin-left:8px;color:var(--dim);font-size:0.8rem;"></span>
                </div>
            </div>
            <div class="btn-row">
                <button class="btn-primary" onclick="connectPort()">연결</button>
                <button class="btn-secondary btn-danger" onclick="disconnectPort()">해제</button>
            </div>
        </div>
    </div>

    <script>
        let selectedPort = null;
        let radarData = {};
        
        // 레이더 캔버스
        const canvas = document.getElementById('radarCanvas');
        const ctx = canvas.getContext('2d');
        
        function drawRadar(data, currentAngle, currentDist, minAngle, minDist) {
            const W = canvas.width;
            const H = canvas.height;
            const cx = W / 2;
            const cy = H - 10;
            const maxR = H - 20;
            const maxDist = 200; // 최대 표시 거리 (cm)
            
            // 배경
            ctx.fillStyle = '#0f172a';
            ctx.fillRect(0, 0, W, H);
            
            // 거리 원 (그리드)
            ctx.strokeStyle = '#334155';
            ctx.lineWidth = 1;
            for (let r = 1; r <= 4; r++) {
                const radius = (maxR / 4) * r;
                ctx.beginPath();
                ctx.arc(cx, cy, radius, Math.PI, 0, false);
                ctx.stroke();
                
                // 거리 라벨
                ctx.fillStyle = '#64748b';
                ctx.font = '10px sans-serif';
                ctx.textAlign = 'center';
                ctx.fillText((maxDist / 4 * r) + 'cm', cx, cy - radius + 12);
            }
            
            // 각도 선
            ctx.strokeStyle = '#334155';
            for (let deg = 30; deg <= 150; deg += 30) {
                const rad = deg * Math.PI / 180;
                ctx.beginPath();
                ctx.moveTo(cx, cy);
                ctx.lineTo(cx - Math.cos(rad) * maxR, cy - Math.sin(rad) * maxR);
                ctx.stroke();
                
                // 각도 라벨
                ctx.fillStyle = '#64748b';
                ctx.font = '9px sans-serif';
                const lx = cx - Math.cos(rad) * (maxR + 12);
                const ly = cy - Math.sin(rad) * (maxR + 12);
                ctx.fillText(deg + '°', lx, ly + 3);
            }
            
            // 스캔 데이터 포인트 및 영역
            const angles = Object.keys(data).map(Number).sort((a, b) => a - b);
            
            if (angles.length > 1) {
                // 채워진 영역
                ctx.beginPath();
                ctx.moveTo(cx, cy);
                angles.forEach((deg, i) => {
                    const dist = Math.min(data[deg], maxDist);
                    const r = (dist / maxDist) * maxR;
                    const rad = deg * Math.PI / 180;
                    const x = cx - Math.cos(rad) * r;
                    const y = cy - Math.sin(rad) * r;
                    if (i === 0) ctx.lineTo(x, y);
                    else ctx.lineTo(x, y);
                });
                ctx.closePath();
                ctx.fillStyle = 'rgba(34, 197, 94, 0.2)';
                ctx.fill();
                ctx.strokeStyle = '#22c55e';
                ctx.lineWidth = 2;
                ctx.stroke();
            }
            
            // 각 포인트
            angles.forEach(deg => {
                const dist = Math.min(data[deg], maxDist);
                const r = (dist / maxDist) * maxR;
                const rad = deg * Math.PI / 180;
                const x = cx - Math.cos(rad) * r;
                const y = cy - Math.sin(rad) * r;
                
                ctx.beginPath();
                ctx.arc(x, y, 4, 0, Math.PI * 2);
                ctx.fillStyle = dist < 30 ? '#ef4444' : dist < 60 ? '#f97316' : '#22c55e';
                ctx.fill();
            });
            
            // 현재 스캔 위치 (큰 원)
            if (currentAngle >= 30 && currentAngle <= 150) {
                const dist = Math.min(currentDist, maxDist);
                const r = (dist / maxDist) * maxR;
                const rad = currentAngle * Math.PI / 180;
                const x = cx - Math.cos(rad) * r;
                const y = cy - Math.sin(rad) * r;
                
                // 스캔 라인
                ctx.beginPath();
                ctx.moveTo(cx, cy);
                ctx.lineTo(cx - Math.cos(rad) * maxR, cy - Math.sin(rad) * maxR);
                ctx.strokeStyle = 'rgba(96, 165, 250, 0.5)';
                ctx.lineWidth = 2;
                ctx.stroke();
                
                // 현재 포인트
                ctx.beginPath();
                ctx.arc(x, y, 6, 0, Math.PI * 2);
                ctx.fillStyle = '#60a5fa';
                ctx.fill();
                ctx.strokeStyle = '#fff';
                ctx.lineWidth = 2;
                ctx.stroke();
            }
            
            // 최소 거리 위치 표시
            if (minAngle >= 30 && minAngle <= 150 && minDist < 999) {
                const dist = Math.min(minDist, maxDist);
                const r = (dist / maxDist) * maxR;
                const rad = minAngle * Math.PI / 180;
                const x = cx - Math.cos(rad) * r;
                const y = cy - Math.sin(rad) * r;
                
                ctx.beginPath();
                ctx.arc(x, y, 8, 0, Math.PI * 2);
                ctx.strokeStyle = '#f97316';
                ctx.lineWidth = 3;
                ctx.stroke();
                
                // 최소 거리 라벨
                ctx.fillStyle = '#f97316';
                ctx.font = 'bold 10px sans-serif';
                ctx.textAlign = 'center';
                ctx.fillText('MIN', x, y - 12);
            }
            
            // 로봇 위치 표시
            ctx.beginPath();
            ctx.arc(cx, cy, 8, 0, Math.PI * 2);
            ctx.fillStyle = '#8b5cf6';
            ctx.fill();
            ctx.fillStyle = '#fff';
            ctx.font = 'bold 8px sans-serif';
            ctx.textAlign = 'center';
            ctx.fillText('🚗', cx, cy + 3);
        }
        
        function sendCmd(cmd) {
            fetch('/api/cmd?c=' + cmd).then(r => r.json()).then(updateUI).catch(() => setConn(false));
        }
        
        function updateUI(data) {
            document.getElementById('state').textContent = data.state;
            document.getElementById('direction').textContent = data.direction;
            document.getElementById('mode').textContent = data.mode;
            setConn(data.connected);
            
            // 레이더 데이터 업데이트
            if (data.radar) radarData = data.radar;
            document.getElementById('currentAngle').textContent = data.currentAngle + '°';
            document.getElementById('currentDist').textContent = data.currentDistance + ' cm';
            document.getElementById('minAngle').textContent = data.minAngle + '°';
            document.getElementById('minDist').textContent = data.minDistance + ' cm';
            
            drawRadar(radarData, data.currentAngle, data.currentDistance, data.minAngle, data.minDistance);
        }
        
        function setConn(on) {
            const el = document.getElementById('connection');
            el.textContent = on ? '●' : '○';
            el.className = 'status-value ' + (on ? 'on' : 'off');
        }
        
        // 버튼 이벤트
        document.querySelectorAll('.btn[data-cmd]').forEach(btn => {
            const cmd = btn.dataset.cmd;
            ['touchstart','mousedown'].forEach(e => btn.addEventListener(e, ev => { ev.preventDefault(); btn.classList.add('active'); sendCmd(cmd); }));
            ['touchend','mouseup','mouseleave'].forEach(e => btn.addEventListener(e, () => { if(btn.classList.contains('active')) { btn.classList.remove('active'); if(cmd!=='x') sendCmd('x'); }}));
        });
        
        // 키보드
        const keyMap = { w:'w',W:'w',ArrowUp:'w', s:'s',S:'s',ArrowDown:'s', a:'a',A:'a',ArrowLeft:'a', d:'d',D:'d',ArrowRight:'d', x:'x',X:'x',' ':'x',Escape:'x', t:'t',T:'t', r:'r',R:'r' };
        const pressed = new Set();
        document.addEventListener('keydown', e => {
            if(keyMap[e.key] && !pressed.has(e.key)) {
                e.preventDefault(); pressed.add(e.key);
                sendCmd(keyMap[e.key]);
                const btn = document.getElementById('btn'+keyMap[e.key].toUpperCase());
                if(btn) btn.classList.add('active');
            }
        });
        document.addEventListener('keyup', e => {
            if(keyMap[e.key]) {
                pressed.delete(e.key);
                const btn = document.getElementById('btn'+keyMap[e.key].toUpperCase());
                if(btn) btn.classList.remove('active');
                if(['w','a','s','d'].includes(keyMap[e.key]) && pressed.size===0) sendCmd('x');
            }
        });
        
        // 설정
        function openSettings() { document.getElementById('settingsModal').classList.add('show'); refreshPorts(); updateModal(); }
        function closeSettings() { document.getElementById('settingsModal').classList.remove('show'); }
        
        function refreshPorts() {
            const list = document.getElementById('portList');
            list.innerHTML = '<div style="color:var(--dim);text-align:center;padding:12px;">로딩...</div>';
            fetch('/api/ports').then(r=>r.json()).then(ports => {
                if(!ports.length) { list.innerHTML = '<div style="color:var(--dim);text-align:center;padding:12px;">포트 없음</div>'; return; }
                list.innerHTML = ports.map(p => '<div class="port-item" onclick="selPort(this,\\''+p.path+'\\')"><div class="port-name">'+p.path+'</div><div class="port-desc">'+(p.manufacturer||p.friendlyName||'')+'</div></div>').join('');
            });
        }
        
        function selPort(el, port) {
            document.querySelectorAll('.port-item').forEach(i=>i.classList.remove('selected'));
            el.classList.add('selected');
            selectedPort = port;
        }
        
        function connectPort() {
            if(!selectedPort) { alert('포트를 선택하세요'); return; }
            fetch('/api/connect', { method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({port:selectedPort, baudRate:parseInt(document.getElementById('baudRate').value)}) })
            .then(r=>r.json()).then(d => { updateModal(); if(d.success) alert('연결 성공'); else alert('연결 실패: '+d.error); });
        }
        
        function disconnectPort() {
            fetch('/api/disconnect',{method:'POST'}).then(()=>{ updateModal(); });
        }
        
        function updateModal() {
            fetch('/api/status').then(r=>r.json()).then(data => {
                const badge = document.getElementById('modalStatus');
                const info = document.getElementById('modalPort');
                if(data.connected) { badge.textContent='연결됨'; badge.className='status-badge connected'; info.textContent=data.port+' @ '+data.baudRate; }
                else { badge.textContent='연결 안됨'; badge.className='status-badge disconnected'; info.textContent=''; }
            });
        }
        
        document.getElementById('settingsModal').addEventListener('click', e => { if(e.target.id==='settingsModal') closeSettings(); });
        
        // 폴링 (빠른 업데이트)
        setInterval(() => fetch('/api/status').then(r=>r.json()).then(updateUI).catch(()=>setConn(false)), 200);
        
        // 초기화
        drawRadar({}, 90, 0, 90, 999);
        fetch('/api/status').then(r=>r.json()).then(updateUI);
    </script>
</body>
</html>
`;

// ===== 라우트 =====
app.get('/', (req, res) => res.send(HTML));
app.get('/api/cmd', (req, res) => { const c = req.query.c; if(c) send(c); res.json(getFullStatus()); });
app.get('/api/status', (req, res) => res.json(getFullStatus()));
app.get('/api/ports', async (req, res) => res.json(await listPorts()));
app.post('/api/connect', async (req, res) => {
    const { port, baudRate } = req.body;
    const ok = await connect(port, baudRate);
    res.json({ success: ok, port: status.port, error: ok ? null : '연결 실패' });
});
app.post('/api/disconnect', async (req, res) => { await disconnect(); res.json({ success: true }); });

function getFullStatus() {
    return {
        ...status,
        radar: radarData,
        currentAngle: currentAngle,
        currentDistance: currentDistance,
        minAngle: minAngle,
        minDistance: minDistance
    };
}

// ===== 시작 =====
async function start() {
    console.log('='.repeat(50));
    console.log('   STM32 RC Car Web Controller + Radar');
    console.log('='.repeat(50));
    console.log(`📋 설정: ${config.serial.port} @ ${config.serial.baudRate} baud`);
    
    await connect(config.serial.port, config.serial.baudRate);
    
    app.listen(config.server.port, '0.0.0.0', () => {
        const nets = os.networkInterfaces();
        let ip = 'localhost';
        for (const name of Object.keys(nets)) {
            for (const net of nets[name]) {
                if (net.family === 'IPv4' && !net.internal) { ip = net.address; break; }
            }
        }
        console.log('🌐 서버: http://localhost:' + config.server.port);
        console.log('📱 모바일: http://' + ip + ':' + config.server.port);
        console.log('='.repeat(50));
    });
}

start();
