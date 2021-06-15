This code is for ESP32, its take UART telemetry from Victron Solar chargers and relay the data to MQTT server.


This code is based on project: http://www.svpartyoffive.com/2018/02/28/victron-monitors-technical/

Be advide Victron UART port use 5V logic levels, ESP32 is designed for 3.3V logic levels

Use logic level converter to avoid damage your ESP32!!!

define UART2 at pins 16 and 17



PID 0xA043      -- Product ID for BlueSolar MPPT 100/15

FW  119     -- Firmware version of controller, v1.19

SER#  HQXXXXXXXXX   -- Serial number

V 13790     -- Battery voltage, mV

I -10     -- Battery current, mA

VPV 15950     -- Panel voltage, mV

PPV 0     -- Panel power, W

CS  5     -- Charge state, 0 to 9

ERR 0     -- Error code, 0 to 119

LOAD  ON      -- Load output state, ON/OFF

IL  0     -- Load current, mA

H19 0       -- Yield total, kWh

H20 0     -- Yield today, kWh

H21 397     -- Maximum power today, W

H22 0       -- Yield yesterday, kWh

H23 0     -- Maximum power yesterday, W

HSDS  0     -- Day sequence number, 0 to 365

Checksum  l:A0002000148   -- Message checksum


MQTT topics

victron/mode      reports the charger mode 0=OFF, 1=LOW_Power, 2=fault, 3=BULK, 4=Absorption, 5=FLOAT, 6=Inverter

victron/solar/w   reports the power of the solar panels

victron/solar/v   reports the voltage of the solar panels

victron/batery/c  reports the charging current

victron/batery/v  reports the battery voltage

Tags: Color Control GX CCGX Solar charger Victron MQTT MPPT
