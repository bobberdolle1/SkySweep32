-- ============================================================================
-- SkySweep32 Widget — EdgeTX Color Screen Widget
-- Place in: /WIDGETS/SkySweep/main.lua on SD card
-- Compatible: EdgeTX 2.8+ (color screen radios only)
-- Radios: TX16S, Boxer, Zorro, Pocket, Horus X10/X12, etc.
-- ============================================================================

local name = "SkySweep"
local options = {
  { "Mode", VALUE, 0, 0, 2 },     -- 0=radar, 1=bars, 2=compact
  { "Color", COLOR, GREEN },
}

-- ============================================================================
-- Sensor & state
-- ============================================================================
local sCache = {}
local data = {
  rssi1 = 0, rssi2 = 0, rqly = 0, rsnr = 0,
  ant = 0, rfmd = 0, tpwr = 0,
  jamScore = 0,
  envScore = 100,
  sweepAngle = 0,
}

local rssiHist = {}
local histLen = 30
local histIdx = 1
local lqHist = {}
local lqIdx = 1

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

local function clamp(v,lo,hi) if v<lo then return lo end; if v>hi then return hi end; return v end

-- ============================================================================
-- Widget lifecycle
-- ============================================================================

local function create(zone, opt)
  local w = {zone=zone, options=opt}
  for i=1,histLen do rssiHist[i]=-80; lqHist[i]=100 end
  return w
end

local function update(w, opt)
  w.options = opt
end

local function background(w)
  data.rssi1 = gv("1RSS") or gv("RSSI") or 0
  data.rssi2 = gv("2RSS") or 0
  data.rqly  = gv("RQly") or 0
  data.rsnr  = gv("RSNR") or 0
  data.ant   = gv("ANT") or 0
  data.rfmd  = gv("RFMD") or 0
  data.tpwr  = gv("TPWR") or 0

  -- Read jamming score from background script's GV
  data.jamScore = model.getGlobalVariable(8, 0) or 0

  -- Store RSSI history
  rssiHist[histIdx] = data.rssi1
  lqHist[lqIdx] = data.rqly
  histIdx = (histIdx % histLen) + 1
  lqIdx = (lqIdx % histLen) + 1

  -- Compute env score
  local env = 100
  env = env - clamp((100 - data.rqly) * 0.5, 0, 30)
  if data.rsnr < 5 then env = env - clamp((5 - data.rsnr) * 3, 0, 25) end
  env = env - data.jamScore * 0.3
  data.envScore = clamp(math.floor(env + 0.5), 0, 100)

  -- Sweep animation
  data.sweepAngle = (data.sweepAngle + 3) % 360
end

-- ============================================================================
-- Drawing
-- ============================================================================

local function drawRadar(w, x, y, width, height)
  local cx = x + math.floor(width / 2)
  local cy = y + math.floor(height / 2)
  local cr = math.min(width, height) / 2 - 4

  -- Colors
  local cBg    = lcd.RGB(5, 18, 8)
  local cGrid  = lcd.RGB(20, 60, 30)
  local cSweep = lcd.RGB(0, 200, 60)
  local cBlip  = lcd.RGB(255, 40, 40)
  local cText  = lcd.RGB(180, 220, 190)

  -- Background
  lcd.drawFilledRectangle(x, y, width, height, cBg)

  -- Radar rings
  for r = 1, 3 do
    lcd.drawCircle(cx, cy, math.floor(cr * r / 3), cGrid)
  end
  lcd.drawCircle(cx, cy, math.floor(cr), cGrid)

  -- Crosshair
  lcd.drawLine(cx - math.floor(cr), cy, cx + math.floor(cr), cy, DOTTED, cGrid)
  lcd.drawLine(cx, cy - math.floor(cr), cx, cy + math.floor(cr), DOTTED, cGrid)

  -- Sweep line
  local sa = math.rad(data.sweepAngle)
  local sx = cx + math.floor(math.cos(sa) * cr)
  local sy = cy - math.floor(math.sin(sa) * cr)
  lcd.drawLine(cx, cy, sx, sy, SOLID, cSweep)

  -- Trailing fade (3 lines behind sweep)
  for i = 1, 3 do
    local fa = math.rad((data.sweepAngle - i * 8) % 360)
    local alpha = math.floor(200 / (i + 1))
    local fc = lcd.RGB(0, alpha, math.floor(alpha / 3))
    local fx = cx + math.floor(math.cos(fa) * cr)
    local fy = cy - math.floor(math.sin(fa) * cr)
    lcd.drawLine(cx, cy, fx, fy, SOLID, fc)
  end

  -- Signal strength blip at center (own signal)
  local sigR = clamp(math.floor((data.rssi1 + 130) / 100 * cr), 3, math.floor(cr * 0.8))
  lcd.drawCircle(cx, cy, sigR, lcd.RGB(0, 150, 200))

  -- Jamming indicator blip
  if data.jamScore > 20 then
    -- Direction estimation: compare antennas
    local delta = (data.rssi1 or 0) - (data.rssi2 or 0)
    local angle = 0
    if delta > 3 then angle = 90
    elseif delta < -3 then angle = 270
    else angle = 0 end

    local dist = clamp(math.floor(cr * (1 - data.jamScore / 120)), 5, math.floor(cr - 5))
    local bx = cx + math.floor(math.cos(math.rad(angle)) * dist)
    local by = cy - math.floor(math.sin(math.rad(angle)) * dist)

    lcd.drawFilledRectangle(bx - 3, by - 3, 7, 7, cBlip)

    if data.jamScore > 50 then
      lcd.drawText(bx + 6, by - 6, "!", SMLSIZE + BLINK + cBlip)
    end
  end

  -- Corner labels
  lcd.drawText(x + 2, y + 2, string.format("E:%d%%", data.envScore), SMLSIZE + cText)
  lcd.drawText(x + 2, y + height - 14,
    string.format("J:%d", data.jamScore), SMLSIZE +
    (data.jamScore > 50 and cBlip or cText))
