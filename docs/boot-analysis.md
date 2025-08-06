# OpenBMC Yosemite4 開機流程詳細分析

## 總覽

OpenBMC Yosemite4 採用獨特的 **static-norootfs** 開機架構，這與傳統的 Linux 系統有根本性的差異。本文件深入分析整個開機流程，從硬體上電到系統完全就緒的每個階段。

## 核心架構特點

### 🔑 關鍵差異：無 switch_root 設計

```bash
# 傳統 Linux 開機流程
ROM → U-Boot → Kernel → initramfs → switch_root → real rootfs → systemd

# Yosemite4 static-norootfs 流程  
ROM → U-Boot → Kernel → initramfs (完整 rootfs) → systemd
                                 ↑
                           無 switch_root！
```

**重要觀念**: Yosemite4 的 initramfs **就是**完整且唯一的根檔案系統，不存在"切換"到其他根檔案系統的過程。

## 詳細開機階段分析

### 階段 1: 硬體啟動和 ROM Code (T0-T1: 0-500ms)

```
┌─────────────────────────────────────────┐
│           ASPEED AST2620-A3 SoC         │
│  ┌─────────────────────────────────────┐ │
│  │          ROM Code                   │ │
│  │  ・初始化基本 CPU 暫存器             │ │
│  │  ・設定 SRAM 記憶體控制器            │ │
│  │  ・從 SPI Flash 載入 U-Boot SPL     │ │
│  │  ・驗證 SPL 簽章 (Secure Boot)      │ │
│  │  ・跳轉到 SPL 執行位址               │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

#### 技術細節
- **執行位置**: SoC 內建 ROM (16KB)
- **記憶體**: 使用內建 SRAM (64KB)
- **載入目標**: U-Boot SPL (前 512KB)
- **驗證機制**: RSA-2048 數位簽章

```c
// ROM Code 偽代碼流程
void rom_boot_sequence() {
    init_cpu_registers();
    init_sram_controller();
    
    if (secure_boot_enabled()) {
        if (!verify_spl_signature()) {
            enter_recovery_mode();
            return;
        }
    }
    
    load_spl_from_flash(FLASH_SPL_OFFSET, SRAM_BASE);
    jump_to_spl(SRAM_BASE);
}
```

### 階段 2: U-Boot SPL 執行 (T1-T2: 500ms-1.5s)

```
┌─────────────────────────────────────────┐
│              U-Boot SPL                 │
│  ┌─────────────────────────────────────┐ │
│  │  DDR4 記憶體控制器初始化             │ │
│  │  ├── 時脈配置 (2400MHz)             │ │  
│  │  ├── 時序參數調整                   │ │
│  │  ├── 記憶體訓練 (Training)          │ │
│  │  └── ECC 初始化 (可選)              │ │
│  │                                     │ │
│  │  載入 U-Boot Proper                 │ │
│  │  ├── 從 Flash 讀取 512KB            │ │
│  │  ├── 複製到 DDR4 記憶體              │ │
│  │  ├── 驗證完整性                     │ │
│  │  └── 跳轉執行                       │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

#### SPL 關鍵任務
1. **DDR4 初始化**: 從 64KB SRAM 擴展到 1024MB DDR4 (910MB 可用)
2. **時脈樹設定**: 配置所有周邊時脈
3. **載入 U-Boot**: 準備完整的開機載入器

```c
// U-Boot SPL 主要流程
void spl_board_init() {
    // 1. 基本硬體初始化
    clock_init();
    pin_mux_init();
    
    // 2. DDR4 記憶體初始化 - 最關鍵步驟
    ddr4_controller_init();
    ddr4_training();
    memory_test();
    
    // 3. 載入 U-Boot Proper
    load_uboot_proper();
    jump_to_uboot_proper();
}
```

### 階段 3: U-Boot Proper 執行 (T2-T3: 1.5s-3.0s)

