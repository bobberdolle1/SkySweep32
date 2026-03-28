-- ============================================================================
-- SkySweep32 — RF Environment Monitor & Passive Drone Detector
-- EdgeTX / OpenTX Lua Tool Script
-- Compatible: EdgeTX 2.8+, OpenTX 2.3.15+
-- Screens: 128x64 mono, 212x64 mono, 480x272 color, 320x480 color
-- Protocol: CRSF (ExpressLRS / Crossfire)
-- ============================================================================
-- This script monitors the RF environment through the ELRS/Crossfire TX
-- module's own telemetry data. It provides:
--   • Real-time RSSI/LQ/SNR monitoring (both antennas)
--   • Jamming and interference detection algorithms
--   • RF environment quality scoring
--   • Passive drone detection via anomaly patterns
--   • Signal radar visualization
--   • Audio alerts for threats
--   • Dual-band/True Diversity support
-- ============================================================================
-- LEGAL: For authorized defense and research use only
-- ============================================================================

-- ============================================================================
-- CRSF Protocol Constants
-- ============================================================================
local CRSF_ADDRESS_BROADCAST    = 0x00
local CRSF_ADDRESS_FC           = 0xC8
local CRSF_ADDRESS_MODULE       = 0xEE
local CRSF_ADDRESS_RADIO        = 0xEA
local CRSF_ADDRESS_RECEIVER     = 0xEC

local CRSF_FRAME_DEVICE_PING    = 0x28
local CRSF_FRAME_DEVICE_INFO    = 0x29
local CRSF_FRAME_PARAM_ENTRY    = 0x2B
local CRSF_FRAME_PARAM_READ     = 0x2C
local CRSF_FRAME_PARAM_WRITE    = 0x2D
local CRSF_FRAME_LINK_STATS     = 0x14
local CRSF_FRAME_RC_CHANNELS    = 0x16
local CRSF_FRAME_GPS            = 0x02
local CRSF_FRAME_BATTERY        = 0x08
local CRSF_FRAME_ATTITUDE       = 0x1E
local CRSF_FRAME_FLIGHT_MODE    = 0x21

-- ============================================================================
-- ELRS-specific constants
-- ============================================================================
local ELRS_RATES = {
  [0] = "4Hz",   [1] = "25Hz",  [2] = "50Hz",  [3] = "100Hz",
  [4] = "150Hz", [5] = "200Hz", [6] = "250Hz", [7] = "333Hz",
  [8] = "500Hz", [9] = "D250",  [10]= "D500",  [11]= "F500",
  [12]= "F1000"
}

local ELRS_POWERS = {
  [0] = "DYN", [1] = "10mW", [2] = "25mW",  [3] = "50mW",
  [4] = "100", [5] = "250",  [6] = "500",   [7] = "1000",
  [8] = "2000"
}

-- ============================================================================
-- Screen detection and UI constants
-- ============================================================================
local LCD_W, LCD_H
local IS_COLOR = false
local IS_HIRES = false

local function detectScreen()
  -- EdgeTX 2.8+ provides LCD_W and LCD_H as globals
  LCD_W = LCD_W or 128
  LCD_H = LCD_H or 64
  -- Detect color capability
  if lcd.RGB then
    IS_COLOR = true
    if LCD_W >= 320 then IS_HIRES = true end
  else
    IS_COLOR = false
    -- On mono screens, try to detect 212x64 (X9D+, X9E)
    if LCD_W > 128 then
      -- Already detected via global LCD_W
    elseif lcd.getLastRightPos then
      -- OpenTX compatibility: probe screen width
      lcd.drawText(200, 0, " ", 0)
      if lcd.getLastRightPos() > 128 then
        LCD_W = 212
      end
    end
  end
end

-- ============================================================================
-- Color palette
-- ============================================================================
local C_BG, C_FG, C_GREEN, C_YELLOW, C_RED, C_CYAN, C_GRAY, C_DARKGRAY
local C_GRID, C_RADAR_BG, C_ALERT_BG

local function setupColors()
  if IS_COLOR then
    C_BG       = lcd.RGB(10, 12, 18)
    C_FG       = lcd.RGB(220, 225, 235)
    C_GREEN    = lcd.RGB(0, 220, 80)
    C_YELLOW   = lcd.RGB(255, 200, 0)
    C_RED      = lcd.RGB(255, 40, 40)
    C_CYAN     = lcd.RGB(0, 200, 255)
    C_GRAY     = lcd.RGB(100, 110, 120)
    C_DARKGRAY = lcd.RGB(40, 45, 55)
    C_GRID     = lcd.RGB(30, 35, 45)
    C_RADAR_BG = lcd.RGB(5, 20, 10)
    C_ALERT_BG = lcd.RGB(60, 10, 10)
  else
    C_BG       = WHITE or 0
    C_FG       = BLACK or 0xFF
    C_GREEN    = BLACK or 0xFF
    C_YELLOW   = BLACK or 0xFF
    C_RED      = BLACK or 0xFF
    C_CYAN     = BLACK or 0xFF
    C_GRAY     = GREY_DEFAULT or 0x77
    C_DARKGRAY = GREY_DEFAULT or 0x77
    C_GRID     = GREY_DEFAULT or 0x77
  end
end

-- ============================================================================
-- State variables
-- ============================================================================
local STATE = {
  page = 0,           -- 0=radar, 1=link, 2=spectrum, 3=alerts
  maxPages = 4,
  running = true,
  initDone = false,
  lastUpdate = 0,
  frameCount = 0,
}

-- Telemetry data
local TEL = {
  rssi1 = 0,         -- Antenna 1 RSSI (dBm, negative)
  rssi2 = 0,         -- Antenna 2 RSSI (dBm, negative)
  rqly = 0,          -- RX Link Quality (0-100%)
  rsnr = 0,          -- RX SNR (dB)
  ant = 0,           -- Active antenna (0 or 1)
  rfmd = 0,          -- RF Mode (packet rate index)
  tpwr = 0,          -- TX Power (mW)
  tqly = 0,          -- TX Link Quality
  trss = 0,          -- TX RSSI
  tsnr = 0,          -- TX SNR
  -- GPS (from drone)
  gpsLat = 0,
  gpsLon = 0,
  gpsSpd = 0,
  gpsHdg = 0,
  gpsAlt = 0,
  gpsSat = 0,
  -- Battery
  rxBat = 0,
  -- Timestamps
  lastTelemetryMs = 0,
  telemetryActive = false,
}

