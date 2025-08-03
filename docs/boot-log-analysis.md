# Yosemite4 開機 Log 分析

本文檔分析真實的 Yosemite4 開機 Log，驗證文件內容的準確性。

## 實際開機 Log 驗證結果

### 1. U-Boot 資訊

**實際數據：**
```
U-Boot SPL 2019.04 yosemite4-v2025.26.1.b1896 (May 23 2025 - 06:46:26 +0000)
U-Boot 2019.04 yosemite4-v2025.26.1.b1896 (May 23 2025 - 06:46:26 +0000)
SOC: AST2620-A3
RST: Power On
DRAM: already initialized, 910 MiB (capacity:1024 MiB, VGA:0 MiB, ECC:on, ECC size:910 MiB)
```

**驗證結果：** ✅ 確認
- U-Boot 版本：2019.04 (與文件一致)
- SOC 型號：AST2620-A3 (實際是 AST2620，非 AST2600)
- 記憶體：910 MiB 可用 (總容量 1024 MiB，ECC 開啟)

### 2. Linux 核心資訊

**實際數據：**
```
Linux version 6.6.94-fe092ec-dirty-9fb2cfa-00552-g9fb2cfa6cebd
CPU: ARMv7 Processor [410fc075] revision 5 (ARMv7), cr=10c5387d
OF: fdt: Machine model: Facebook Yosemite 4 BMC
arch_timer: cp15 timer(s) running at 1200.00MHz (phys).
Calibrating delay loop: 2400.00 BogoMIPS (lpj=12000000)
smp: Brought up 1 node, 2 CPUs
SMP: Total of 2 processors activated (4800.00 BogoMIPS).
```

**驗證結果：** ✅ 確認
- 核心版本：6.6.94 (較新版本)
- CPU 時脈：1200.00MHz (與文件修正後的規格一致)
- CPU 核心數：2 (雙核心確認)
- BogoMIPS：2400.00 x 2 = 4800.00 (符合預期)

### 3. 記憶體配置

**實際數據：**
```
Zone ranges:
  Normal   [mem 0x0000000080000000-0x00000000b8dfffff]
Memory: 842768K/931840K available (10240K kernel code, 869K rwdata, 2600K rodata, 1024K init, 180K bss, 72688K reserved, 16384K cma-reserved, 0K highmem)
```

**驗證結果：** ✅ 基本正確
- 可用記憶體：~842MB
- 總記憶體：~931MB (約 910 MiB 換算正確)

### 4. 檔案系統掛載順序

**實際數據：**
```
[   15.554443] ubi0: attaching mtd5
[   15.611257] ubi0: attached mtd5 (name "rwfs", size 32 MiB)
[   17.677145] UBIFS (ubi0:0): UBIFS: mounted UBI device 0, volume 0, name "rwfs"
[   32.466598] overlayfs: failed to set xattr on upper
[   32.477530] overlayfs: ...falling back to redirect_dir=nofollow.
[   32.490887] overlayfs: ...falling back to uuid=null.
```

**驗證結果：** ✅ 符合 static-norootfs 架構
- UBIFS 掛載於 15-17 秒
- OverlayFS 在 32 秒左右初始化
- 確認使用 /dev/ram 作為 root (kernel command line)

### 5. systemd 啟動

**實際數據：**
```
[   18.158054] systemd[1]: systemd 257.1 running in system mode
Welcome to Facebook OpenBMC yosemite4-v2025.26.1.b1896!
[   18.288499] systemd[1]: Hostname set to <bmc>.
```

**驗證結果：** ✅ 確認
- systemd 版本：257.1 (較新版本)
- 啟動時間：約 18 秒後開始
- 主機名設定：bmc

### 6. 硬體初始化時序

**實際時間軸：**
```
[    0.000000] Booting Linux
[    2.768723] Serial ports initialized
[    4.331342] Freeing initrd memory: 47784K
[   15.554443] UBI attachment starts
[   17.677145] UBIFS mounted
[   18.158054] systemd starts
[   40.179537] Network interfaces up
[   88.592948] Final hardware initialization
```

**驗證結果：** ✅ 時序合理
- initramfs 釋放：4.3 秒
- 檔案系統就緒：17.7 秒
- 系統服務啟動：18.2 秒
- 網路就緒：40.2 秒
- 完整啟動：約 88 秒

## 需要修正的文件內容

### 1. 硬體規格修正

**SOC 型號更正：**
- 文件中：AST2600
- 實際：AST2620-A3

### 2. 版本資訊更新

**軟體版本：**
- U-Boot：2019.04 (正確)
- Linux：6.6.94 (比文件中的版本更新)
- systemd：257.1 (較新版本)
- OpenBMC：yosemite4-v2025.26.1.b1896

### 3. 記憶體配置確認

**實際記憶體配置：**
- 總容量：1024 MiB
- ECC 開啟，可用：910 MiB
- CMA 保留：16 MiB

## 啟動效能分析

### 關鍵時間點
1. **核心啟動完成**：4.3 秒 (initramfs 釋放)
2. **檔案系統就緒**：17.7 秒 (UBIFS 掛載完成)
3. **系統服務啟動**：18.2 秒 (systemd 開始)
4. **基本服務就緒**：40 秒 (網路啟動)
5. **完整系統就緒**：88 秒 (所有硬體初始化)

### 效能評估
- **核心啟動**：4.3 秒 (良好)
- **檔案系統掛載**：13.4 秒 (UBIFS 恢復需要時間，正常)
- **服務啟動**：22 秒 (18.2 到 40 秒，合理)
- **硬體初始化**：48 秒 (40 到 88 秒，I2C 設備較多)

## 驗證總結

✅ **已確認正確**：
- CPU 時脈 1.2GHz
- 雙核心 ARM Cortex-A7
- static-norootfs 架構
- OverlayFS + UBIFS 組合
- systemd 服務管理

⚠️ **需要更新**：
- SOC 型號：AST2620-A3 (非 AST2600)
- 軟體版本資訊
- 實際開機時序

❌ **文件錯誤**：
- 無重大錯誤，主要是版本資訊需要更新
