# OpenSolder
![](/Hardware/Images/Front.png)  
Fully open source JBC T245-compatible soldering station and iron holder

## Introduction
When my cheap T12 clone soldering station went up in smoke, i started looking into either buying a JBC CDB, or building a proper soldering station. The internet is already crowded with DIY JBC compatible stations, but most are poorly documented, or does not have the quality-of-life features that the original have.

The [unisolder project](https://github.com/sparkybg/UniSolder-5.2) is absolutely impressive, but waaay to complex and "universal" for my needs. [Marco Reps](https://youtu.be/GYIiOkr6x9o) built a cool C470 station, and i liked the simplicity of that.

## Goals:
- _T12 clone like_ form factor station that does not take up valuable bench space
- JBC T245 tip compatibility, with similar performance to the original station
- QoL features that the CDB station have, like auto standby, tip remover, holder, tip cleaner ect.
- Simple design using cheap off-the-shelf parts
- A compact handle stand with these features:
	- Detection when the handle is in the tool holder (auto standby)
	- Detection when you're about to swap tips (cut power)
	- Integrated tip remover, tip storage, tip cleaner and a soldering wire spool holder
	- Simple to build

## Design Specifications
- 24V toroidal transformer
- Zero Crossing AC switching
- OLED display and rotary encoder for adjusting temperature
- Compact and low profile design
- Super fast heatup time
- Hardware should be mostly 3D printable, and easy to build using regular tools

## Station
![](/Hardware/Images/Station Page 1.png)  
The station is built in a Hammond enclosure, with a center mounted transformer. The PCB is mounted to the custom front panel, and all 230VAC connections is in the rear of the chassis.
There is two connectors on the rear panel, one for connecting to the stand, and an optional ST-link connector for firmware upgrades.

## Stand
![](/Hardware/Images/Stand Page 1.png)  
The stand consists of 3D-printed parts, a genuine JBC tool holder, a tip remover made from aluminium angle and some hardware. 

## Electronics

## Firmware
Coming soon... :)

## Links and Sources

[The unisolder project](https://github.com/sparkybg/UniSolder-5.2)  
[Marco Reps](https://youtu.be/GYIiOkr6x9o)  
[foldvarid93's JBC Soldering Station](https://github.com/foldvarid93/JBC_SolderingStation)  
[johnmx's eevblog thread](https://eevblog.com/forum/testgear/jbc-soldering-station-cd-2bc-complete-schematic-analysis/)  