-- Anomaly detection engine
local DETECT = {
  -- Jamming detection
  jammingScore = 0,       -- 0-100
  jammingAlert = false,
  jammingHistory = {},    -- recent LQ values
  jammingHistLen = 40,
  jammingIdx = 1,

  -- SNR tracking
  snrHistory = {},
  snrHistLen = 40,
  snrIdx = 1,
  snrBaseline = 0,
  snrCalibrated = false,

  -- RSSI tracking
  rssi1History = {},
  rssi2History = {},
  rssiHistLen = 40,
  rssiIdx = 1,

  -- RF environment score (0=terrible, 100=excellent)
  envScore = 100,

  -- Detected anomalies log
  alerts = {},
  maxAlerts = 16,

  -- Interference direction estimation
  directionEstimate = -1,  -- -1=unknown, 0-359 degrees
  directionConfidence = 0, -- 0-100%

  -- Counters
  lqDropCount = 0,        -- LQ drops below threshold
  snrDropCount = 0,       -- SNR drops
  antSwitchCount = 0,     -- Antenna switches per period
  antSwitchWindow = {},
  antSwitchWinLen = 20,
  antSwitchIdx = 1,
  lastAnt = -1,

  -- Rate change detection
  lastRfmd = -1,
  rateDowngrades = 0,

  -- Timing
  lastAnalysisMs = 0,
  analysisIntervalMs = 200,

  -- Calibration
  calibrating = false,
  calibSamples = 0,
  calibRssiSum = 0,
  calibSnrSum = 0,
  calibLqSum = 0,
  baselineRssi = -80,
  baselineLq = 100,
  baselineSnr = 10,
}

-- CRSF device info
local DEVICE = {
  name = "Unknown",
  serialNo = 0,
  hwVer = "",
  swVer = "",
  fieldCount = 0,
  queried = false,
  lastPingMs = 0,
}

-- Spectrum visualization data (RSSI over time as heatmap)
local SPECTRUM = {
  data = {},
  width = 40,    -- data points in history
  idx = 1,
  updateMs = 0,
}

-- ============================================================================
-- Utility functions
-- ============================================================================

-- Polyfill: lcd.drawCircle may not exist on older OpenTX mono builds
if not lcd.drawCircle then
  lcd.drawCircle = function(cx, cy, r)
    local x, y, d = r, 0, 1 - r
    while x >= y do
      lcd.drawPoint(cx+x, cy+y)
      lcd.drawPoint(cx+y, cy+x)
      lcd.drawPoint(cx-y, cy+x)
      lcd.drawPoint(cx-x, cy+y)
      lcd.drawPoint(cx-x, cy-y)
      lcd.drawPoint(cx-y, cy-x)
      lcd.drawPoint(cx+y, cy-x)
      lcd.drawPoint(cx+x, cy-y)
      y = y + 1
      if d < 0 then
        d = d + 2 * y + 1
      else
        x = x - 1
        d = d + 2 * (y - x) + 1
      end
    end
  end
end

