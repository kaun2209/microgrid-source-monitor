# Microgrid Source Monitor (ESP32 simulation)

A low-voltage prototype modelling the home backup-power setup
where grid, battery inverter and generator switch automatically during blackouts.

**Priority logic:** GRID > GENERATOR > BATTERY

## How it works
ESP32 (simulated in Wokwi) -> MQTT (HiveMQ public broker) -> browser dashboard
- The ESP32 reads two switches (grid, generator) and picks the active source.
- Each change is published as JSON, e.g. {"source":"GRID","uptime_s":55}
- The dashboard shows the live source, an alert on grid loss, and a switchover log.

## Demo
- Wokwi project: https://wokwi.com/projects/479674477807654913

## Limitations / future work
- Simulated hardware; public broker with no authentication
- No stored history (could add a database, or AWS IoT Core with certificates)
- Next: real ESP32 with voltage sensing, debouncing, current measurement

## How to run
open the Wokwi link, press Play, open the dashboard, and flip a switch. Mention that the dashboard shows live data only while the simulation is running, and that the MQTT topic in both files must match.

