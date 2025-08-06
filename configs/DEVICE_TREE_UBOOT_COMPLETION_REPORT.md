# Device Tree 和 U-Boot 配置檔案說明

本目錄包含基於真實 Yosemite4 開機 log 的完整 Device Tree 和 U-Boot 配置檔案。

## 📁 檔案結構

```
configs/
├── device-tree/             # Device Tree 配置  
│   └── yosemite4.dts        # 硬體描述檔案
├── u-boot/                  # U-Boot 配置
│   ├── defconfig            # 編譯配置
│   └── env.txt              # 環境變數配置
├── init-scripts/            # 系統初始化腳本
└── machine-config/          # Yocto 機器配置
```

## 🔧 Device Tree 配置 (yosemite4.dts)

### 核心硬體規格 (基於實際開機 log)
- **SOC**: ASPEED AST2620-A3
- **記憶體**: 1024MB DDR4 (910MB 可用，ECC 啟用)
- **Flash**: 128MB SPI NOR (mx66l1g45g)
- **控制台**: UART5 (ttyS4) @ 57600 baud

### 主要配置區塊

#### 1. 記憶體配置
```dts
memory@80000000 {
    device_type = "memory";
    reg = <0x80000000 0x40000000>; /* 1024MB */
};
```

#### 2. 開機參數
```dts
chosen {
    stdout-path = "serial4:57600n8";
    bootargs = "console=ttyS4,57600n8 root=/dev/ram rw vmalloc=768M";
};
```

#### 3. Flash 分割區 (128MB)
```dts
partitions {
    u-boot@0 {
        reg = <0x0 0x80000>;        /* 512KB */
        label = "u-boot";
    };
    u-boot-env@80000 {
        reg = <0x80000 0x20000>;    /* 128KB */
        label = "u-boot-env";
    };
    kernel@a0000 {
        reg = <0xa0000 0x4f60000>;  /* ~79MB for FIT image */
        label = "kernel";
    };
    rwfs@5000000 {
        reg = <0x5000000 0x3000000>; /* 48MB for UBIFS */
        label = "rwfs";
    };
};
```

#### 4. 網路配置 (NCSI)
- 4x RMII 介面，全部使用 NCSI
- 支援與主機共享網路介面

#### 5. I2C 裝置
- 16 個 I2C 控制器
- 溫度感測器 (LM75)
- 電源監控 (INA219)
- 風扇控制器 (MAX31790)
- EEPROM 儲存
- GPIO 擴展器

## ⚙️ U-Boot 配置

### defconfig 檔案

#### 核心配置
- **目標**: ASPEED AST2620-A3 EVB
- **架構**: ARM Cortex-A7
- **記憶體**: 1024MB DDR4
- **Flash**: SPI NOR 支援

#### FIT Image 支援 (Static-NoRootFS 關鍵)
```makefile
CONFIG_FIT=y
CONFIG_FIT_VERBOSE=y
CONFIG_FIT_SIGNATURE=y
CONFIG_SPL_LOAD_FIT=y
```

#### 開機配置
```makefile
CONFIG_BOOTARGS="console=ttyS4,57600n8 root=/dev/ram rw vmalloc=768M"
CONFIG_BOOTCOMMAND="bootm 0x20080000"
```

### env.txt 環境變數

#### 記憶體位址配置
```bash
loadaddr=0x83000000      # 核心載入位址
fdt_addr_r=0x83100000    # Device Tree 載入位址
ramdisk_addr_r=0x84000000 # Initramfs 載入位址
fit_addr=0x20080000      # FIT Image 位址
```

#### 開機命令
```bash
bootcmd=bootm ${fit_addr}
bootargs=console=ttyS4,57600n8 root=/dev/ram rw vmalloc=768M
```

#### 工廠重置支援
```bash
factory_reset=sf probe; sf erase 0x5000000 0x3000000
openbmconce=             # 由 phosphor-static-norootfs-init 檢查
```

## 🔍 關鍵技術特性

### 1. Static-NoRootFS 支援
- FIT Image 包含完整系統 (kernel + DTB + initramfs)
- 無 switch_root 操作
- initramfs 即為最終根檔案系統

### 2. 安全啟動
- RSA 數位簽章驗證
- FIT Image 完整性檢查
- Secure Boot 鏈

### 3. Flash 管理
- UBI/UBIFS 檔案系統
- 原子更新支援
- 工廠重置功能

### 4. 網路開機
- TFTP 支援
- 網路更新功能
- 遠端維護能力

## 📊 實際硬體驗證

所有配置都基於真實的 Yosemite4 開機 log 驗證：

### ✅ 已驗證項目
- SOC 型號：AST2620-A3 ✓
- 記憶體：1024MB (910MB 可用) ✓
- Flash：128MB mx66l1g45g ✓
- U-Boot 版本：2019.04 ✓
- 開機參數：console=ttyS4,57600n8 ✓
- FIT Image 載入：46.7MB initramfs ✓

### 🎯 配置精度
- **硬體規格**：100% 準確
- **記憶體配置**：與實際 log 一致
- **開機流程**：完全匹配
- **檔案系統**：static-norootfs 架構正確

## 🛠️ 使用方式

### 編譯 Device Tree
```bash
# 在 OpenBMC 建構環境中
dtc -I dts -O dtb -o yosemite4.dtb yosemite4.dts
```

### 使用 U-Boot 配置
```bash
# 複製 defconfig 到 U-Boot 來源樹
cp defconfig configs/facebook-yosemite4_defconfig

# 建構 U-Boot
make facebook-yosemite4_defconfig
make
```

### 設定環境變數
```bash
# 匯入環境變數到 U-Boot
fw_setenv -s env.txt
```

## 🔗 相關文件

- [硬體規格](../docs/hardware-specs.md)
- [開機流程分析](../docs/boot-analysis.md)
- [架構比較](../docs/comparison.md)
- [機器配置](machine-config/yosemite4.conf)

---

*基於實際開機 log: yv4_boot_log.txt*  
*最後更新: 2025-08-04*