local function clamp(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

local function lerp(a, b, t)
  return a + (b - a) * t
end

local function msNow()
  return getTime() * 10  -- getTime() returns in 10ms units
end

local function round(v)
  return math.floor(v + 0.5)
end

local function abs(v)
  if v < 0 then return -v end
  return v
end

-- Ring buffer average
local function ringAvg(buf, len)
  local sum = 0
  local cnt = 0
  for i = 1, len do
    if buf[i] and buf[i] ~= 0 then
      sum = sum + buf[i]
      cnt = cnt + 1
    end
  end
  if cnt == 0 then return 0 end
  return sum / cnt
end

-- Ring buffer variance
local function ringVariance(buf, len)
  local avg = ringAvg(buf, len)
  local sumSq = 0
  local cnt = 0
  for i = 1, len do
    if buf[i] and buf[i] ~= 0 then
      local d = buf[i] - avg
      sumSq = sumSq + d * d
      cnt = cnt + 1
    end
  end
  if cnt <= 1 then return 0 end
  return sumSq / (cnt - 1)
end

-- Ring buffer min
local function ringMin(buf, len)
  local m = 99999
  for i = 1, len do
    if buf[i] and buf[i] ~= 0 and buf[i] < m then
      m = buf[i]
    end
  end
  if m == 99999 then return 0 end
  return m
end

-- Add to alerts log
local function addAlert(level, text)
  local now = msNow()
  table.insert(DETECT.alerts, 1, {
    time = now,
    level = level,  -- 0=info, 1=warn, 2=critical
    text = text,
  })
  -- Trim old alerts
  while #DETECT.alerts > DETECT.maxAlerts do
    table.remove(DETECT.alerts)
  end
  -- Audio alert
  if level >= 2 then
    playTone(1200, 300, 100, PLAY_NOW)
    playTone(1600, 200, 100)
  elseif level >= 1 then
    playTone(800, 200, 80, PLAY_NOW)
  end
end

-- ============================================================================
-- Telemetry reading
-- ============================================================================
local sensorCache = {}

local function getSensorValue(name)
  -- Cache field info lookups for performance
  if not sensorCache[name] then
    local info = getFieldInfo(name)
    if info then
      sensorCache[name] = info.id
    else
      return nil
    end
  end
  local val = getValue(sensorCache[name])
  return val
end

local function readTelemetry()
  -- ELRS / Crossfire standard sensor names
  local r1 = getSensorValue("1RSS")
  local r2 = getSensorValue("2RSS")
  local rq = getSensorValue("RQly")
  local rs = getSensorValue("RSNR")
  local an = getSensorValue("ANT")
  local rf = getSensorValue("RFMD")
  local tp = getSensorValue("TPWR")
  local tq = getSensorValue("TQly")
  local tr = getSensorValue("TRSS")
  local ts = getSensorValue("TSNR")

  -- Alt sensor names (OpenTX / different ELRS versions)
  if not r1 then r1 = getSensorValue("RSSI") end
  if not r1 then r1 = getSensorValue("Rssi") end

  if r1 then TEL.rssi1 = r1 end
  if r2 then TEL.rssi2 = r2 end
  if rq then TEL.rqly = rq end
  if rs then TEL.rsnr = rs end
  if an then TEL.ant = an end
  if rf then TEL.rfmd = rf end
  if tp then TEL.tpwr = tp end
  if tq then TEL.tqly = tq end
  if tr then TEL.trss = tr end
  if ts then TEL.tsnr = ts end

  -- GPS telemetry (from drone's FC via ELRS)
  local gps = getSensorValue("GPS")
  if gps and type(gps) == "table" then
    TEL.gpsLat = gps.lat or gps[1] or 0
    TEL.gpsLon = gps.lon or gps[2] or 0
  end
  local ga = getSensorValue("Alt")
  if ga then TEL.gpsAlt = ga end
  local gs = getSensorValue("GSpd")
  if gs then TEL.gpsSpd = gs end
  local gh = getSensorValue("Hdg")
  if gh then TEL.gpsHdg = gh end
  local gsat = getSensorValue("Sats")
  if gsat then TEL.gpsSat = gsat end

  -- Battery
  local rxb = getSensorValue("RxBt")
  if rxb then TEL.rxBat = rxb end

  -- Check if telemetry is active
  local now = msNow()
  if r1 or rq then
    TEL.lastTelemetryMs = now
    TEL.telemetryActive = true
  elseif now - TEL.lastTelemetryMs > 3000 then
    TEL.telemetryActive = false
  end
end

-- ============================================================================
-- CRSF packet communication with ELRS module
-- ============================================================================
local function crsfPing()
  -- Ping ELRS module to get device info
  local now = msNow()
  if now - DEVICE.lastPingMs < 5000 then return end
  DEVICE.lastPingMs = now
  crossfireTelemetryPush(CRSF_FRAME_DEVICE_PING, {
    CRSF_ADDRESS_MODULE, CRSF_ADDRESS_RADIO
  })
end

local function crsfPoll()
  -- Poll for any incoming CRSF frames from the module
  local cmd, data = crossfireTelemetryPop()
  if not cmd then return end

  if cmd == CRSF_FRAME_DEVICE_INFO and data then
    -- Parse device name (null-terminated string at start of data)
    local name = ""
    local i = 2
    while i <= #data and data[i] ~= 0 do
      name = name .. string.char(data[i])
      i = i + 1
    end
    if #name > 0 then
      DEVICE.name = name
      DEVICE.queried = true
    end
  end

  -- Note: Link stats (0x14) are typically handled by EdgeTX internally
  -- and exposed as sensors. We read them via getValue() above.
  -- But if we catch raw frames, parse them too:
  if cmd == CRSF_FRAME_LINK_STATS and data and #data >= 10 then
    TEL.rssi1 = -(data[1] or 0)
    TEL.rssi2 = -(data[2] or 0)
    TEL.rqly = data[3] or 0
    TEL.rsnr = data[4] or 0  -- signed
    if TEL.rsnr > 127 then TEL.rsnr = TEL.rsnr - 256 end
    TEL.ant = data[5] or 0
    TEL.rfmd = data[6] or 0
    TEL.tpwr = data[7] or 0
    TEL.trss = data[8] or 0
    TEL.tqly = data[9] or 0
    TEL.tsnr = data[10] or 0
    if TEL.tsnr > 127 then TEL.tsnr = TEL.tsnr - 256 end
    TEL.lastTelemetryMs = msNow()
    TEL.telemetryActive = true
  end
end

-- ============================================================================
-- Anomaly Detection Engine
-- ============================================================================
local function updateDetection()
  local now = msNow()
  if now - DETECT.lastAnalysisMs < DETECT.analysisIntervalMs then return end
  DETECT.lastAnalysisMs = now

  if not TEL.telemetryActive then
    DETECT.envScore = 0
    return
  end

  -- Store history
  DETECT.jammingHistory[DETECT.jammingIdx] = TEL.rqly
  DETECT.jammingIdx = (DETECT.jammingIdx % DETECT.jammingHistLen) + 1

  DETECT.snrHistory[DETECT.snrIdx] = TEL.rsnr
  DETECT.snrIdx = (DETECT.snrIdx % DETECT.snrHistLen) + 1

  DETECT.rssi1History[DETECT.rssiIdx] = TEL.rssi1
  DETECT.rssi2History[DETECT.rssiIdx] = TEL.rssi2
  DETECT.rssiIdx = (DETECT.rssiIdx % DETECT.rssiHistLen) + 1

  -- Track antenna switches
  if DETECT.lastAnt >= 0 and TEL.ant ~= DETECT.lastAnt then
    DETECT.antSwitchWindow[DETECT.antSwitchIdx] = 1
  else
    DETECT.antSwitchWindow[DETECT.antSwitchIdx] = 0
  end
  DETECT.antSwitchIdx = (DETECT.antSwitchIdx % DETECT.antSwitchWinLen) + 1
  DETECT.lastAnt = TEL.ant

  -- Count antenna switches in window
  DETECT.antSwitchCount = 0
  for i = 1, DETECT.antSwitchWinLen do
    if DETECT.antSwitchWindow[i] and DETECT.antSwitchWindow[i] > 0 then
      DETECT.antSwitchCount = DETECT.antSwitchCount + 1
    end
  end

  -- Track RF mode changes
  if DETECT.lastRfmd >= 0 and TEL.rfmd < DETECT.lastRfmd then
    DETECT.rateDowngrades = DETECT.rateDowngrades + 1
    addAlert(1, "RATE DN: " .. (ELRS_RATES[TEL.rfmd] or "?"))
  end
  DETECT.lastRfmd = TEL.rfmd

  -- =============================================
  -- JAMMING DETECTION ALGORITHM
  -- =============================================
  local jammingScore = 0

  -- Factor 1: Link Quality drop (most reliable indicator)
  local avgLq = ringAvg(DETECT.jammingHistory, DETECT.jammingHistLen)
  if TEL.rqly < 70 then
    jammingScore = jammingScore + clamp((70 - TEL.rqly) * 2, 0, 40)
  end
  if avgLq < 80 then
    jammingScore = jammingScore + clamp((80 - avgLq), 0, 20)
  end

  -- Factor 2: SNR degradation (noise floor rising = jamming)
  if DETECT.snrCalibrated then
    local snrDelta = DETECT.baselineSnr - TEL.rsnr
    if snrDelta > 3 then
      jammingScore = jammingScore + clamp(snrDelta * 3, 0, 30)
    end
  else
    -- Without baseline, use absolute threshold
    if TEL.rsnr < -2 then
      jammingScore = jammingScore + clamp(abs(TEL.rsnr) * 2, 0, 20)
    end
  end

  -- Factor 3: LQ variance (jamming causes erratic LQ patterns)
  local lqVariance = ringVariance(DETECT.jammingHistory, DETECT.jammingHistLen)
  if lqVariance > 200 then
    jammingScore = jammingScore + clamp((lqVariance - 200) / 20, 0, 15)
  end

  -- Factor 4: Rapid antenna switching (interference from specific direction)
  if DETECT.antSwitchCount > DETECT.antSwitchWinLen * 0.6 then
    jammingScore = jammingScore + 10
  end

  -- Factor 5: RSSI stable but LQ dropping (classic narrowband jam pattern)
  local rssiVar = ringVariance(DETECT.rssi1History, DETECT.rssiHistLen)
  if rssiVar < 10 and TEL.rqly < 60 then
    jammingScore = jammingScore + 15  -- Strong indicator
  end

  -- Factor 6: Both antennas degraded simultaneously (broadband)
  local r1avg = ringAvg(DETECT.rssi1History, DETECT.rssiHistLen)
  local r2avg = ringAvg(DETECT.rssi2History, DETECT.rssiHistLen)
  if r1avg < -90 and r2avg < -90 then
    jammingScore = jammingScore + 10
  end

  DETECT.jammingScore = clamp(jammingScore, 0, 100)

  -- Alert on jamming
  if DETECT.jammingScore >= 60 and not DETECT.jammingAlert then
    DETECT.jammingAlert = true
    addAlert(2, "JAM DETECT! Score:" .. DETECT.jammingScore)
  elseif DETECT.jammingScore < 30 then
    DETECT.jammingAlert = false
  end

  -- =============================================
  -- RF ENVIRONMENT SCORE
  -- =============================================
  local envScore = 100

  -- LQ contribution (most important)
  envScore = envScore - clamp((100 - TEL.rqly) * 0.5, 0, 30)

  -- SNR contribution
  if TEL.rsnr < 5 then
    envScore = envScore - clamp((5 - TEL.rsnr) * 3, 0, 25)
  end

  -- RSSI contribution (best antenna)
  local bestRssi = TEL.rssi1
  if TEL.rssi2 ~= 0 and TEL.rssi2 > bestRssi then
    bestRssi = TEL.rssi2
  end
  if bestRssi < -100 then
    envScore = envScore - clamp((-100 - bestRssi) * 2, 0, 20)
  end

  -- Jamming penalty
  envScore = envScore - DETECT.jammingScore * 0.3

  -- Antenna instability penalty
  if DETECT.antSwitchCount > 10 then
    envScore = envScore - 5
  end

  DETECT.envScore = clamp(round(envScore), 0, 100)

  -- =============================================
  -- DIRECTION ESTIMATION (from antenna diversity)
  -- =============================================
  -- Very rough: compare RSSI between antenna 0 and antenna 1
  -- Positive delta = interference source closer to ant1 side
  if TEL.rssi1 ~= 0 and TEL.rssi2 ~= 0 then
    local delta = TEL.rssi1 - TEL.rssi2
    if abs(delta) > 6 then
      DETECT.directionConfidence = clamp(abs(delta) * 5, 0, 80)
      if delta > 0 then
        DETECT.directionEstimate = 90   -- Stronger on antenna 1
      else
        DETECT.directionEstimate = 270  -- Stronger on antenna 2
      end
    else
      DETECT.directionConfidence = clamp(DETECT.directionConfidence - 5, 0, 100)
    end
  end

  -- Spectrum data update
  SPECTRUM.data[SPECTRUM.idx] = {
    r1 = TEL.rssi1,
    r2 = TEL.rssi2,
    lq = TEL.rqly,
    snr = TEL.rsnr,
    jam = DETECT.jammingScore,
  }
  SPECTRUM.idx = (SPECTRUM.idx % SPECTRUM.width) + 1
end

-- ============================================================================
-- Calibration (baseline measurement)
-- ============================================================================
local function startCalibration()
  DETECT.calibrating = true
  DETECT.calibSamples = 0
  DETECT.calibRssiSum = 0
  DETECT.calibSnrSum = 0
  DETECT.calibLqSum = 0
  addAlert(0, "CAL START: Hold 5s")
end

local function updateCalibration()
  if not DETECT.calibrating then return end
  if not TEL.telemetryActive then return end

  DETECT.calibSamples = DETECT.calibSamples + 1
  DETECT.calibRssiSum = DETECT.calibRssiSum + TEL.rssi1
  DETECT.calibSnrSum = DETECT.calibSnrSum + TEL.rsnr
  DETECT.calibLqSum = DETECT.calibLqSum + TEL.rqly

  if DETECT.calibSamples >= 25 then  -- ~5 seconds at 200ms intervals
    DETECT.baselineRssi = DETECT.calibRssiSum / DETECT.calibSamples
    DETECT.baselineSnr = DETECT.calibSnrSum / DETECT.calibSamples
    DETECT.baselineLq = DETECT.calibLqSum / DETECT.calibSamples
    DETECT.snrBaseline = DETECT.baselineSnr
    DETECT.snrCalibrated = true
    DETECT.calibrating = false
    addAlert(0, string.format("CAL OK: SNR=%.1f LQ=%.0f", DETECT.baselineSnr, DETECT.baselineLq))
  end
end

-- ============================================================================
-- DRAWING — Mono 128x64 screens
-- ============================================================================

local function drawHeaderMono()
  lcd.drawText(0, 0, "SkySweep", SMLSIZE)
  -- Telemetry status indicator
  if TEL.telemetryActive then
    lcd.drawFilledRectangle(80, 0, 4, 6)
  else
    lcd.drawRectangle(80, 0, 4, 6)
  end
  -- Page indicator
  local pages = {"RAD", "LNK", "SPE", "ALR"}
  lcd.drawText(90, 0, pages[STATE.page + 1] or "?", SMLSIZE + INVERS)
  -- Environment score
  local envTxt = string.format("%d%%", DETECT.envScore)
  lcd.drawText(115, 0, envTxt, SMLSIZE)
  lcd.drawLine(0, 8, 127, 8, SOLID, FORCE)
end

-- Page 0: Radar / Overview
local function drawRadarMono()
  drawHeaderMono()

  -- Radar circle (center of left half)
  local cx, cy, cr = 32, 38, 22
  -- Draw radar circles
  lcd.drawCircle(cx, cy, cr)
  lcd.drawCircle(cx, cy, round(cr * 0.66))
  lcd.drawCircle(cx, cy, round(cr * 0.33))
  -- Crosshair
  lcd.drawLine(cx - cr, cy, cx + cr, cy, DOTTED, 0)
  lcd.drawLine(cx, cy - cr, cx, cy + cr, DOTTED, 0)

  -- Sweep line (rotating) for visual effect
  local sweepAngle = (msNow() / 30) % 360
  local sx = cx + round(math.cos(math.rad(sweepAngle)) * cr)
  local sy = cy - round(math.sin(math.rad(sweepAngle)) * cr)
  lcd.drawLine(cx, cy, sx, sy, SOLID, FORCE)

  -- Direction indicator (if interference detected)
  if DETECT.directionConfidence > 20 and DETECT.jammingScore > 15 then
    local da = math.rad(DETECT.directionEstimate)
    local dx = cx + round(math.cos(da) * (cr - 4))
    local dy = cy - round(math.sin(da) * (cr - 4))
    lcd.drawFilledRectangle(dx - 2, dy - 2, 5, 5)
  end

  -- Right panel: key metrics
  local rx = 68
  local ry = 11

  -- RSSI bars
  lcd.drawText(rx, ry, "R1:", SMLSIZE)
  local r1bar = clamp(round((TEL.rssi1 + 130) * 0.76), 0, 38)
  lcd.drawRectangle(rx + 16, ry, 40, 6)
  if r1bar > 0 then lcd.drawFilledRectangle(rx + 17, ry + 1, r1bar, 4) end
  lcd.drawText(rx + 58, ry, tostring(round(TEL.rssi1)), SMLSIZE)

  ry = ry + 8
  lcd.drawText(rx, ry, "R2:", SMLSIZE)
  local r2bar = clamp(round((TEL.rssi2 + 130) * 0.76), 0, 38)
  lcd.drawRectangle(rx + 16, ry, 40, 6)
  if r2bar > 0 then lcd.drawFilledRectangle(rx + 17, ry + 1, r2bar, 4) end
  lcd.drawText(rx + 58, ry, tostring(round(TEL.rssi2)), SMLSIZE)

  -- LQ
  ry = ry + 9
  lcd.drawText(rx, ry, "LQ:", SMLSIZE)
  lcd.drawText(rx + 16, ry, string.format("%d%%", TEL.rqly), SMLSIZE)

  -- SNR
  ry = ry + 8
  lcd.drawText(rx, ry, "SN:", SMLSIZE)
  lcd.drawText(rx + 16, ry, string.format("%ddB", TEL.rsnr), SMLSIZE)

  -- RF Mode
  ry = ry + 8
  lcd.drawText(rx, ry, "RF:", SMLSIZE)
  lcd.drawText(rx + 16, ry, ELRS_RATES[TEL.rfmd] or "?", SMLSIZE)

  -- Jamming indicator bar (bottom)
  lcd.drawText(0, 58, "JAM:", SMLSIZE)
  local jamW = clamp(round(DETECT.jammingScore * 0.9), 0, 90)
  lcd.drawRectangle(24, 58, 92, 6)
  if jamW > 0 then
    lcd.drawFilledRectangle(25, 59, jamW, 4)
  end
  if DETECT.jammingAlert then
    lcd.drawText(116, 58, "!", SMLSIZE + BLINK)
  end
end

-- Page 1: Link Details
local function drawLinkMono()
  drawHeaderMono()
  local y = 11

  lcd.drawText(0, y, "ANT1 RSSI:", SMLSIZE)
  lcd.drawText(60, y, string.format("%d dBm", round(TEL.rssi1)), SMLSIZE)
  y = y + 8

  lcd.drawText(0, y, "ANT2 RSSI:", SMLSIZE)
  lcd.drawText(60, y, string.format("%d dBm", round(TEL.rssi2)), SMLSIZE)
  y = y + 8

  lcd.drawText(0, y, "Link Qlty:", SMLSIZE)
  lcd.drawText(60, y, string.format("%d %%", TEL.rqly), SMLSIZE)
  y = y + 8

  lcd.drawText(0, y, "SNR:", SMLSIZE)
  lcd.drawText(60, y, string.format("%d dB", TEL.rsnr), SMLSIZE)
  y = y + 8

  lcd.drawText(0, y, "Active Ant:", SMLSIZE)
  lcd.drawText(60, y, string.format("ANT %d", TEL.ant), SMLSIZE)
  y = y + 8

  lcd.drawText(0, y, "TX Power:", SMLSIZE)
  lcd.drawText(60, y, string.format("%d mW", TEL.tpwr), SMLSIZE)
  y = y + 8

  lcd.drawText(0, y, "RF Mode:", SMLSIZE)
  lcd.drawText(60, y, ELRS_RATES[TEL.rfmd] or "?", SMLSIZE)

  -- Device info at bottom
  if DEVICE.queried then
    lcd.drawText(0, 56, DEVICE.name, SMLSIZE)
  end
end

-- Page 2: Spectrum / RSSI History Graph
local function drawSpectrumMono()
  drawHeaderMono()

  local gx, gy, gw, gh = 0, 10, 128, 42

  -- Grid
  lcd.drawRectangle(gx, gy, gw, gh)
  lcd.drawLine(gx, gy + round(gh/3), gx + gw - 1, gy + round(gh/3), DOTTED, 0)
  lcd.drawLine(gx, gy + round(gh*2/3), gx + gw - 1, gy + round(gh*2/3), DOTTED, 0)

  -- Labels
  lcd.drawText(gx, gy - 1, "-40", SMLSIZE)
  lcd.drawText(gx, gy + gh - 1, "-120", SMLSIZE)

  -- Draw RSSI1 history as line graph
  local dataCount = 0
  for i = 1, DETECT.rssiHistLen do
    if DETECT.rssi1History[i] then dataCount = dataCount + 1 end
  end

  if dataCount > 1 then
    local stepW = (gw - 4) / (DETECT.rssiHistLen - 1)
    local prevX, prevY1, prevY2

    for i = 0, DETECT.rssiHistLen - 1 do
      local idx = ((DETECT.rssiIdx - 1 + i) % DETECT.rssiHistLen) + 1
      local r1 = DETECT.rssi1History[idx]
      local r2 = DETECT.rssi2History[idx]

      if r1 and r1 ~= 0 then
        local px = gx + 2 + round(i * stepW)
        local py1 = gy + gh - 2 - round(clamp((r1 + 120) / 80, 0, 1) * (gh - 4))

        if prevX then
          lcd.drawLine(prevX, prevY1, px, py1, SOLID, FORCE)
        end
        prevX = px
        prevY1 = py1

        -- Draw RSSI2 as dotted if available
        if r2 and r2 ~= 0 then
          local py2 = gy + gh - 2 - round(clamp((r2 + 120) / 80, 0, 1) * (gh - 4))
          if prevY2 then
            lcd.drawLine(prevX - round(stepW), prevY2, px, py2, DOTTED, FORCE)
          end
          prevY2 = py2
        end
      end
    end
  end

  -- LQ bar at bottom
  lcd.drawText(0, 54, "LQ:", SMLSIZE)
  local lqW = clamp(round(TEL.rqly * 1.0), 0, 100)
  lcd.drawRectangle(16, 54, 102, 6)
  if lqW > 0 then lcd.drawFilledRectangle(17, 55, lqW, 4) end
  lcd.drawText(120, 54, string.format("%d", TEL.rqly), SMLSIZE)

  -- Env score
  lcd.drawText(0, 62, string.format("ENV:%d%%", DETECT.envScore), SMLSIZE)
  lcd.drawText(50, 62, string.format("JAM:%d", DETECT.jammingScore), SMLSIZE)
  if DETECT.snrCalibrated then
    lcd.drawText(90, 62, "CAL", SMLSIZE + INVERS)
  end
end

-- Page 3: Alert Log
local function drawAlertsMono()
  drawHeaderMono()

  if #DETECT.alerts == 0 then
    lcd.drawText(10, 30, "No alerts", SMLSIZE)
    lcd.drawText(10, 40, "ENTER=Calibrate", SMLSIZE)
    return
  end

  local y = 11
  local maxShow = 6
  for i = 1, math.min(#DETECT.alerts, maxShow) do
    local a = DETECT.alerts[i]
    local prefix = ""
    local flags = SMLSIZE
    if a.level >= 2 then
      prefix = "!!"
      flags = SMLSIZE + BLINK
    elseif a.level >= 1 then
      prefix = "! "
    else
      prefix = "  "
    end
    local ageS = round((msNow() - a.time) / 1000)
    local line = string.format("%s%ds %s", prefix, ageS, a.text)
    lcd.drawText(0, y, string.sub(line, 1, 24), flags)
    y = y + 8
  end

  lcd.drawText(0, 58, string.format("Total: %d  ENTER=Cal", #DETECT.alerts), SMLSIZE)
end

-- ============================================================================
-- DRAWING — Color screens (480x272 and similar)
-- ============================================================================

local function drawHeaderColor()
  lcd.drawFilledRectangle(0, 0, LCD_W, 24, C_DARKGRAY)
  lcd.drawText(6, 2, "SkySweep32", MIDSIZE + C_CYAN)

  -- Status dot
  if TEL.telemetryActive then
    lcd.drawFilledRectangle(130, 6, 10, 10, C_GREEN)
  else
    lcd.drawFilledRectangle(130, 6, 10, 10, C_RED)
  end

  -- Page tabs
  local pages = {"RADAR", "LINK", "SPECTRUM", "ALERTS"}
  local tabX = 200
  for i = 0, #pages - 1 do
    local col = (i == STATE.page) and C_CYAN or C_GRAY
    lcd.drawText(tabX, 4, pages[i + 1], SMLSIZE + col)
    tabX = tabX + 68
  end

  -- Env score
  local envCol = C_GREEN
  if DETECT.envScore < 50 then envCol = C_RED
  elseif DETECT.envScore < 75 then envCol = C_YELLOW end
  lcd.drawText(LCD_W - 50, 4, string.format("%d%%", DETECT.envScore), MIDSIZE + envCol)
end

local function drawRadarColor()
  drawHeaderColor()

  -- Radar circle area
  local cx, cy, cr = 120, 152, 100
  lcd.drawFilledRectangle(cx - cr - 5, cy - cr - 5, cr * 2 + 10, cr * 2 + 10, C_RADAR_BG)

  -- Radar rings
  for r = 1, 3 do
    lcd.drawCircle(cx, cy, round(cr * r / 3), C_GRID)
  end
  lcd.drawCircle(cx, cy, cr, C_GRAY)

  -- Crosshair
  lcd.drawLine(cx - cr, cy, cx + cr, cy, DOTTED, C_GRID)
  lcd.drawLine(cx, cy - cr, cx, cy + cr, DOTTED, C_GRID)

  -- Sweep animation
  local sweepAngle = (msNow() / 25) % 360
  local sx = cx + round(math.cos(math.rad(sweepAngle)) * cr)
  local sy = cy - round(math.sin(math.rad(sweepAngle)) * cr)
  lcd.drawLine(cx, cy, sx, sy, SOLID, C_GREEN)

  -- RSSI strength as radar fill (based on signal level)
  local signalR = clamp(round((TEL.rssi1 + 130) / 100 * cr), 5, cr)
  lcd.drawCircle(cx, cy, signalR, C_CYAN)

  -- Direction indicator
  if DETECT.directionConfidence > 20 and DETECT.jammingScore > 15 then
    local da = math.rad(DETECT.directionEstimate)
    local dx = cx + round(math.cos(da) * (cr - 15))
    local dy = cy - round(math.sin(da) * (cr - 15))
    lcd.drawFilledRectangle(dx - 5, dy - 5, 11, 11, C_RED)
    lcd.drawText(dx + 8, dy - 4, "THR", SMLSIZE + C_RED)
  end

  -- Range labels
  lcd.drawText(cx - cr, cy + cr + 4, "-120", SMLSIZE + C_GRAY)
  lcd.drawText(cx, cy + cr + 4, "dBm", SMLSIZE + C_GRAY)

  -- Right panel
  local px = 250
  local py = 30

  -- RSSI bars (colored)
  lcd.drawText(px, py, "ANT 1", SMLSIZE + C_FG)
  py = py + 14
  local r1pct = clamp(round((TEL.rssi1 + 130) * 0.77), 0, 100)
  local r1col = r1pct > 60 and C_GREEN or (r1pct > 30 and C_YELLOW or C_RED)
  lcd.drawFilledRectangle(px, py, round(r1pct * 2), 12, r1col)
  lcd.drawRectangle(px, py, 200, 12, C_GRAY)
  lcd.drawText(px + 205, py, string.format("%d dBm", round(TEL.rssi1)), SMLSIZE + C_FG)

  py = py + 18
  lcd.drawText(px, py, "ANT 2", SMLSIZE + C_FG)
  py = py + 14
  local r2pct = clamp(round((TEL.rssi2 + 130) * 0.77), 0, 100)
  local r2col = r2pct > 60 and C_GREEN or (r2pct > 30 and C_YELLOW or C_RED)
  lcd.drawFilledRectangle(px, py, round(r2pct * 2), 12, r2col)
  lcd.drawRectangle(px, py, 200, 12, C_GRAY)
  lcd.drawText(px + 205, py, string.format("%d dBm", round(TEL.rssi2)), SMLSIZE + C_FG)

  -- LQ / SNR
  py = py + 22
  lcd.drawText(px, py, string.format("LQ: %d%%", TEL.rqly), 0 + (TEL.rqly > 80 and C_GREEN or (TEL.rqly > 50 and C_YELLOW or C_RED)))
  lcd.drawText(px + 100, py, string.format("SNR: %ddB", TEL.rsnr), 0 + C_FG)

  py = py + 18
  lcd.drawText(px, py, string.format("RF: %s", ELRS_RATES[TEL.rfmd] or "?"), 0 + C_CYAN)
  lcd.drawText(px + 100, py, string.format("PWR: %dmW", TEL.tpwr), 0 + C_FG)

  py = py + 18
  lcd.drawText(px, py, string.format("Active: ANT%d", TEL.ant), 0 + C_FG)
  lcd.drawText(px + 100, py, string.format("Switches: %d", DETECT.antSwitchCount), 0 + C_FG)

  -- Jamming bar at bottom
  local jy = 248
  lcd.drawText(px, jy, "JAMMING:", SMLSIZE + C_FG)
  local jamCol = DETECT.jammingScore > 60 and C_RED or (DETECT.jammingScore > 30 and C_YELLOW or C_GREEN)
  lcd.drawFilledRectangle(px + 55, jy, round(DETECT.jammingScore * 1.7), 14, jamCol)
  lcd.drawRectangle(px + 55, jy, 170, 14, C_GRAY)
  lcd.drawText(px + 55 + 175, jy, string.format("%d", DETECT.jammingScore), SMLSIZE + jamCol)
  if DETECT.jammingAlert then
    lcd.drawText(px + 55 + 60, jy + 1, "!! DETECTED !!", SMLSIZE + C_RED + BLINK)
  end
end

local function drawLinkColor()
  drawHeaderColor()

  local px, py = 20, 35
  local lineH = 22

  -- Main telemetry grid
  local labels = {
    {"ANT 1 RSSI",    string.format("%d dBm", round(TEL.rssi1))},
    {"ANT 2 RSSI",    string.format("%d dBm", round(TEL.rssi2))},
    {"Link Quality",  string.format("%d %%", TEL.rqly)},
    {"SNR",           string.format("%d dB", TEL.rsnr)},
    {"Active Antenna", string.format("ANT %d", TEL.ant)},
    {"RF Mode",       ELRS_RATES[TEL.rfmd] or "Unknown"},
    {"TX Power",      string.format("%d mW", TEL.tpwr)},
    {"TX RSSI",       string.format("%d dBm", round(TEL.trss))},
    {"TX Quality",    string.format("%d %%", TEL.tqly)},
    {"TX SNR",        string.format("%d dB", TEL.tsnr)},
  }

  for i, row in ipairs(labels) do
    local y = py + (i - 1) * lineH
    if i > 5 then
      -- Second column
      local x2 = 250
      y = py + (i - 6) * lineH
      lcd.drawText(x2, y, row[1] .. ":", SMLSIZE + C_GRAY)
      lcd.drawText(x2 + 100, y, row[2], SMLSIZE + C_FG)
    else
      lcd.drawText(px, y, row[1] .. ":", SMLSIZE + C_GRAY)
      lcd.drawText(px + 120, y, row[2], 0 + C_FG)
    end
  end

  -- GPS data if available
  py = 150
  lcd.drawLine(px, py, LCD_W - 20, py, SOLID, C_DARKGRAY)
  py = py + 5
  if TEL.gpsLat ~= 0 then
    lcd.drawText(px, py, string.format("GPS: %.6f, %.6f", TEL.gpsLat, TEL.gpsLon), SMLSIZE + C_CYAN)
    lcd.drawText(px, py + 16, string.format("Alt: %.0fm  Spd: %.1fkm/h  Sat: %d",
      TEL.gpsAlt, TEL.gpsSpd, TEL.gpsSat), SMLSIZE + C_FG)
  else
    lcd.drawText(px, py, "GPS: No data", SMLSIZE + C_GRAY)
  end

  -- Device info
  py = 210
  lcd.drawText(px, py, "Module: " .. DEVICE.name, SMLSIZE + C_GRAY)
  if DETECT.snrCalibrated then
    lcd.drawText(px, py + 16, string.format("Baseline: SNR=%.1f LQ=%.0f RSSI=%.0f",
      DETECT.baselineSnr, DETECT.baselineLq, DETECT.baselineRssi), SMLSIZE + C_GREEN)
  else
    lcd.drawText(px, py + 16, "Not calibrated — press ENTER to calibrate", SMLSIZE + C_YELLOW)
  end
end

local function drawSpectrumColor()
  drawHeaderColor()

  local gx, gy, gw, gh = 20, 35, LCD_W - 40, 170

  -- Background
  lcd.drawFilledRectangle(gx, gy, gw, gh, C_RADAR_BG)
  lcd.drawRectangle(gx, gy, gw, gh, C_GRAY)

  -- Grid lines
  for i = 1, 3 do
    local ly = gy + round(gh * i / 4)
    lcd.drawLine(gx, ly, gx + gw, ly, DOTTED, C_GRID)
  end

  -- Y-axis labels
  lcd.drawText(gx - 18, gy, "-40", SMLSIZE + C_GRAY)
  lcd.drawText(gx - 18, gy + round(gh/2), "-80", SMLSIZE + C_GRAY)
  lcd.drawText(gx - 18, gy + gh - 8, "-120", SMLSIZE + C_GRAY)

  -- Plot RSSI1 line (solid green)
  local stepW = (gw - 4) / (DETECT.rssiHistLen - 1)
  local prevX, prevY1, prevY2

  for i = 0, DETECT.rssiHistLen - 1 do
    local idx = ((DETECT.rssiIdx - 1 + i) % DETECT.rssiHistLen) + 1
    local r1 = DETECT.rssi1History[idx]
    local r2 = DETECT.rssi2History[idx]

    if r1 and r1 ~= 0 then
      local px = gx + 2 + round(i * stepW)
      local py1 = gy + gh - 2 - round(clamp((r1 + 120) / 80, 0, 1) * (gh - 4))

      if prevX then
        lcd.drawLine(prevX, prevY1, px, py1, SOLID, C_GREEN)
      end
      prevX = px
      prevY1 = py1

      if r2 and r2 ~= 0 then
        local py2 = gy + gh - 2 - round(clamp((r2 + 120) / 80, 0, 1) * (gh - 4))
        if prevY2 then
          lcd.drawLine(prevX - round(stepW), prevY2, px, py2, SOLID, C_CYAN)
        end
        prevY2 = py2
      end
    end
  end

  -- Legend
  lcd.drawFilledRectangle(gx + 5, gy + 3, 8, 8, C_GREEN)
  lcd.drawText(gx + 16, gy + 2, "ANT1", SMLSIZE + C_GREEN)
  lcd.drawFilledRectangle(gx + 60, gy + 3, 8, 8, C_CYAN)
  lcd.drawText(gx + 71, gy + 2, "ANT2", SMLSIZE + C_CYAN)

  -- Status panel below graph
  local sy = gy + gh + 8
  lcd.drawText(gx, sy, string.format("LQ: %d%%", TEL.rqly), 0 + (TEL.rqly > 80 and C_GREEN or C_RED))
  lcd.drawText(gx + 100, sy, string.format("SNR: %ddB", TEL.rsnr), 0 + C_FG)
  lcd.drawText(gx + 200, sy, string.format("ENV: %d%%", DETECT.envScore), 0 + C_FG)
  lcd.drawText(gx + 310, sy, string.format("JAM: %d", DETECT.jammingScore),
    0 + (DETECT.jammingScore > 30 and C_RED or C_GREEN))

  if DETECT.snrCalibrated then
    lcd.drawText(gx + gw - 30, sy, "CAL", SMLSIZE + INVERS + C_GREEN)
  end
end

local function drawAlertsColor()
  drawHeaderColor()

  local px, py = 20, 35

  if #DETECT.alerts == 0 then
    lcd.drawText(px + 100, 130, "No Alerts", MIDSIZE + C_GRAY)
    lcd.drawText(px + 80, 155, "Press ENTER to calibrate", SMLSIZE + C_GRAY)
    return
  end

  local lineH = 20
  local maxShow = 10
  for i = 1, math.min(#DETECT.alerts, maxShow) do
    local a = DETECT.alerts[i]
    local y = py + (i - 1) * lineH
    local col = C_FG
    local bgCol = nil

    if a.level >= 2 then
      col = C_RED
      bgCol = C_ALERT_BG
    elseif a.level >= 1 then
      col = C_YELLOW
    end

    if bgCol then
      lcd.drawFilledRectangle(px - 2, y - 1, LCD_W - 36, lineH, bgCol)
    end

    local ageS = round((msNow() - a.time) / 1000)
    local timeStr = ""
    if ageS < 60 then
      timeStr = string.format("%ds", ageS)
    else
      timeStr = string.format("%dm", round(ageS / 60))
    end

    local levelStr = a.level >= 2 and "CRIT" or (a.level >= 1 and "WARN" or "INFO")
    lcd.drawText(px, y, string.format("[%s] %s  — %s", levelStr, timeStr, a.text), SMLSIZE + col)
  end

  lcd.drawText(px, LCD_H - 20, string.format("Total alerts: %d    ENTER=Calibrate", #DETECT.alerts), SMLSIZE + C_GRAY)
end

-- ============================================================================
-- Main drawing dispatcher
-- ============================================================================
local function drawScreen()
  lcd.clear()

  if IS_COLOR then
    lcd.drawFilledRectangle(0, 0, LCD_W, LCD_H, C_BG)
    if STATE.page == 0 then drawRadarColor()
    elseif STATE.page == 1 then drawLinkColor()
    elseif STATE.page == 2 then drawSpectrumColor()
    elseif STATE.page == 3 then drawAlertsColor()
    end
  else
    if STATE.page == 0 then drawRadarMono()
    elseif STATE.page == 1 then drawLinkMono()
    elseif STATE.page == 2 then drawSpectrumMono()
    elseif STATE.page == 3 then drawAlertsMono()
    end
  end
end

-- ============================================================================
-- Input handling
-- ============================================================================
local function handleInput(event)
  if event == EVT_VIRTUAL_NEXT or event == EVT_PLUS_BREAK or event == EVT_ROT_RIGHT then
    STATE.page = (STATE.page + 1) % STATE.maxPages
  elseif event == EVT_VIRTUAL_PREV or event == EVT_MINUS_BREAK or event == EVT_ROT_LEFT then
    STATE.page = (STATE.page - 1) % STATE.maxPages
  elseif event == EVT_VIRTUAL_ENTER or event == EVT_ENTER_BREAK then
    -- Calibrate
    if not DETECT.calibrating then
      startCalibration()
    end
  elseif event == EVT_VIRTUAL_EXIT or event == EVT_EXIT_BREAK then
    return 1  -- Exit script
  end
  return 0
end

-- ============================================================================
-- EdgeTX/OpenTX Script Interface
-- ============================================================================

local function init()
  -- Detect screen capabilities
  detectScreen()
  setupColors()

  -- Initialize history buffers
  for i = 1, DETECT.jammingHistLen do DETECT.jammingHistory[i] = 100 end
  for i = 1, DETECT.snrHistLen do DETECT.snrHistory[i] = 10 end
  for i = 1, DETECT.rssiHistLen do
    DETECT.rssi1History[i] = -80
    DETECT.rssi2History[i] = -80
  end
  for i = 1, DETECT.antSwitchWinLen do DETECT.antSwitchWindow[i] = 0 end
  for i = 1, SPECTRUM.width do
    SPECTRUM.data[i] = {r1 = -80, r2 = -80, lq = 100, snr = 10, jam = 0}
  end

  STATE.initDone = true
  addAlert(0, "SkySweep32 INIT OK")
end

local function run(event)
  if not STATE.initDone then
    init()
  end

  -- Handle user input
  local exitCode = handleInput(event)
  if exitCode == 1 then return 2 end  -- Return 2 = exit tool script

  -- Read telemetry data
  readTelemetry()

  -- Poll CRSF device
  crsfPoll()

  -- Ping device periodically
  if not DEVICE.queried then
    crsfPing()
  end

  -- Run detection engine
  updateDetection()

  -- Update calibration if active
  updateCalibration()

  -- Draw UI
  drawScreen()

  STATE.frameCount = STATE.frameCount + 1
  return 0
end

return { init = init, run = run }