end

local function drawBars(w, x, y, width, height)
  local cBg   = lcd.RGB(12, 14, 20)
  local cText = lcd.RGB(200, 210, 220)
  local cGrn  = lcd.RGB(0, 200, 80)
  local cYel  = lcd.RGB(255, 200, 0)
  local cRed  = lcd.RGB(255, 40, 40)
  local cCyan = lcd.RGB(0, 180, 240)
  local cGray = lcd.RGB(50, 55, 65)

  lcd.drawFilledRectangle(x, y, width, height, cBg)

  local px = x + 4
  local py = y + 2
  local barMaxW = width - 50

  -- ANT 1 RSSI
  lcd.drawText(px, py, "R1", SMLSIZE + cText)
  local r1 = clamp(math.floor((data.rssi1 + 130) / 100 * barMaxW), 0, barMaxW)
  local r1c = r1 > barMaxW*0.6 and cGrn or (r1 > barMaxW*0.3 and cYel or cRed)
  lcd.drawFilledRectangle(px + 18, py, r1, math.floor(height/7), r1c)
  lcd.drawRectangle(px + 18, py, barMaxW, math.floor(height/7), cGray)
  lcd.drawText(px + barMaxW + 20, py, tostring(math.floor(data.rssi1)), SMLSIZE + cText)

  -- ANT 2 RSSI
  py = py + math.floor(height / 6)
  lcd.drawText(px, py, "R2", SMLSIZE + cText)
  local r2 = clamp(math.floor((data.rssi2 + 130) / 100 * barMaxW), 0, barMaxW)
  local r2c = r2 > barMaxW*0.6 and cGrn or (r2 > barMaxW*0.3 and cYel or cRed)
  lcd.drawFilledRectangle(px + 18, py, r2, math.floor(height/7), r2c)
  lcd.drawRectangle(px + 18, py, barMaxW, math.floor(height/7), cGray)
  lcd.drawText(px + barMaxW + 20, py, tostring(math.floor(data.rssi2)), SMLSIZE + cText)

  -- LQ bar
  py = py + math.floor(height / 6)
  lcd.drawText(px, py, "LQ", SMLSIZE + cText)
  local lqW = clamp(math.floor(data.rqly / 100 * barMaxW), 0, barMaxW)
  local lqc = data.rqly > 80 and cGrn or (data.rqly > 50 and cYel or cRed)
  lcd.drawFilledRectangle(px + 18, py, lqW, math.floor(height/7), lqc)
  lcd.drawRectangle(px + 18, py, barMaxW, math.floor(height/7), cGray)
  lcd.drawText(px + barMaxW + 20, py, data.rqly.."%", SMLSIZE + cText)

  -- JAM bar
  py = py + math.floor(height / 6)
  lcd.drawText(px, py, "JM", SMLSIZE + cText)
  local jW = clamp(math.floor(data.jamScore / 100 * barMaxW), 0, barMaxW)
  local jc = data.jamScore > 50 and cRed or (data.jamScore > 25 and cYel or cGrn)
  lcd.drawFilledRectangle(px + 18, py, jW, math.floor(height/7), jc)
  lcd.drawRectangle(px + 18, py, barMaxW, math.floor(height/7), cGray)

  -- Bottom line: RF mode, power, env score
  py = y + height - 14
  lcd.drawText(px, py, string.format("%s %dmW E:%d%%",
    ELRS_RATES[data.rfmd] or "?", data.tpwr, data.envScore), SMLSIZE + cCyan)
end

local function drawCompact(w, x, y, width, height)
  local cBg   = lcd.RGB(12, 14, 20)
  local cText = lcd.RGB(200, 210, 220)
  local cGrn  = lcd.RGB(0, 200, 80)
  local cYel  = lcd.RGB(255, 200, 0)
  local cRed  = lcd.RGB(255, 40, 40)

  lcd.drawFilledRectangle(x, y, width, height, cBg)

  local envCol = data.envScore > 70 and cGrn or (data.envScore > 40 and cYel or cRed)
  local jamCol = data.jamScore > 50 and cRed or (data.jamScore > 25 and cYel or cGrn)

  if height < 30 then
    -- Ultra compact: single line
    lcd.drawText(x+2, y+2, string.format("E:%d%% J:%d LQ:%d%%",
      data.envScore, data.jamScore, data.rqly), SMLSIZE + envCol)
  else
    -- Two lines
    lcd.drawText(x+2, y+2, string.format("ENV: %d%%", data.envScore), SMLSIZE + envCol)
    lcd.drawText(x+2, y + math.floor(height/2), string.format("JAM: %d  LQ:%d%%",
      data.jamScore, data.rqly), SMLSIZE + jamCol)
  end
end

local function refresh(w, event, touchState)
  background(w)

  local mode = w.options.Mode or 0
  local x, y = w.zone.x, w.zone.y
  local width, height = w.zone.w, w.zone.h

  if mode == 0 then
    drawRadar(w, x, y, width, height)
  elseif mode == 1 then
    drawBars(w, x, y, width, height)
  else
    drawCompact(w, x, y, width, height)
  end
end

return {
  name = name,
  options = options,
  create = create,
  update = update,
  background = background,
  refresh = refresh,
}
