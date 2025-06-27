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
  - Toggle debug console for errors logging  
- **Auto-save**  
  Settings are loaded/saved automatically to an INI file. (`C:\Windows\temp\locos-artnet-to-dmx-config.ini`)

---

## Requirements

- **OS:** Windows 10 (tested)  
  - Windows 11 should work  
- **Graphics:** DirectX 11 runtime  
- **Runtime:** Microsoft Visual C++ 2015–2022 Redistributable  
- **DMX Interface:** FTDI-chip-based interface (e.g. Enttec Open DMX, KM-LED) with drivers installed 

---

## Usage

1. Download the exe from latest [release](https://github.com/Locardium/artnet-to-dmx/releases).  
2. Run `artnet-to-dmx.exe`
3. In **Status**, wait for the interface to connect  
4. Select your **Network Interface** (or leave `0.0.0.0` to listen on all)
5. Set **Universe** (default `1`) and **Refresh Rate** (default `25 ms`)
6. (Optional) Check **Show debug console** to view errors logs
7. Click **Start** to begin sending DMX data over the FTDI COM port
