/**
 * Honda CL250 Telemetry - Advanced Mobile App Logic & Trip Engine
 */

// BLE Definitions
const BLE_SERVICE_UUID           = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const BLE_CHARACTERISTIC_TX_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const BLE_CHARACTERISTIC_RX_UUID = "828919fe-e41c-40ee-b4c6-2c974c2d3345";

// Main App State
const appState = {
    connected: false,
    connType: "NONE", // "BLE", "WIFI", or "NONE"
    demoMode: false,
    bleDevice: null,
    rxCharacteristic: null,
    wifiPollInterval: null,
    demoInterval: null,
    telemetry: {
        rpm: 0,
        speed: 0,
        coolantTemp: 0,
        throttlePos: 0,
        batteryVolt: 0.0,
        leanAngle: 0.0,
        maxLeanRight: 0.0,
        maxLeanLeft: 0.0
    }
};

// Task 1: Trip Recording Engine State
const tripState = {
    recording: false,
    startTime: null,
    timerInterval: null,
    elapsedSec: 0,
    maxSpeed: 0,
    maxRpm: 0,
    maxLeanLeft: 0,
    maxLeanRight: 0,
    snapshots: []
};

// UI Elements
const connectBtn     = document.getElementById("connectBtn");
const statusDot      = document.getElementById("statusDot");
const statusText     = document.getElementById("statusText");

const speedVal       = document.getElementById("speedVal");
const rpmVal         = document.getElementById("rpmVal");
const rpmGaugeArc    = document.getElementById("rpmGaugeArc");

const leanAngleVal   = document.getElementById("leanAngleVal");
const maxLeftVal     = document.getElementById("maxLeftVal");
const maxRightVal    = document.getElementById("maxRightVal");
const bikeChassisSvg = document.getElementById("bikeChassisSvg");

const coolantVal     = document.getElementById("coolantVal");
const throttleVal    = document.getElementById("throttleVal");
const batteryVal     = document.getElementById("batteryVal");

const songInput      = document.getElementById("songInput");
const artistInput    = document.getElementById("artistInput");
const navDistInput   = document.getElementById("navDistInput");
const syncBtn        = document.getElementById("syncBtn");

// Trip UI Elements
const tripTimerVal   = document.getElementById("tripTimerVal");
const tripMaxSpeed   = document.getElementById("tripMaxSpeed");
const tripMaxRpm     = document.getElementById("tripMaxRpm");
const tripPeakLeft   = document.getElementById("tripPeakLeft");
const tripPeakRight  = document.getElementById("tripPeakRight");

const startTripBtn   = document.getElementById("startTripBtn");
const resetTripBtn   = document.getElementById("resetTripBtn");
const exportCsvBtn   = document.getElementById("exportCsvBtn");

// Mode Switchers
const trackModeBtn   = document.getElementById("trackModeBtn");
const urbanModeBtn   = document.getElementById("urbanModeBtn");
const demoModeBtn    = document.getElementById("demoModeBtn");

// Event Listeners
connectBtn.addEventListener("click", toggleBLEConnection);
syncBtn.addEventListener("click", syncTelematicsToBLE);

startTripBtn.addEventListener("click", toggleTripRecording);
resetTripBtn.addEventListener("click", resetTripSession);
exportCsvBtn.addEventListener("click", exportTripCSV);

trackModeBtn.addEventListener("click", () => setDisplayMode("track"));
urbanModeBtn.addEventListener("click", () => setDisplayMode("urban"));
demoModeBtn.addEventListener("click", () => setDisplayMode("demo"));

initRpmGauge();

/**
 * Task 4: Multi-Transport Communication Engine (BLE + Wi-Fi Fallback)
 */
