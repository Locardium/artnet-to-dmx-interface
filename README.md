# Artnet to DMX Interfaces

**Artnet to DMX Interfaces** converts Art-Net signals to DMX and sends them out via an FTDI-based DMX interface over a serial port, with a lightweight Dear ImGui + DirectX 11 UI.

---

## Showcase

![Photo](https://i.imgur.com/dqhfiNd.png)

---

## Features

- **Art-Net → DMX**  
  Listens for Art-Net (UDP) packets on your LAN and forwards up to 512 DMX channels.
- **Interactive UI**   
  - Select network interface (bind to single IP or `0.0.0.0` for all)  
  - Choose DMX universe  
  - Set refresh rate (ms)  
  - Toggle debug console for error logging
- **Auto Network Adapter**  
  Automatically create a loopback network adapter dedicated for Art-Net if none exists.
- **MA2 Parameters Release**  
  Option to simulate nodes for free MA2 parameters so Art-Net can run without conflicts.  
- **Auto-save**  
  Settings are loaded/saved automatically to an INI file. (`C:\Windows\temp\locos-artnet-to-dmx-config.ini`) 

---

## Requirements

- **OS:** Windows 10 (tested)  
  - Windows 11 should work  
- **Graphics:** DirectX 11 runtime  
- **Runtime:** [Microsoft Visual C++ 2015–2022 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) (_vc_redist.x64)_
- **DMX Interface:** FTDI-chip-based interface (e.g. Enttec Open DMX, KM-LED) with drivers installed 

---

## Usage

1. Download the exe from the latest [release](https://github.com/Locardium/artnet-to-dmx/releases).  
2. Run `artnet-to-dmx.exe`  
3. In **Status**, wait for the interface to connect  
4. Select your **Network Interface** (or leave `0.0.0.0` to listen on all)
   a. If no Art-Net adapter exists, use **Install** to generate one automatically  
5. Set **Universe** (default `1`) and **Refresh Rate** (default `25 ms`)  
6. (Optional) Use **MA2 Nodes** for realease MA2 parameters  
7. Click **Start** to begin sending DMX data over the FTDI COM port
