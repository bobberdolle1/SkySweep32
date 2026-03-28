-- ============================================================================
-- SkySweep32 — Telemetry Screen Script
-- For mono screen radios (128x64, 212x64)
-- Place in: /SCRIPTS/TELEMETRY/ on SD card
-- Assign in: Model → Telemetry → Screen X → Script → skysweep_t
-- Compatible: EdgeTX 2.8+, OpenTX 2.3.15+
-- ============================================================================
-- This script provides a live telemetry screen with RF monitoring.
-- Unlike the TOOLS script, this runs as a telemetry page you can
-- scroll to during flight using the PAGE button.
-- ============================================================================

local sCache = {}
local rssiHist1 = {}
local rssiHist2 = {}
local lqHist = {}
local histLen = 30
local histIdx = 1
local jammingScore = 0
local initialized = false
local page = 0  -- 0=overview, 1=graph

local ELRS_RATES = {
  [0]="4Hz",[1]="25Hz",[2]="50Hz",[3]="100Hz",[4]="150Hz",
  [5]="200Hz",[6]="250Hz",[7]="333Hz",[8]="500Hz",[9]="D250",
  [10]="D500",[11]="F500",[12]="F1K"
}

local function gv(n)
  if not sCache[n] then
    local f = getFieldInfo(n)
    if f then sCache[n] = f.id else return nil end
  end
  return getValue(sCache[n])
end

-- Polyfill: lcd.drawCircle for older OpenTX
if not lcd.drawCircle then
  lcd.drawCircle = function(cx, cy, r)
    local x, y, d = r, 0, 1 - r
    while x >= y do
      lcd.drawPoint(cx+x,cy+y); lcd.drawPoint(cx+y,cy+x)
      lcd.drawPoint(cx-y,cy+x); lcd.drawPoint(cx-x,cy+y)
      lcd.drawPoint(cx-x,cy-y); lcd.drawPoint(cx-y,cy-x)
      lcd.drawPoint(cx+y,cy-x); lcd.drawPoint(cx+x,cy-y)
      y = y + 1
      if d < 0 then d = d + 2*y + 1
      else x = x - 1; d = d + 2*(y-x) + 1 end
    end
  end
end

local function clamp(v,lo,hi) if v<lo then return lo end; if v>hi then return hi end; return v end

local function init()
  for i = 1, histLen do
    rssiHist1[i] = -80
    rssiHist2[i] = -80
    lqHist[i] = 100
  end
  initialized = true
end