async function toggleBLEConnection() {
    if (appState.connected) {
        disconnectTransport();
        return;
    }

    try {
        statusText.innerText = "Scanning BLE...";
        const device = await navigator.bluetooth.requestDevice({
            filters: [{ namePrefix: "Honda-CL250" }],
            optionalServices: [BLE_SERVICE_UUID]
        });

        appState.bleDevice = device;
        device.addEventListener("gattserverdisconnected", onBLEDisconnected);

        statusText.innerText = "Connecting BLE...";
        const server = await device.gatt.connect();

        const service = await server.getPrimaryService(BLE_SERVICE_UUID);
        const txChar = await service.getCharacteristic(BLE_CHARACTERISTIC_TX_UUID);
        await txChar.startNotifications();
        txChar.addEventListener("characteristicvaluechanged", handleTelemetryNotification);

        try {
            appState.rxCharacteristic = await service.getCharacteristic(BLE_CHARACTERISTIC_RX_UUID);
        } catch (e) {
            console.warn("RX Characteristic unavailable", e);
        }

        setConnectedState("BLE");
    } catch (error) {
        console.warn("BLE Failed. Attempting Wi-Fi SoftAP fallback...", error);
        startWifiPollingFallback();
    }
}

function startWifiPollingFallback() {
    statusText.innerText = "Checking Wi-Fi AP...";
    
    // Poll http://192.168.4.1/api/telemetry
    appState.wifiPollInterval = setInterval(async () => {
        try {
            const res = await fetch("http://192.168.4.1/api/telemetry");
            if (res.ok) {
                const data = await res.json();
                appState.telemetry.rpm          = data.rpm || 0;
                appState.telemetry.speed        = data.speed || 0;
                appState.telemetry.coolantTemp  = data.coolantTemp || 0;
                appState.telemetry.throttlePos  = data.throttlePos || 0;
                appState.telemetry.batteryVolt  = data.batteryVoltage || 0;
                appState.telemetry.leanAngle    = data.leanAngle || 0;
                appState.telemetry.maxLeanLeft  = data.maxLeanLeft || 0;
                appState.telemetry.maxLeanRight = data.maxLeanRight || 0;

                if (appState.connType !== "WIFI") {
                    setConnectedState("WIFI");
                }
                updateUI();
            }
        } catch (e) {
            if (appState.connType === "WIFI") {
                disconnectTransport();
            }
        }
    }, 200); // 5Hz polling
}

function disconnectTransport() {
    if (appState.bleDevice && appState.bleDevice.gatt.connected) {
        appState.bleDevice.gatt.disconnect();
    }
    if (appState.wifiPollInterval) {
        clearInterval(appState.wifiPollInterval);
        appState.wifiPollInterval = null;
    }
    setConnectedState("NONE");
}

function onBLEDisconnected() {
    setConnectedState("NONE");
    startWifiPollingFallback(); // Fallback to Wi-Fi if available
}

function setConnectedState(type) {
    appState.connType = type;
    appState.connected = (type !== "NONE");

    statusDot.className = "status-dot";
    if (type === "BLE") {
        statusDot.classList.add("connected");
        statusText.innerText = "Connected (BLE)";
        connectBtn.innerText = "Disconnect";
        disableDemoMode();
    } else if (type === "WIFI") {
        statusDot.classList.add("wifi");
        statusText.innerText = "Connected (Wi-Fi)";
        connectBtn.innerText = "Disconnect";
        disableDemoMode();
    } else {
        statusText.innerText = "Disconnected";
        connectBtn.innerText = "Connect BLE";
    }
}

/**
 * 12-byte Binary BLE Telemetry Decoder
 */
function handleTelemetryNotification(event) {
    const value = event.target.value;
    if (value.byteLength < 12) return;

    appState.telemetry.rpm          = value.getUint16(0, true);
    appState.telemetry.speed        = value.getUint8(2);
    appState.telemetry.coolantTemp  = value.getInt8(3);
    appState.telemetry.throttlePos  = value.getUint8(4);
    appState.telemetry.batteryVolt  = value.getUint16(5, true) / 1000.0;
    appState.telemetry.leanAngle    = value.getInt16(7, true) / 10.0;
    appState.telemetry.maxLeanRight = value.getInt16(9, true) / 10.0;
    appState.telemetry.maxLeanLeft  = value.getInt16(11, true) / 10.0;

    updateUI();
}

