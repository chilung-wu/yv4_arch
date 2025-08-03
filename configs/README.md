# OpenBMC Yosemite4 配置檔案範例

本目錄包含 Yosemite4 開機流程中使用的關鍵配置檔案範例。

## 目錄結構

```
configs/
├── machine-config/          # Yocto 機器配置
│   ├── yosemite4.conf       # 主機器配置
│   └── static-norootfs.inc  # static-norootfs 配置
├── device-tree/             # Device Tree 配置  
│   └── yosemite4.dts        # 硬體描述檔案
├── u-boot/                  # U-Boot 配置
│   ├── defconfig            # 編譯配置
│   └── env.txt              # 環境變數配置
└── init-scripts/            # 初始化腳本
    ├── 10-early-mounts      # 基礎檔案系統掛載
    ├── 20-udev              # 裝置管理初始化
    ├── 30-ubiattach         # Flash 儲存準備
    └── 50-mount-persistent  # OverlayFS 配置
```

## 使用說明

這些配置檔案展示了 Yosemite4 static-norootfs 架構的核心設定，可作為：

1. **學習參考**: 了解 OpenBMC 配置結構
2. **開發基礎**: 自訂 BMC 韌體的起點
3. **除錯工具**: 分析開機問題的參考
4. **文件資料**: 技術規格的具體實現

## 注意事項

- 這些檔案僅供學習和參考使用
- 實際部署前請根據具體硬體調整配置
- 建議參考最新的 OpenBMC 官方文件
