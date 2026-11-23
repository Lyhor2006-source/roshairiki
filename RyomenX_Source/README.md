# RyomenX VIP [iOS Native] — Reverse Engineered Source & Architecture

## Overview
Reconstructed C++17, Objective-C++, and ImGui source codebase extracted from `RyomenXCracked.dylib` (Rules of Survival, iOS arm64, NetEase NeoX / BigWorld Engine).

---

## 1. Architecture Highlights

### A. Dual-Engine Hybrid Pipeline
1. **Dynamic Python Runtime Interop Bridge (`PythonBridge` & `neox_battle_payload.py`)**:
   - Resolves Python 2.7 C-API runtime symbols dynamically from the game binary (`Py_IsInitialized`, `PyGILState_Ensure`, `PyRun_StringFlags`).
   - Hooks into BigWorld engine space (`BigWorld.player()`, `BigWorld.entities`).
   - Evaluates World-To-Screen (`_pb_w2s_get`), bone joints (`_pb_joint_screen`), health, stance, and enemy identification without fragile static memory offsets.
   - Outputs live entity stream to `Library/Caches/RyomenX/esp.txt`.

2. **Native iOS Metal & Dear ImGui Layer (`RyomenXOverlayView` & `IosUI`)**:
   - High performance Metal rendering (`MTKView` delegate) at native screen refresh rate (60/120 FPS).
   - Seamless Touch-Passthrough (`pointInside:withEvent:` returns `NO` for coordinates outside menus, allowing 100% normal gameplay controls).
   - Draggable HUD Floating Bar and full 5-tab main menu (`Visuals`, `Adv`, `Aim`, `Mem`, `Settings`).

3. **Darwin Mach VM Memory Scanner (`MachMemory` & `MemoryCheats`)**:
   - Direct memory region inspection using `vm_region_recurse_64`, `vm_read_overwrite`, and `vm_write`.
   - Multi-item Group Proximity Scanning (`GroupScan`) for weapon damage multipliers, zero recoil, fast reload, speed hack, fly hack, no grass, and no fall damage.
   - Background `FreezeThread` to lock player physics values continuously.

4. **Stealth & Device Spoofing (`StealthBypass`)**:
   - Hooks `NSFileManager fileExistsAtPath:` to hide jailbreak artifacts (`Cydia.app`, `Sileo.app`, `MobileSubstrate`, `/bin/bash`).
   - Hooks `UIDevice identifierForVendor` to generate randomized UUIDs on demand for instant hardware/device unban.

---

## 2. Directory Structure
```
RyomenX_Source/
├── CMakeLists.txt              # Cross-compilation manifest
├── README.md                   # Technical breakdown & guide
├── include/
│   ├── Config.hpp              # Cheat configuration & persistence
│   ├── Entity.hpp              # Player entity struct & 15 bone links
│   ├── IosUI.hpp               # ESP drawing headers
│   ├── MachMemory.hpp          # Mach VM memory read/write & group scan
│   ├── MemoryCheats.hpp        # 20 memory cheat features & worker
│   ├── Menu.h                  # Eclipse White-Theme ImGui menu definitions
│   ├── OverlayMath.hpp         # Vec2/3/4, EspBox, 2D/3D/Corner boxes
│   ├── PythonBridge.hpp        # Python 2.7 C-API dynamic resolver
│   └── StealthBypass.hpp       # Jailbreak hide & IDFV randomizer
├── scripts/
│   └── neox_battle_payload.py  # Extracted 68KB BigWorld Python engine payload
└── src/
    ├── Config.cpp              # Settings defaults & NSUserDefaults sync
    ├── IosUI.cpp               # ESP overlay draw loop & geometry
    ├── MachMemory.cpp          # Darwin VM virtual memory engine
    ├── Main.mm                 # Dylib constructor & window attachment
    ├── MemoryCheats.cpp        # 20 weapon/player/environment memory patches
    ├── Menu.cpp                # Eclipse ImGui engine, animated toggles & 5 tabs
    ├── OverlayMath.cpp         # 2D/3D/Corner box geometry calculations
    ├── PythonBridge.cpp        # BigWorld Python payload execution & file sync
    ├── RyomenXOverlayView.mm   # Metal MTKView, Eclipse theme & touch passthrough
    └── StealthBypass.mm        # Method swizzling & unban spoofing
```

---

## 3. Compiling & Packaging

### Option A: GitHub Actions Mac Cloud (Free macOS-14 Apple Silicon Runner)
កូដនៅក្នុង repo នេះមាន setup រួចជាស្រេចនូវ `.github/workflows/build.yml`។ គ្រាន់តែ push គម្រោងទៅ GitHub៖
1. GitHub នឹងដំណើរការ compile នៅលើ Cloud Mac (`macos-14` M1 / Xcode 15+)។
2. ទាញយក `RyomenX.dylib` ពីផ្ទាំង **Actions -> Artifacts**។
3. File dylib ត្រូវបាន fake-sign ដោយ `ldid -S` និង ad-hoc code sign រួចជាស្រេច។

### Option B: With CMake (macOS Local / iOS Toolchain):
```bash
mkdir build && cd build
cmake .. -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
cmake --build . --config Release
```

### Option C: With Theos (Makefile):
```bash
make
```

### Injecting into IPA:
Use `azule` or `optool`:
```bash
azule -i RulesOfSurvival.ipa -o ROS_Modded.ipa -f RyomenX.dylib
```
