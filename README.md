# 🐍 SnakeGame Win32

A high-performance, native Windows Snake game written in **C++** using the **Win32 API**. No bloated engines, no external dependencies—just pure Windows power and custom-rendered graphics.

## 🚀 Features
* **Pure Win32 API:** Direct interaction with the Windows OS for ultra-low latency.
* **Custom UI Engine:** Hand-crafted menu system, settings, and game-over states.
* **GDI+ Rendering:** Smooth, flicker-free graphics using double buffering.
* **DPI Aware:** Includes an application manifest to ensure crisp visuals on 4K monitors.
* **Static Build:** Compiled to run on any Windows machine without needing extra DLLs.

## 🛠 Prerequisites
To compile this project from source, you need:
* **MinGW-w64** (specifically `g++` and `windres`).
* **Windows OS** (10 or 11 recommended).

## 🔨 How to Compile

I've included a `build.bat` script to automate the process, but here is what’s happening under the hood:

### 1. Compile Resources
First, we turn the `.rc` file (icon, version info, manifest) into a linkable object:
```bash
windres resource.rc -o resource.o
