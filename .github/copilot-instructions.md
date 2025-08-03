<!-- OpenBMC Yosemite4 Boot Flow Analysis Workspace -->

# OpenBMC Yosemite4 分析工作區

此工作區專門用於分析 Facebook OpenBMC Yosemite4 的開機流程，特別是其 static-norootfs 架構的實作細節。

## 專案目標
- 分析 Yosemite4 的 static-norootfs 開機架構
- 建立詳細的開機流程圖 (Mermaid 格式)
- 記錄硬體規格和軟體配置
- 比較與傳統 switch_root 架構的差異

## 主要研究範圍
1. Yosemite4 硬體架構 (AST2600 BMC)
2. OpenBMC phosphor-static-norootfs 實作
3. 檔案系統掛載順序和 overlay 機制
4. U-Boot, 核心, initramfs 載入流程
5. systemd 服務啟動順序

## 技術堆疊
- **硬體**: ASPEED AST2600 BMC SoC
- **韌體**: U-Boot SPL + U-Boot Proper
- **作業系統**: OpenBMC (Yocto-based Linux)
- **檔案系統**: static-norootfs + OverlayFS + UBIFS
- **服務管理**: systemd

## 文件結構
- `docs/` - 技術文件和分析報告
- `diagrams/` - 流程圖和架構圖
- `configs/` - 配置檔案範例
- `scripts/` - 分析腳本和工具

## 進度追蹤
- [x] 建立工作區結構
- [x] 收集 Yosemite4 硬體規格
- [x] 分析 static-norootfs 實作
- [x] 建立開機流程圖
- [x] 撰寫技術文件
- [x] 建立比較分析
- [x] 創建配置檔案範例
- [x] 完成文件撰寫

## 參考資源
- OpenBMC GitHub Repository
- Facebook OpenBMC Meta-layers
- ASPEED AST2600 技術文件
- Yocto Project 文件
