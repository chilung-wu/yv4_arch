```mermaid
graph TD
    %% Power On and Initial Boot
    A[Power On] --> B{ROM Code<br/>ASPEED AST2620-A3}
    B --> C{U-Boot SPL<br/>SRAM 執行}
    C --> D{U-Boot Proper<br/>DDR4 初始化}
    
    %% U-Boot Loading Phase - 實際時間: 0-3秒
    D -- "載入 FIT Image" --> E[Kernel Image<br/>vmlinux 4.4MB]
    D -- "載入 FIT Image" --> F[Device Tree Blob<br/>AST2620 DTB 71.5KB]
    D -- "載入 FIT Image" --> G[Initramfs<br/>46.7MB 完整 rootfs]
    
    %% Kernel Boot - 實際時間: 3-18秒
    D -- "執行核心" --> E
    E -- "使用 DTB" --> F
    E -- "掛載 initramfs" --> G
    E -- "執行" --> H["/init<br/>phosphor-static-norootfs-init"]
    
    %% Static-NoRootFS Initialization
    H --> I[10-early-mounts<br/>基礎虛擬檔案系統]
    I --> J[20-udev<br/>裝置管理]
    J --> K[21-factory-reset<br/>工廠重置檢查]
    K --> L[30-ubiattach-or-format<br/>Flash 儲存準備]
    L --> M[50-mount-persistent<br/>OverlayFS 設定]
    
    %% File System Mounts Detail
    I --> I1[mount /dev<br/>devtmpfs]
    I --> I2[mount /sys<br/>sysfs]
    I --> I3[mount /proc<br/>procfs]
    I --> I4[mount /run<br/>tmpfs]
    
    L --> L1[ubiformat /dev/mtd/rwfs]
    L --> L2[ubiattach -p /dev/mtd/rwfs]
    L --> L3[mount UBIFS to<br/>/run/mnt-persist]
    
    M --> M1[overlay mount /var]
    M --> M2[overlay mount /etc]
    M --> M3[overlay mount /home]
    
    %% SystemD Launch
    M -- "exec" --> N["/lib/systemd/systemd<br/>PID 1"]
    N --> O[systemd 目標]
    O --> P[BMC 服務啟動]
    P --> Q[網路服務]
    P --> R[管理介面]
    P --> S[硬體監控]
    Q --> T[SSH/Web 介面可用]
    R --> T
    S --> T
    
    %% Memory and Storage Layout
    subgraph "記憶體配置 (1024MB DDR4)"
        direction TB
        MEM1[核心空間<br/>~200MB]
        MEM2[rootfs tmpfs<br/>~300MB]
        MEM3[服務和緩存<br/>~350MB]
        MEM4[可用記憶體<br/>~60MB]
    end
    
    subgraph "Flash 儲存 (128MB SPI NOR)"
        direction TB
        FLASH1[U-Boot<br/>~1MB]
        FLASH2[U-Boot 環境<br/>~128KB]
        FLASH3[FIT Image<br/>~80MB]
        FLASH4[持久化分割區<br/>~32MB]
    end
    
    subgraph "檔案系統架構"
        direction TB
        FS1[rootfs: tmpfs<br/>記憶體檔案系統]
        FS2["/var: overlay<br/>記憶體+Flash"]
        FS3["/etc: overlay<br/>記憶體+Flash"] 
        FS4["/home: overlay<br/>記憶體+Flash"]
        FS5["/run/mnt-persist<br/>UBIFS on Flash"]
    end
    
    %% Connections to subgraphs
    G -.-> MEM2
    L3 -.-> FS5
    M1 -.-> FS2
    M2 -.-> FS3
    M3 -.-> FS4
    
    %% Hardware Components
    subgraph "ASPEED AST2620-A3 SoC"
        direction LR
        HW1[ARM Cortex-A7<br/>雙核心 1.2GHz]
        HW2[DDR4 控制器]
        HW3[SPI 控制器]
        HW4[Ethernet MAC]
        HW5[GPIO/PWM]
    end
    
    %% Boot Media
    subgraph "開機媒體"
        direction TB
        BOOT1[SPI NOR Flash<br/>128MB]
        BOOT2[備用 Flash<br/>可選]
    end
    
    %% Styling
    classDef romCode fill:#ff9999,stroke:#333,stroke-width:2px
    classDef uboot fill:#99ccff,stroke:#333,stroke-width:2px
    classDef kernel fill:#99ff99,stroke:#333,stroke-width:2px
    classDef init fill:#ffcc99,stroke:#333,stroke-width:2px
    classDef systemd fill:#cc99ff,stroke:#333,stroke-width:2px
    classDef service fill:#ffff99,stroke:#333,stroke-width:2px
    classDef ready fill:#99ffcc,stroke:#333,stroke-width:4px
    
    class A romCode
    class B romCode
    class C,D uboot
    class E,F,G kernel
    class H,I,J,K,L,M init
    class N,O systemd
    class P,Q,R,S service
    class T ready
    
    %% Critical Path Highlighting
    A -.->|關鍵路徑| B
    B -.->|關鍵路徑| C
    C -.->|關鍵路徑| D
    D -.->|關鍵路徑| E
    E -.->|關鍵路徑| H
    H -.->|關鍵路徑| N
    N -.->|關鍵路徑| T
```

## 開機時序詳細說明

### 階段 1: 硬體初始化 (0-2 秒)
- **ROM Code**: ASPEED AST2600 內建 ROM 程式碼執行
- **U-Boot SPL**: 在有限的 SRAM 中執行，初始化 DDR4
- **U-Boot Proper**: 完整的 U-Boot，準備載入作業系統

### 階段 2: 系統載入 (2-5 秒)  
- **FIT Image 載入**: 單一映像檔包含核心、DTB、完整 rootfs
- **記憶體配置**: initramfs 解壓縮到 tmpfs，成為唯一的根檔案系統
- **核心啟動**: Linux 核心初始化硬體和記憶體管理

### 階段 3: 檔案系統初始化 (5-8 秒)
- **基礎掛載**: devtmpfs, sysfs, procfs, tmpfs 虛擬檔案系統
- **裝置初始化**: udev 掃描和建立裝置節點
- **持久化準備**: UBIFS 格式化或掛載 Flash 分割區

### 階段 4: OverlayFS 配置 (8-10 秒)
- **選擇性持久化**: 只有 /var, /etc, /home 使用 overlay 掛載
- **混合儲存**: 上層寫入 Flash，下層讀取記憶體
- **透明操作**: 應用程式無法察覺 overlay 機制

### 階段 5: 系統服務啟動 (10-15 秒)
- **systemd 接管**: PID 1 轉換為 systemd
- **並行啟動**: BMC 特定服務和網路服務
- **就緒狀態**: SSH 和 Web 管理介面可用

## 關鍵技術特點

### ✅ 無 switch_root 設計
- **傳統方式**: initramfs → switch_root → 真實 rootfs
- **Yosemite4**: initramfs 即為完整且唯一的 rootfs
- **優勢**: 簡化開機流程，提高穩定性

### 🔄 混合儲存架構
- **系統檔案**: 完全在記憶體 (tmpfs)，快速且唯讀
- **資料檔案**: 透過 OverlayFS 持久化到 Flash
- **工廠重置**: 清除 Flash 分割區即可恢復初始狀態

### 💾 記憶體最佳化
- **壓縮儲存**: initramfs 使用 gzip/lzma 壓縮
- **按需載入**: 程式和函式庫動態載入到記憶體
- **共享頁面**: 相同檔案在多程序間共享記憶體
```
