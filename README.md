# ROS Hairiki / RyomenX Project Workspace

## Structure

```
d:\RosHairiki\
├── .github/
│   └── workflows/
│       └── build.yml            # GitHub Actions Mac Cloud CI (macOS-14 Apple Silicon runner)
│
├── ROSHAIRIKI_ORIGINAL/         # Original target binaries and dylib
│   ├── ROS-Hariki-iOS-v78b-lean-unsigned.ipa
│   └── RyomenXCracked.dylib
│
├── RyomenX_Source/              # Reconstructed C++17 / Objective-C++ / ImGui Source Code
│   ├── CMakeLists.txt           # CMake with FetchContent (ImGui v1.90.4 & Metal backend)
│   ├── Makefile                 # Theos build manifest for iOS arm64
│   ├── README.md
│   ├── include/                 # Header declarations (Menu.h, MemoryCheats.hpp, etc.)
│   ├── scripts/                 # Extracted BigWorld Python payload
│   └── src/                     # Core C++, Objective-C++, and Metal implementations
│
├── Analysis/                    # Reverse Engineering Evidence & Artifacts
│   ├── Disassembly/             # Disassembled functions (Memory cheats, ImGui UI, Render loops)
│   ├── Metadata/                # Extracted strings, Mach-O symbols, and Obj-C classes
│   └── Scripts/                 # Extraction and string analysis helper scripts
│
└── .gitignore                   # Excludes huge IPAs (>100MB) & build caches for GitHub
```

---

## 🚀 How to Build on GitHub Mac Cloud (Free macOS-14 Runner)

ដោយសារនៅលើ Windows មិនមាន Apple Clang និង iOS SDK ដោយផ្ទាល់ យើងបានបង្កើត GitHub Actions CI Workflow សម្រាប់ compile នៅលើ GitHub Mac Cloud (`macos-14` M1 runner) ដោយស្វ័យប្រវត្តិ៖

### ជំហានទី ១៖ បង្កើត Git Repository និង Push ទៅកាន់ GitHub
```bash
# នៅក្នុង folder d:\RosHairiki
git init
git add .
git commit -m "feat: RyomenX iOS dylib with Eclipse White ImGui Menu"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPOSITORY.git
git push -u origin main
```
*(ចំណាំ៖ `.gitignore` នឹងរំលងឯកសារ IPA ធំៗ ៨៥០MB ដោយស្វ័យប្រវត្តិ ដើម្បីកុំឱ្យជាប់ limit ១០០MB របស់ GitHub)*

### ជំហានទី ២៖ ទាញយក `RyomenX.dylib`
1. បើក Repository របស់អ្នកនៅលើ GitHub រួចចុចលើផ្ទាំង **Actions**
2. អ្នកនឹងឃើញ Workflow **"Build iOS Dylib (Mac Cloud)"** កំពុងដំណើរការ build
3. នៅពេល build ចប់ (ប្រហែល ១ នាទី) ចុចលើ workflow run នោះ
4. ទាញយក file ពីផ្នែក **Artifacts** ឈ្មោះ `RyomenX-iOS-arm64-dylib` (អ្នកនឹងទទួលបាន `RyomenX.dylib` ដែលបាន sign និង test រួចជាស្រេច)

### ជំហានទី ៣៖ Inject ចូលហ្គេម និងដំឡើងលើ iPhone/iPad
- ប្រើ **Sideloadly**, **Esign**, **Scarlet**, ឬ **Azule** ដើម្បី inject `RyomenX.dylib` ចូលទៅក្នុង `RulesOfSurvival.ipa`
- ដំឡើងតាមរយៈ **TrollStore** ឬ Sign ជាមួយ Certificate ផ្ទាល់ខ្លួន