/**
 * Task 1: Trip Recording Engine Logic
 */
function toggleTripRecording() {
    if (!tripState.recording) {
        tripState.recording = true;
        tripState.startTime = Date.now();
        startTripBtn.innerText = "Pause Trip";
        startTripBtn.classList.remove("btn-start");
        startTripBtn.classList.add("btn-stop");

        tripState.timerInterval = setInterval(() => {
            tripState.elapsedSec++;
            updateTripUI();
        }, 1000);
    } else {
        tripState.recording = false;
        clearInterval(tripState.timerInterval);
        startTripBtn.innerText = "Resume Trip";
        startTripBtn.classList.remove("btn-stop");
        startTripBtn.classList.add("btn-start");
    }
}

function resetTripSession() {
    tripState.recording = false;
    if (tripState.timerInterval) clearInterval(tripState.timerInterval);
    
    tripState.elapsedSec = 0;
    tripState.maxSpeed = 0;
    tripState.maxRpm = 0;
    tripState.maxLeanLeft = 0;
    tripState.maxLeanRight = 0;
    tripState.snapshots = [];

    startTripBtn.innerText = "Start Trip";
    startTripBtn.classList.remove("btn-stop");
    startTripBtn.classList.add("btn-start");

    updateTripUI();
}

function updateTripEngine(t) {
    if (!tripState.recording) return;

    if (t.speed > tripState.maxSpeed) tripState.maxSpeed = t.speed;
    if (t.rpm > tripState.maxRpm) tripState.maxRpm = t.rpm;
    if (t.leanAngle < tripState.maxLeanLeft) tripState.maxLeanLeft = t.leanAngle;
    if (t.leanAngle > tripState.maxLeanRight) tripState.maxLeanRight = t.leanAngle;

    // Record snapshot every 2 seconds
    if (tripState.snapshots.length === 0 || (Date.now() - tripState.snapshots[tripState.snapshots.length - 1].ts >= 2000)) {
        tripState.snapshots.push({
            ts: Date.now(),
            speed: t.speed,
            rpm: t.rpm,
            leanAngle: t.leanAngle,
            coolant: t.coolantTemp
        });
    }

    updateTripUI();
}

