# Phosphor Static NoRootFS Configuration
# 這是 Yosemite4 static-norootfs 架構的核心配置檔案

# 啟用 static-norootfs 功能
DISTRO_FEATURES += "obmc-static-norootfs"
DISTROOVERRIDES .= ":df-obmc-static-norootfs"

# 指定持久化目錄 - 這些目錄會使用 OverlayFS
NOROOTFS_PERSISTENT_DIRS = "\
    var \
    etc \
    home \
    ${@ bb.utils.contains('ROOT_HOME', '/home/root', '', d.getVar('ROOT_HOME')[1:], d)} \
"

# initramfs 配置
INITRAMFS_IMAGE = "obmc-phosphor-initramfs"
INITRAMFS_IMAGE_BUNDLE = "1"  # 將 initramfs 打包進核心
INITRAMFS_MAXSIZE = "42000"   # 最大 42MB

# 根檔案系統類型設定為 static
IMAGE_FSTYPES = "static-norootfs"

# init 系統設定
VIRTUAL-RUNTIME_init_manager = "systemd"
INIT_MANAGER = "systemd"

# 特殊的 init 程式 - 關鍵組件！
VIRTUAL-RUNTIME_initscripts = "phosphor-static-norootfs-init"

# 檔案系統配置
IMAGE_ROOTFS_SIZE = "40960"  # 40MB rootfs
IMAGE_ROOTFS_EXTRA_SPACE = "0"

# 移除不需要的套件 (因為是靜態系統)
IMAGE_FEATURES_remove = "package-management"
PACKAGE_EXCLUDE += "kernel-modules"

# 壓縮設定 (節省 Flash 空間)
CONVERSION_CMD_gz = "gzip -f -9 -n -c ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type} > ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type}.gz"

# U-Boot FIT Image 設定
KERNEL_IMAGETYPE = "fitImage"
KERNEL_CLASSES = "kernel-fitimage"

# initramfs 不需要單獨的映像檔案  
INITRAMFS_FSTYPES = ""
