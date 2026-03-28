@echo off
chcp 65001 >nul
color 0a

echo ==========================================================
echo        SKY SWEEP 32 - АВТОМАТИЧЕСКАЯ УСТАНОВКА (v0.4.0)
echo ==========================================================
echo.
echo Этот скрипт установит прошивку радара на вашу плату ESP32.
echo Вам не нужны знания программирования или VSCode.
echo.
echo ШАГ 1: Пожалуйста, подключите вашу плату ESP32 к USB порту.
echo ШАГ 2: Дождитесь, пока Windows определит устройство.
echo.
pause

echo.
echo Поиск подключенных устройств...
echo.

:: Поиск COM портов через PowerShell
for /f "tokens=*" %%a in ('powershell -command "$ports = [System.IO.Ports.SerialPort]::GetPortNames(); if ($ports.Count -eq 0) { Write-Output 'NONE' } else { Write-Output $ports[0] }"') do set COMPORT=%%a

if "%COMPORT%"=="NONE" (
    color 0c
    echo [ОШИБКА] Устройство не найдено! Проверьте кабель напрямую (не через хаб).
    echo Возможно, вам нужно установить драйвер CH340 или CP2102.
    pause
    exit /b
)

echo [ОК] Найдена плата на порту: %COMPORT%
echo.

:: Меню выбора версии
echo ВЫБЕРИТЕ ВЕРСИЮ ДЛЯ УСТАНОВКИ:
echo [1] Starter  (Базовая версия: только ESP32 + OLED + NRF24)
echo [2] Hunter   (Стандартная версия: База + CC1101 + RX5808)
echo [3] Sentinel (Продвинутая версия: Стандарт + GPS + Логирование)
echo.
set /p tier="Введите цифру (1, 2, или 3) и нажмите ENTER: "

if "%tier%"=="1" set BINFILE=releases\v0.4.0\SkySweep32_Starter_v0.4.0.bin
if "%tier%"=="2" set BINFILE=releases\v0.4.0\SkySweep32_Hunter_v0.4.0.bin
if "%tier%"=="3" set BINFILE=releases\v0.4.0\SkySweep32_Sentinel_v0.4.0.bin

if not exist "%BINFILE%" (
    color 0e
    echo [ПРЕДУПРЕЖДЕНИЕ] Файл %BINFILE% не найден!
    echo Вы пропустили шаг сборки (build), либо файл перемещен.
    echo.
    echo [АЛЬТЕРНАТИВА]: Воспользуйтесь веб-интерфейсом для прошивки: https://espressif.github.io/esptool-js/
    pause
    exit /b
)

echo.
echo Начинаем прошивку %BINFILE% на порт %COMPORT%...
echo ВНИМАНИЕ: Не отключайте кабель! Это займет около минуты.
echo.

:: Проверка наличия esptool (используем powershell, если python нет)
where python >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo Python найден. Устанавливаем esptool...
    python -m pip install esptool >nul 2>nul
    python -m esptool --chip esp32 --port %COMPORT% --baud 460800 write_flash -z 0x10000 %BINFILE%
) else (
    color 0e
    echo [ПРЕДУПРЕЖДЕНИЕ] Python не установлен на этом компьютере.
    echo Скрипт не может автоматически запустить esptool.
    echo.
    echo Для прошивки в 1 клик прямо в браузере (Chrome/Edge):
    echo Откройте сайт: https://espressif.github.io/esptool-js/
    echo 1. Установите Baudrate: 460800
    echo 2. Flash Address: 0x10000
    echo 3. Выберите файл %BINFILE% и нажмите Program.
)

echo.
echo ==========================================================
echo ГОТОВО!
echo Откройте на телефоне WiFi сеть SkySweep32_AP (пароль: skysweep)
echo и зайдите по адресу: http://192.168.4.1
echo ==========================================================
pause