function updateTripUI() {
    const mins = Math.floor(tripState.elapsedSec / 60);
    const secs = tripState.elapsedSec % 60;
    tripTimerVal.innerText = `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    
    tripMaxSpeed.innerText = tripState.maxSpeed + " km/h";
    tripMaxRpm.innerText = Math.round(tripState.maxRpm) + " RPM";
    tripPeakLeft.innerText = tripState.maxLeanLeft.toFixed(1) + "°";
    tripPeakRight.innerText = tripState.maxLeanRight.toFixed(1) + "°";
}

/**
 * Task 3: Export Trip Telemetry (CSV)
 */
function exportTripCSV() {
    if (tripState.snapshots.length === 0) {
        alert("No trip data recorded yet! Start a trip session first.");
        return;
    }

    let csvContent = "data:text/csv;charset=utf-8,Timestamp,Speed_KMH,RPM,LeanAngle_Deg,CoolantTemp_C\n";
    tripState.snapshots.forEach(s => {
        const timeStr = new Date(s.ts).toISOString();
        csvContent += `${timeStr},${s.speed},${s.rpm},${s.leanAngle},${s.coolant}\n`;
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `Honda_CL250_Trip_${Date.now()}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

/**
 * Task 2: Track Mode vs Urban Mode Layout Controller
 */
function setDisplayMode(mode) {
    document.body.classList.remove("track-mode", "urban-mode");
    trackModeBtn.classList.remove("active");
    urbanModeBtn.classList.remove("active");
    demoModeBtn.classList.remove("active");

    if (mode === "track") {
        document.body.classList.add("track-mode");
        trackModeBtn.classList.add("active");
        disableDemoMode();
    } else if (mode === "urban") {
        document.body.classList.add("urban-mode");
        urbanModeBtn.classList.add("active");
        disableDemoMode();
    } else if (mode === "demo") {
        demoModeBtn.classList.add("active");
        enableDemoMode();
    }
}

/**
 * Gauge Renderers
 */
function updateUI() {
    const t = appState.telemetry;

    speedVal.innerText = t.speed;
    rpmVal.innerText = Math.round(t.rpm);
    updateRpmGaugeArc(t.rpm);

    leanAngleVal.innerText = (t.leanAngle >= 0 ? "+" : "") + t.leanAngle.toFixed(1) + "°";
    maxLeftVal.innerText = t.maxLeanLeft.toFixed(1) + "°";
    maxRightVal.innerText = t.maxLeanRight.toFixed(1) + "°";
    bikeChassisSvg.style.transform = `rotate(${t.leanAngle}deg)`;

    coolantVal.innerText = t.coolantTemp + " °C";
    throttleVal.innerText = t.throttlePos + " %";
    batteryVal.innerText = t.batteryVolt.toFixed(1) + " V";

    // Update Trip Engine
    updateTripEngine(t);
}

function initRpmGauge() {
    if (!rpmGaugeArc) return;
    rpmGaugeArc.style.strokeDasharray = "314";
    rpmGaugeArc.style.strokeDashoffset = "314";
}

function updateRpmGaugeArc(rpm) {
    if (!rpmGaugeArc) return;
    const maxRpm = 10000;
    const fraction = Math.min(Math.max(rpm, 0), maxRpm) / maxRpm;
    rpmGaugeArc.style.strokeDashoffset = 314 - (314 * fraction);
}

async function syncTelematicsToBLE() {
    const song = songInput.value.trim() || "Drive";
    const artist = artistInput.value.trim() || "Honda CL250";
    const dist = parseInt(navDistInput.value, 10) || 0;
    const payloadStr = `SONG:${song}|ARTIST:${artist}|DIST:${dist}|ICON:1`;

    if (appState.connected && appState.rxCharacteristic) {
        try {
            const encoder = new TextEncoder();
            await appState.rxCharacteristic.writeValue(encoder.encode(payloadStr));
            alert("Telematics synced to bike display!");
        } catch (e) {
            alert("Sync Failed: " + e.message);
        }
    } else {
        alert("Payload ready: " + payloadStr);
    }
}

function enableDemoMode() {
    if (appState.demoMode) return;
    appState.demoMode = true;

    let angleDir = 1;
    let angle = 0;

    appState.demoInterval = setInterval(() => {
        angle += 0.8 * angleDir;
        if (angle > 34) angleDir = -1;
        if (angle < -30) angleDir = 1;

        appState.telemetry.rpm = 4000 + Math.sin(Date.now() / 300) * 1500;
        appState.telemetry.speed = Math.round(55 + Math.sin(Date.now() / 500) * 25);
        appState.telemetry.coolantTemp = 90;
        appState.telemetry.throttlePos = Math.round(35 + Math.sin(Date.now() / 400) * 30);
        appState.telemetry.batteryVolt = 14.1;
        appState.telemetry.leanAngle = angle;
        appState.telemetry.maxLeanRight = Math.max(appState.telemetry.maxLeanRight, angle);
        appState.telemetry.maxLeanLeft = Math.min(appState.telemetry.maxLeanLeft, angle);

        updateUI();
    }, 50);
}

function disableDemoMode() {
    appState.demoMode = false;
    if (appState.demoInterval) {
        clearInterval(appState.demoInterval);
        appState.demoInterval = null;
    }
}
