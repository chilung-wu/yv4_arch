# OpenBMC Yosemite4 開機流程分析

## 專案概述

此專案深入分析 Facebook OpenBMC Yosemite4 的開機流程，特別針對其獨特的 **static-norootfs** 架構進行詳細研究。Yosemite4 採用了與傳統 Linux 系統不同的開機方式，不使用 `switch_root` 來切換根檔案系統，而是直接在 initramfs 中運行完整的系統。

## 主要特色

### 🔧 技術架構
- **硬體平台**: ASPEED AST2620-A3 BMC SoC
- **記憶體**: 1024MB DDR4 (ECC 啟用，910MB 可用)
- **儲存**: 128MB SPI NOR Flash
- **開機架構**: static-norootfs (無根檔案系統切換)
- **檔案系統**: OverlayFS + UBIFS 混合架構
- **服務管理**: systemd 257.1

### 📊 研究重點
1. **開機流程分析**: 從 ROM Code 到 systemd 的完整流程
2. **檔案系統掛載**: OverlayFS 實作和持久化機制
3. **記憶體管理**: initramfs 在記憶體中的運作方式
4. **架構比較**: 與傳統 switch_root 架構的差異

## 目錄結構

```
yv4_arch/
├── docs/                    # 技術文件
│   ├── hardware-specs.md    # 硬體規格文件
│   ├── boot-analysis.md     # 開機流程分析
│   └── comparison.md        # 架構比較分析
├── diagrams/                # 流程圖和架構圖
│   ├── boot-flow.mmd        # 開機流程圖 (Mermaid)
│   ├── filesystem.mmd       # 檔案系統架構圖
│   └── memory-layout.mmd    # 記憶體配置圖
├── configs/                 # 配置檔案範例
│   ├── machine-config/      # Yocto 機器配置
│   ├── device-tree/         # Device Tree 檔案
│   └── u-boot/             # U-Boot 配置
└── scripts/                # 分析工具和腳本
    ├── parse-logs.py        # 開機日誌分析
    └── mount-analyzer.sh    # 檔案系統分析
```

## 快速開始

### 1. 查看開機流程圖
```bash
# 使用支援 Mermaid 的編輯器開啟
code diagrams/boot-flow.mmd
```

### 2. 閱讀技術分析
```bash
# 開機流程詳細分析
code docs/boot-analysis.md

# 硬體規格文件
code docs/hardware-specs.md
```

### 3. 比較分析
```bash
# 與傳統架構的比較
code docs/comparison.md
```

## 關鍵發現

### ✅ 已確認的技術細節
- Yosemite4 確實使用 `phosphor-static-norootfs.inc` 配置
- 開機過程中**沒有**根檔案系統切換 (`switch_root`)
- 整個 rootfs 在記憶體 (tmpfs) 中運行
- 使用 OverlayFS 實現選擇性目錄持久化

### 🔍 核心技術機制
1. **FIT Image**: U-Boot 載入包含 kernel + DTB + initramfs 的單一映像檔
2. **Memory Layout**: 完整系統載入到 RAM 運行
3. **Overlay Mounts**: 只有 `/var`, `/etc`, `/home` 等目錄可持久化
4. **UBIFS Storage**: Flash 儲存使用 UBIFS 檔案系統

## 技術規格

### 硬體平台
- **SOC**: ASPEED AST2620-A3 (ARM Cortex-A7 雙核心 @ 1.2GHz)
- **記憶體**: 1024MB DDR4-1600 (ECC 啟用，910MB 可用)
- **儲存**: 128MB SPI NOR Flash (mx66l1g45g)
- **網路**: 4x Gigabit Ethernet + NCSI
- **製程**: 28nm 先進製程

### 軟體堆疊 (實際版本)
- **韌體**: U-Boot 2019.04 yosemite4-v2025.26.1.b1896
- **核心**: Linux 6.6.94-fe092ec-dirty-9fb2cfa
- **編譯器**: GCC 14.2.0, GNU Binutils 2.44
- **使用者空間**: OpenBMC yosemite4-v2025.26.1.b1896
- **檔案系統**: static-norootfs + OverlayFS + UBIFS
- **init 系統**: systemd 257.1

## 開發工具

### 必需工具
- **編輯器**: VS Code (支援 Mermaid 外掛)
- **版本控制**: Git
- **文件工具**: Markdown 編輯器

### 推薦擴充套件
- Mermaid Preview (流程圖預覽)
- Markdown All in One (文件編輯)
- GitLens (版本控制)
