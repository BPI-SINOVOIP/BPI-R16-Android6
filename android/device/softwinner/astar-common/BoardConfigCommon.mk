include device/softwinner/common/BoardConfigCommon.mk
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a7

TARGET_NO_BOOTLOADER := true

TARGET_BOARD_PLATFORM := astar
TARGET_BOOTLOADER_BOARD_NAME := exdroid
TARGET_BOOTLOADER_NAME := exdroid

BOARD_EGL_CFG := device/softwinner/astar-common/egl/egl.cfg
BOARD_KERNEL_BASE := 0x40000000


#SurfaceFlinger's configs  Echo add from A64
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true

BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_FUSE_SDCARD := true


BOARD_SEPOLICY_DIRS := \
    device/softwinner/astar-common/sepolicy

#Justin Porting for BPI-M2M Root Start
BOARD_SEPOLICY_UNION := \
	bluetooth.te \
	device.te \
	file_contexts \
	genfs_contexts \
	init.te \
	kernel.te \
	mediaserver.te \
	netd.te \
	preinstall.te \
	recovery.te \
        rild.te \
	sayeye.te \
	sensors.te \
	service_contexts \
	shell.te \
	surfaceflinger.te \
	system_app.te \
	system_server.te \
	unconfined.te \
	vold.te \
	wpa.te \
    file.te \
    logger.te \
	platform_app.te
#Justin Porting for BPI-M2M Root End

USE_OPENGL_RENDERER := true