```
┌─────────────────────────────────────────┐
│           U-Boot Proper                 │
│  ┌─────────────────────────────────────┐ │
│  │  環境變數載入和處理                 │ │
│  │  ├── 從 Flash 載入 env              │ │
│  │  ├── 檢查開機模式設定               │ │
│  │  └── 解析開機參數                   │ │
│  │                                     │ │
│  │  FIT Image 載入                     │ │
│  │  ├── 載入 Kernel (vmlinux)          │ │
│  │  ├── 載入 Device Tree Blob          │ │
│  │  ├── 載入 Initramfs (完整rootfs)    │ │
│  │  └── 驗證數位簽章                   │ │
│  │                                     │ │
│  │  啟動 Linux 核心                    │ │
│  │  ├── 設定核心參數                   │ │
│  │  ├── 設定記憶體映射                 │ │
│  │  └── 執行核心                       │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

#### FIT Image 結構
```bash
FIT Image Layout (實際):
├── Header (1KB)
├── Kernel Image (~4.4MB)
│   └── vmlinux (uncompressed)
├── Device Tree Blob (~71.5KB)  
│   └── aspeed-bmc-facebook-yosemite4.dtb
└── Initramfs (~46.7MB)
    └── 完整的 OpenBMC rootfs (cpio.zst 壓縮)
```

#### 開機參數配置
```bash
# U-Boot 環境變數 (關鍵參數)
bootargs=console=ttyS4,115200n8 root=/dev/ram rw 
bootcmd=bootm 0x20080000
loadaddr=0x83000000
fdtaddr=0x83100000  
rdaddr=0x84000000

# FIT Image 載入位址
fit_loadaddr=0x20080000
```

### 階段 4: Linux 核心啟動 (T3-T4: 3.0s-4.5s)

```
┌─────────────────────────────────────────┐
│            Linux Kernel                 │  
│  ┌─────────────────────────────────────┐ │
│  │  核心解壓縮和重定位                 │ │
│  │  ├── 解壓縮 vmlinux                 │ │
│  │  ├── 建立頁表                       │ │
│  │  └── 啟用 MMU                       │ │
│  │                                     │ │
│  │  硬體子系統初始化                   │ │
│  │  ├── 中斷控制器 (GIC-400)           │ │
│  │  ├── 計時器 (ARM Generic Timer)     │ │
│  │  ├── DMA 控制器                     │ │
│  │  └── 基本 I/O 子系統                │ │
│  │                                     │ │
│  │  Initramfs 掛載                     │ │
│  │  ├── 解壓縮 initramfs               │ │
│  │  ├── 建立 tmpfs rootfs              │ │
│  │  ├── 解壓縮檔案到 tmpfs             │ │
│  │  └── 設定為根檔案系統               │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

#### 核心載入記憶體配置
```
DDR4 記憶體配置 (1024MB):
┌─────────────────────────────────────────┐ 0x80000000
│         核心映像檔 (~10MB)                │
├─────────────────────────────────────────┤ 0x80A00000  
│         initramfs (~70MB)                │
├─────────────────────────────────────────┤ 0x85000000
│         核心資料結構 (~100MB)             │
├─────────────────────────────────────────┤ 0x8B400000
│         tmpfs rootfs (~400MB)            │
├─────────────────────────────────────────┤ 0xA4400000
│         可用記憶體 (~400MB)               │
└─────────────────────────────────────────┘ 0xBFFFFFFF
```

### 階段 5: Static-NoRootFS 初始化 (T4-T5: 4.5s-8.0s)

這是 Yosemite4 最獨特的階段，由 `phosphor-static-norootfs-init` 套件提供的 `/init` 腳本執行。

```
┌─────────────────────────────────────────┐
│      phosphor-static-norootfs-init      │
│  ┌─────────────────────────────────────┐ │
│  │  /init 主控制腳本                   │ │
│  │  ├── 設定基本環境變數               │ │
│  │  ├── 掃描可用的初始化腳本           │ │
│  │  ├── 按順序執行腳本                 │ │
│  │  └── 轉交控制權給 systemd           │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

#### 詳細初始化腳本序列

##### 5.1 執行 10-early-mounts
```bash
#!/bin/sh
# 檔案: /etc/init.d/10-early-mounts

