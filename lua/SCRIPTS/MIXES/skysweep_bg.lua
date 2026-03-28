-- ============================================================================
-- SkySweep32 — Background Mixer Script
-- Continuous ELRS/CRSF monitoring, audio alerts, global state
-- Place in: /SCRIPTS/MIXES/ on SD card
-- Enable in: Model → Lua Scripts → skysweep_bg
-- Compatible: EdgeTX 2.8+, OpenTX 2.3.15+
-- ============================================================================

-- Shared state via globals (accessed by widget)
local gvIndex = 8  -- Global Variable index for jamming score (GV9)

-- Detection state
local lqHistory = {}
local snrHistory = {}
local histLen = 20
local histIdx = 1
local lastLQ = 100
local lastSNR = 10
local lastAnt = -1
local antSwitches = 0
local jammingScore = 0
local lastAlertMs = 0
local rateDowngrades = 0
local lastRfmd = -1
local lqDropEvents = 0
local initialized = false

-- Sensor cache
local sCache = {}

local function gv(name)
  if not sCache[name] then
    local f = getFieldInfo(name)
    if f then sCache[name] = f.id else return nil end
  end
  return getValue(sCache[name])
end

local function init()
  for i = 1, histLen do
    lqHistory[i] = 100
    snrHistory[i] = 10
  end
  initialized = true
end

local function run()
  if not initialized then init() end

  -- Read sensors
  local lq = gv("RQly") or 0
  local snr = gv("RSNR") or 0
  local ant = gv("ANT") or 0
  local rfmd = gv("RFMD") or 0
  local rssi1 = gv("1RSS") or gv("RSSI") or 0
  local rssi2 = gv("2RSS") or 0

  if lq == 0 and rssi1 == 0 then return end  -- No telemetry

  -- Store history
  lqHistory[histIdx] = lq
  snrHistory[histIdx] = snr
  histIdx = (histIdx % histLen) + 1

  -- Antenna switch tracking
  if lastAnt >= 0 and ant ~= lastAnt then
    antSwitches = antSwitches + 1
  end
  lastAnt = ant

  -- Rate downgrade tracking
  if lastRfmd >= 0 and rfmd < lastRfmd then
    rateDowngrades = rateDowngrades + 1
  end
  lastRfmd = rfmd

  -- LQ drop events
  if lq < 70 and lastLQ >= 70 then
    lqDropEvents = lqDropEvents + 1
  end
  lastLQ = lq

  -- ===========================
  -- Compute jamming score
  -- ===========================
  local score = 0

  -- LQ analysis
  if lq < 70 then
    score = score + math.min((70 - lq) * 2, 40)
  end

  -- SNR analysis
  if snr < 0 then
    score = score + math.min(math.abs(snr) * 3, 30)
  end

  -- LQ variance (compute)
  local lqSum, lqCnt = 0, 0
  for i = 1, histLen do
    if lqHistory[i] then lqSum = lqSum + lqHistory[i]; lqCnt = lqCnt + 1 end
  end
  local lqAvg = lqCnt > 0 and (lqSum / lqCnt) or 100
  local lqVarSum = 0
  for i = 1, histLen do
    if lqHistory[i] then
      local d = lqHistory[i] - lqAvg
      lqVarSum = lqVarSum + d * d
    end
  end
  local lqVar = lqCnt > 1 and (lqVarSum / (lqCnt - 1)) or 0

  if lqVar > 200 then
    score = score + math.min((lqVar - 200) / 20, 15)
  end

  -- Antenna switching penalty
  if antSwitches > 10 then
    score = score + 10
    antSwitches = 0  -- Reset window
  end

  -- Rate downgrade penalty
  if rateDowngrades > 0 then
    score = score + math.min(rateDowngrades * 5, 15)
    rateDowngrades = 0
  end

  jammingScore = math.min(math.floor(score + 0.5), 100)

  -- Store in global variable for widget access
  -- GV range is -1024..1024, we store jamming score 0-100
  model.setGlobalVariable(gvIndex, 0, jammingScore)

  -- Audio alerts
  local now = getTime() * 10
  if jammingScore >= 60 and (now - lastAlertMs) > 5000 then
    lastAlertMs = now
    playTone(1200, 300, 100, PLAY_NOW)
    playTone(1600, 200, 100)
    playTone(1200, 300, 100)
  elseif jammingScore >= 30 and (now - lastAlertMs) > 10000 then
    lastAlertMs = now
    playTone(800, 200, 80, PLAY_NOW)
  end
end

return { init = init, run = run, output = {} }