local function run(event)
  if not initialized then init() end

  -- Input
  if event == EVT_PLUS_BREAK or event == EVT_ROT_RIGHT or event == EVT_VIRTUAL_NEXT then
    page = 1 - page
  elseif event == EVT_MINUS_BREAK or event == EVT_ROT_LEFT or event == EVT_VIRTUAL_PREV then
    page = 1 - page
  end

  -- Read telemetry
  local rssi1 = gv("1RSS") or gv("RSSI") or 0
  local rssi2 = gv("2RSS") or 0
  local lq    = gv("RQly") or 0
  local snr   = gv("RSNR") or 0
  local ant   = gv("ANT") or 0
  local rfmd  = gv("RFMD") or 0
  local tpwr  = gv("TPWR") or 0

  -- Store history
  rssiHist1[histIdx] = rssi1
  rssiHist2[histIdx] = rssi2
  lqHist[histIdx] = lq
  histIdx = (histIdx % histLen) + 1

  -- Compute jamming score inline (simplified)
  local jam = 0
  if lq < 70 then jam = jam + math.min((70 - lq) * 2, 40) end
  if snr < 0 then jam = jam + math.min(math.abs(snr) * 3, 30) end
  -- LQ variance
  local lqSum, lqCnt = 0, 0
  for i = 1, histLen do
    if lqHist[i] then lqSum = lqSum + lqHist[i]; lqCnt = lqCnt + 1 end
  end
  local lqAvg = lqCnt > 0 and (lqSum / lqCnt) or 100
  local lqVarSum = 0
  for i = 1, histLen do
    if lqHist[i] then
      local d = lqHist[i] - lqAvg
      lqVarSum = lqVarSum + d * d
    end
  end
  local lqVar = lqCnt > 1 and (lqVarSum / (lqCnt - 1)) or 0
  if lqVar > 200 then jam = jam + math.min((lqVar - 200) / 20, 15) end
  jammingScore = math.min(math.floor(jam + 0.5), 100)

  -- Draw
  lcd.clear()

  if page == 0 then
    -- ======== OVERVIEW PAGE ========
    -- Header
    lcd.drawText(0, 0, "SkySweep", SMLSIZE)
    lcd.drawText(58, 0, "OVR", SMLSIZE + INVERS)
    lcd.drawLine(0, 8, 127, 8, SOLID, FORCE)

    -- Radar mini (left side)
    local cx, cy, cr = 24, 38, 20
    lcd.drawCircle(cx, cy, cr)
    lcd.drawCircle(cx, cy, math.floor(cr * 0.5))
    lcd.drawLine(cx - cr, cy, cx + cr, cy, DOTTED, 0)
    lcd.drawLine(cx, cy - cr, cx, cy + cr, DOTTED, 0)

    -- Sweep
    local sweep = (getTime() * 3) % 360
    local sx = cx + math.floor(math.cos(math.rad(sweep)) * cr)
    local sy = cy - math.floor(math.sin(math.rad(sweep)) * cr)
    lcd.drawLine(cx, cy, sx, sy, SOLID, FORCE)

    -- Signal ring
    local sigR = clamp(math.floor((rssi1 + 130) / 100 * cr), 2, cr - 2)
    lcd.drawCircle(cx, cy, sigR)

    -- Jamming blip
    if jammingScore > 25 then
      local delta = rssi1 - rssi2
      local bAngle = 0
      if delta > 3 then bAngle = 90
      elseif delta < -3 then bAngle = 270 end
      local bDist = clamp(math.floor(cr * (1 - jammingScore / 120)), 3, cr - 3)
      local bx = cx + math.floor(math.cos(math.rad(bAngle)) * bDist)
      local by = cy - math.floor(math.sin(math.rad(bAngle)) * bDist)
      lcd.drawFilledRectangle(bx - 2, by - 2, 5, 5)
    end

    -- Right panel
    local rx = 52
    lcd.drawText(rx, 10, "R1:", SMLSIZE)
    lcd.drawText(rx + 16, 10, tostring(math.floor(rssi1)), SMLSIZE)

    lcd.drawText(rx, 18, "R2:", SMLSIZE)
    lcd.drawText(rx + 16, 18, tostring(math.floor(rssi2)), SMLSIZE)

    lcd.drawText(rx, 26, "LQ:", SMLSIZE)
    lcd.drawText(rx + 16, 26, lq .. "%", SMLSIZE)

    lcd.drawText(rx, 34, "SN:", SMLSIZE)
    lcd.drawText(rx + 16, 34, tostring(snr) .. "dB", SMLSIZE)

    lcd.drawText(rx, 42, "RF:", SMLSIZE)
    lcd.drawText(rx + 16, 42, ELRS_RATES[rfmd] or "?", SMLSIZE)

    lcd.drawText(rx, 50, "PW:", SMLSIZE)
    lcd.drawText(rx + 16, 50, tpwr .. "mW", SMLSIZE)

    -- Antenna indicator
    lcd.drawText(100, 10, "A" .. ant, SMLSIZE + INVERS)

    -- Jamming bar (bottom)
    lcd.drawText(0, 58, "J:", SMLSIZE)
    local jW = clamp(math.floor(jammingScore * 0.9), 0, 90)
    lcd.drawRectangle(12, 58, 92, 6)
    if jW > 0 then lcd.drawFilledRectangle(13, 59, jW, 4) end
    if jammingScore > 50 then lcd.drawText(106, 58, "!", SMLSIZE + BLINK) end
    lcd.drawText(112, 58, tostring(jammingScore), SMLSIZE)

  else
    -- ======== GRAPH PAGE ========
    lcd.drawText(0, 0, "SkySweep", SMLSIZE)
    lcd.drawText(58, 0, "GRF", SMLSIZE + INVERS)
    lcd.drawLine(0, 8, 127, 8, SOLID, FORCE)

    -- Graph area
    local gx, gy, gw, gh = 0, 10, 128, 38

    lcd.drawRectangle(gx, gy, gw, gh)
    lcd.drawLine(gx, gy + math.floor(gh/3), gx + gw - 1, gy + math.floor(gh/3), DOTTED, 0)
    lcd.drawLine(gx, gy + math.floor(gh*2/3), gx + gw - 1, gy + math.floor(gh*2/3), DOTTED, 0)

    -- Plot RSSI history
    local stepW = (gw - 4) / (histLen - 1)
    local prevX, prevY1, prevY2

    for i = 0, histLen - 1 do
      local idx = ((histIdx - 1 + i) % histLen) + 1
      local r1 = rssiHist1[idx]
      local r2 = rssiHist2[idx]

      if r1 and r1 ~= 0 then
        local px = gx + 2 + math.floor(i * stepW + 0.5)
        local py1 = gy + gh - 2 - math.floor(clamp((r1 + 120) / 80, 0, 1) * (gh - 4))

        if prevX then
          lcd.drawLine(prevX, prevY1, px, py1, SOLID, FORCE)
        end
        prevX = px
        prevY1 = py1

        if r2 and r2 ~= 0 then
          local py2 = gy + gh - 2 - math.floor(clamp((r2 + 120) / 80, 0, 1) * (gh - 4))
          if prevY2 then
            lcd.drawLine(prevX - math.floor(stepW + 0.5), prevY2, px, py2, DOTTED, FORCE)
          end
          prevY2 = py2
        end
      end
    end

    -- LQ bar
    lcd.drawText(0, 50, "LQ:", SMLSIZE)
    local lqW = clamp(math.floor(lq * 0.9), 0, 90)
    lcd.drawRectangle(16, 50, 92, 6)
    if lqW > 0 then lcd.drawFilledRectangle(17, 51, lqW, 4) end
    lcd.drawText(110, 50, lq.."%", SMLSIZE)

    -- Jamming bar
    lcd.drawText(0, 58, "JM:", SMLSIZE)
    local jW = clamp(math.floor(jammingScore * 0.9), 0, 90)
    lcd.drawRectangle(16, 58, 92, 6)
    if jW > 0 then lcd.drawFilledRectangle(17, 59, jW, 4) end
    lcd.drawText(110, 58, tostring(jammingScore), SMLSIZE)
  end

  return 0
end

return { init = init, run = run }