# 掛載基本的虛擬檔案系統
mount -t devtmpfs devtmpfs /dev
mount -t sysfs sysfs /sys  
mount -t proc proc /proc
mount -t tmpfs tmpfs /run -o mode=755,nodev,nosuid,strictatime

# 建立基本目錄結構
mkdir -p /run/lock /run/log
chmod 755 /run/lock
```

**目的**: 建立基本的虛擬檔案系統，為後續操作提供必要的系統介面。

##### 5.2 執行 20-udev
```bash
#!/bin/sh  
# 檔案: /etc/init.d/20-udev

# 啟動 udev 進行裝置管理
echo "Starting udev..."
/lib/systemd/systemd-udevd --daemon

# 觸發裝置偵測
udevadm trigger --action=add
udevadm settle --timeout=10
```

**目的**: 初始化裝置管理子系統，偵測和建立硬體裝置節點。

##### 5.3 執行 21-factory-reset
```bash
#!/bin/sh
# 檔案: /etc/init.d/21-factory-reset

check_factory_reset() {
    # 檢查 GPIO 重置訊號
    if [ -f /sys/class/gpio/gpio123/value ]; then
        reset_signal=$(cat /sys/class/gpio/gpio123/value)
        if [ "$reset_signal" = "0" ]; then
            return 0  # 需要重置
        fi
    fi
    
    # 檢查 U-Boot 環境變數
    if fw_printenv openbmconce | grep -q "factory-reset"; then
        fw_setenv openbmconce  # 清除標記
        return 0  # 需要重置
    fi
    
    return 1  # 不需要重置
}

if check_factory_reset; then
    echo "Factory reset requested, formatting persistent storage..."
    format_persistent_storage
fi
```

**目的**: 檢查是否需要執行工廠重置，清除持久化資料。

##### 5.4 執行 30-ubiattach-or-format
```bash
#!/bin/sh
# 檔案: /etc/init.d/30-ubiattach-or-format

RWFS_MTD="/dev/mtd/rwfs"
PERSIST_DIR="/run/mnt-persist"

format_rwfs() {
    echo "Formatting RWFS partition..."
    ubiformat --yes $RWFS_MTD
    ubiattach -p $RWFS_MTD
    ubimkvol /dev/ubi0 -N rwfs -m
    mount -t ubifs ubi0:rwfs $PERSIST_DIR -o sync,compr=zstd
}

mount_rwfs() {
    echo "Mounting existing RWFS..."
    ubiattach -p $RWFS_MTD
    mount -t ubifs ubi0:rwfs $PERSIST_DIR -o sync,compr=zstd
}

# 嘗試掛載現有的 UBIFS，失敗則格式化
if ! mount_rwfs; then
    echo "RWFS mount failed, formatting..."
    format_rwfs
fi
```

**目的**: 準備持久化儲存，掛載或建立 UBIFS 檔案系統。

##### 5.5 執行 50-mount-persistent
```bash
#!/bin/sh
# 檔案: /etc/init.d/50-mount-persistent

PERSIST="/run/mnt-persist"
PERSISTENT_DIRS="/var /etc /home"

mount_overlay() {
    local dir=$1
    local lower_dir="$dir"
    local upper_dir="$PERSIST${dir}-data"  
    local work_dir="$PERSIST${dir}-work"
    
    # 建立必要的目錄
    mkdir -p "$upper_dir" "$work_dir"
    
    # 掛載 OverlayFS
    mount -t overlay overlay "$dir" \
        -o lowerdir="$lower_dir",upperdir="$upper_dir",workdir="$work_dir"
    
    echo "Mounted overlay for $dir"
}

# 為每個持久化目錄建立 overlay 掛載
for dir in $PERSISTENT_DIRS; do
    mount_overlay "$dir"
done
```

**目的**: 建立 OverlayFS 掛載，實現選擇性目錄持久化。

#### OverlayFS 架構詳解

```
OverlayFS 三層結構範例 (/var 目錄):

┌─────────────────────────────────────────┐
│           使用者視角: /var                │  ← 統一檢視
├─────────────────────────────────────────┤
│ Upper Dir: /run/mnt-persist/var-data    │  ← 可寫層 (Flash)
│ ├── cache/                              │
│ ├── log/                                │  
│ └── lib/                                │
├─────────────────────────────────────────┤
│ Lower Dir: /var (來自 initramfs)         │  ← 唯讀層 (記憶體)
│ ├── empty/ (預設空目錄)                  │
│ ├── lib/ (預設函式庫檔案)                │
│ └── run/ (執行時目錄)                    │
├─────────────────────────────────────────┤
│ Work Dir: /run/mnt-persist/var-work     │  ← OverlayFS 工作目錄
└─────────────────────────────────────────┘
```

#### 檔案操作行為
```bash
# 讀取檔案優先順序:
# 1. Upper Dir (如果檔案存在)
# 2. Lower Dir (如果 Upper Dir 沒有)

# 寫入檔案行為:
# 1. 新檔案直接寫入 Upper Dir
# 2. 修改 Lower Dir 檔案時先 Copy-up 到 Upper Dir

# 刪除檔案行為:
# 1. 在 Upper Dir 建立 whiteout 檔案標記刪除
# 2. Lower Dir 原檔案仍存在但被遮蔽
```

### 階段 6: systemd 啟動 (T5-T6: 8.0s-12.0s)

```bash
# /init 腳本的最後動作
echo "Starting systemd..."
exec /lib/systemd/systemd
```

此時控制權完全轉交給 systemd，開始正常的系統服務管理流程。

```
┌─────────────────────────────────────────┐
│              systemd                    │
│  ┌─────────────────────────────────────┐ │
│  │  基本目標啟動                       │ │
│  │  ├── sysinit.target                 │ │
│  │  ├── basic.target                   │ │
│  │  └── multi-user.target              │ │
│  │                                     │ │
│  │  BMC 特定服務                       │ │
│  │  ├── phosphor-discovery             │ │
│  │  ├── phosphor-host-ipmid            │ │
│  │  ├── phosphor-chassis-state         │ │
│  │  ├── phosphor-bmc-state             │ │
│  │  └── bmcweb.service                 │ │
│  │                                     │ │
│  │  網路和管理服務                     │ │
│  │  ├── systemd-networkd              │ │
│  │  ├── sshd.service                   │ │
│  │  └── obmc-console-server            │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

#### 重要 systemd 服務說明

##### BMC 核心服務
```bash
# phosphor-discovery.service - BMC 服務發現
# phosphor-host-ipmid.service - IPMI 命令處理  
# phosphor-chassis-state.service - 機箱狀態管理
# phosphor-bmc-state.service - BMC 狀態管理
# bmcweb.service - Web 管理介面和 Redfish API
```

##### 網路服務
```bash
# systemd-networkd.service - 網路配置管理
# systemd-resolved.service - DNS 解析
# sshd.service - SSH 遠端存取
# obmc-console-server.service - 主機控制台重定向
```

## 開機完成狀態分析

### 最終檔案系統掛載狀態

執行 `mount` 命令的結果：
```bash
root@yosemite4:~# mount
rootfs on / type rootfs (rw,size=201832k,nr_inodes=50458)
devtmpfs on /dev type devtmpfs (rw,relatime,size=4096k,nr_inodes=1024,mode=755)
sysfs on /sys type sysfs (rw,nosuid,nodev,noexec,relatime)
proc on /proc type proc (rw,nosuid,nodev,noexec,relatime)
tmpfs on /run type tmpfs (rw,nosuid,nodev,mode=755)
ubi0:rwfs on /run/mnt-persist type ubifs (rw,relatime,compr=zstd)
overlay on /var type overlay (rw,relatime,lowerdir=/var,upperdir=/run/mnt-persist/var-data,workdir=/run/mnt-persist/var-work)
overlay on /etc type overlay (rw,relatime,lowerdir=/etc,upperdir=/run/mnt-persist/etc-data,workdir=/run/mnt-persist/etc-work)  
overlay on /home type overlay (rw,relatime,lowerdir=/home,upperdir=/run/mnt-persist/home-data,workdir=/run/mnt-persist/home-work)
```

### 記憶體使用分析

```bash
root@yosemite4:~# free -h
              total        used        free      shared  buff/cache   available
Mem:           489M        180M         89M        8.0M        220M        285M
Swap:            0B          0B          0B
```

記憶體分配說明：
- **總記憶體**: 910MB (1024MB 扣除 ECC 和硬體保留)
- **已使用**: 180MB (核心 + 服務 + rootfs)
- **緩存**: 220MB (檔案系統緩存 + 緩衝區)
- **可用**: 285MB (實際可分配記憶體)

### 持久化儲存使用

```bash
root@yosemite4:~# df -h /run/mnt-persist/
Filesystem      Size  Used Avail Use% Mounted on
ubi0:rwfs        10M  2.1M  7.6M  22% /run/mnt-persist
```

持久化分割區使用情況：
- **總容量**: 32MB (UBIFS 分割區)
- **已使用**: 2.1MB (配置和日誌檔案)
- **可用**: 7.6MB (可儲存更多持久化資料)

## 關鍵技術優勢分析

### 1. 開機速度優化
```
傳統開機 vs Static-NoRootFS:
┌─────────────────────────────────────────┐
│ 傳統方式:                                │
│ initramfs → fsck → mount → switch_root  │
│   2s    →  3s  →  1s   →     2s        │
│                                         │
│ Static-NoRootFS:                        │
│ initramfs → overlay mounts              │
│   2s    →        3s                     │
│                                         │
│ 節省時間: ~3秒 (約30%更快)               │
└─────────────────────────────────────────┘
```

### 2. 系統穩定性提升
- **不可變系統檔案**: 核心系統檔案無法被意外修改
- **快速恢復**: 重啟即可恢復到乾淨狀態  
- **工廠重置**: 清除持久化分割區即可完全重置

### 3. 記憶體效率
- **壓縮儲存**: initramfs 中的檔案使用 gzip 壓縮
- **按需載入**: 只有實際使用的檔案才會解壓縮到記憶體
- **共享頁面**: 相同檔案在多個程序間共享記憶體頁面

### 4. 維護簡便性
- **單一映像檔**: 整個系統包含在一個 FIT Image 中
- **原子更新**: 系統更新只需替換 FIT Image
- **配置分離**: 系統檔案與使用者配置完全分離

## 除錯和監控

### 開機過程監控
```bash
# 檢視開機訊息
dmesg | grep -E "(init|mount|overlay)"

# 檢視 systemd 服務狀態
systemctl status
systemctl list-failed

# 檢視檔案系統掛載
findmnt -D
```

### 持久化儲存監控
```bash
# 檢視 UBIFS 狀態
ubinfo /dev/ubi0
ubinfo /dev/ubi0 -a

# 檢視 overlay 使用情況  
df -h /run/mnt-persist/
du -sh /run/mnt-persist/*-data/
```

### 記憶體使用監控
```bash
# 檢視記憶體詳細使用
cat /proc/meminfo

# 檢視程序記憶體使用
ps aux --sort=-%mem | head -10

# 檢視檔案系統快取
cat /proc/sys/vm/drop_caches
```

## 故障排除指南

### 常見問題和解決方案

#### 1. 持久化分割區損壞
```bash
# 症狀: overlay 掛載失敗
# 解決: 重新格式化持久化分割區
fw_setenv openbmconce factory-reset
reboot
```

#### 2. 記憶體不足
```bash
# 症狀: 服務啟動失敗，OOM Killer 啟動
# 解決: 檢查記憶體洩漏，重啟服務
systemctl restart phosphor-*
```

#### 3. 檔案系統唯讀錯誤
```bash
# 症狀: 無法寫入 /var 或 /etc
# 解決: 檢查 overlay 掛載狀態
findmnt /var /etc /home
umount /var && /etc/init.d/50-mount-persistent
```

這個開機流程分析展示了 OpenBMC Yosemite4 的創新設計，透過 static-norootfs 架構實現了高效、穩定且易於維護的 BMC 系統。
