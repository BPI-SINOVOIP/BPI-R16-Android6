$(call inherit-product, build/target/product/full_base.mk)
$(call inherit-product, device/softwinner/astar-common/astar-common.mk)
$(call inherit-product-if-exists, device/softwinner/astar-evb30/modules/modules.mk)

DEVICE_PACKAGE_OVERLAYS := device/softwinner/astar-evb30/overlay \
                           $(DEVICE_PACKAGE_OVERLAYS)

PRODUCT_PACKAGES += Launcher3
PRODUCT_PACKAGES += \
    ESFileExplorer \
    VideoPlayer \
    Bluetooth 

#   PartnerChromeCustomizationsProvider

PRODUCT_COPY_FILES += \
    device/softwinner/astar-evb30/kernel:kernel \
    device/softwinner/astar-evb30/fstab.sun8i:root/fstab.sun8i \
    device/softwinner/astar-evb30/init.sun8i.rc:root/init.sun8i.rc \
    device/softwinner/astar-evb30/init.recovery.sun8i.rc:root/init.recovery.sun8i.rc \
    device/softwinner/astar-evb30/ueventd.sun8i.rc:root/ueventd.sun8i.rc \
    device/softwinner/astar-evb30/recovery.fstab:recovery.fstab \
    device/softwinner/astar-evb30/modules/modules/nand.ko:root/nand.ko \
    device/softwinner/astar-evb30/modules/modules/disp.ko:root/disp.ko \
    device/softwinner/astar-evb30/modules/modules/lcd.ko:root/lcd.ko \
    device/softwinner/astar-evb30/modules/modules/gslX680new.ko:root/gslX680new.ko \
    device/softwinner/astar-evb30/modules/modules/sunxi-keyboard.ko:root/sunxi-keyboard.ko \
    device/softwinner/astar-evb30/modules/modules/sw-device.ko:root/sw-device.ko \
    device/softwinner/astar-evb30/modules/modules/sunxi-keyboard.ko:obj/sunxi-keyboard.ko \
    device/softwinner/astar-evb30/modules/modules/sw-device.ko:obj/sw-device.ko \
    device/softwinner/astar-evb30/modules/modules/gslX680new.ko:obj/gslX680new.ko

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.ethernet.xml:system/etc/permissions/android.hardware.ethernet.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml

# Low mem(memory <= 512M) device should not copy android.software.managed_users.xml
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.software.managed_users.xml:system/etc/permissions/android.software.managed_users.xml

PRODUCT_COPY_FILES += \
    device/softwinner/astar-evb30/configs/camera.cfg:system/etc/camera.cfg \
    device/softwinner/astar-evb30/configs/gsensor.cfg:system/usr/gsensor.cfg \
    device/softwinner/astar-evb30/configs/media_profiles.xml:system/etc/media_profiles.xml \
    device/softwinner/astar-evb30/configs/sunxi-keyboard.kl:system/usr/keylayout/sunxi-keyboard.kl \
    device/softwinner/astar-evb30/configs/tp.idc:system/usr/idc/tp.idc

#PRODUCT_COPY_FILES += \
#   device/softwinner/astar-evb30/bluetooth/bt_vendor.conf:system/etc/bluetooth/bt_vendor.conf
	
# bootanimation
PRODUCT_COPY_FILES += \
    device/softwinner/astar-evb30/media/bootanimation.zip:system/media/bootanimation.zip

# camera config for camera detector
PRODUCT_COPY_FILES += \
    device/softwinner/astar-evb30/hawkview/sensor_list_cfg.ini:system/etc/hawkview/sensor_list_cfg.ini

# Radio Packages and Configuration Flie
$(call inherit-product, device/softwinner/common/rild/radio_common.mk)
#$(call inherit-product, device/softwinner/common/ril_modem/huawei/mu509/huawei_mu509.mk)
#$(call inherit-product, device/softwinner/common/ril_modem/Oviphone/em55/oviphone_em55.mk)

PRODUCT_PROPERTY_OVERRIDES += \
# limit dex2oat threads to improve thermals
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.boot-dex2oat-threads=4 \
    dalvik.vm.dex2oat-threads=3 \
    dalvik.vm.image-dex2oat-threads=4


# Realtek wifi efuse map
#PRODUCT_COPY_FILES += \
#    device/softwinner/astar-d7/wifi_efuse_8723bs-vq0.map:system/etc/wifi/wifi_efuse_8723bs-vq0.map




PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.dex2oat-flags=--no-watch-dog \
    dalvik.vm.jit.codecachesize=0 \
    ro.am.reschedule_service=true

PRODUCT_PROPERTY_OVERRIDES += \
    ro.frp.pst=/dev/block/by-name/frp

PRODUCT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp \
    ro.adb.secure=1 \
    ro.sys.mutedrm=true \
    rw.logger=0

# A33 Media
PRODUCT_PROPERTY_OVERRIDES += \
   ro.config.media=1 

PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.heapsize=384m \
    dalvik.vm.heapstartsize=8m \
    dalvik.vm.heapgrowthlimit=80m \
    dalvik.vm.heaptargetutilization=0.75 \
    dalvik.vm.heapminfree=512k \
    dalvik.vm.heapmaxfree=8m \
    ro.zygote.disable_gl_preload=false

#PRODUCT_PROPERTY_OVERRIDES += \
#   ro.config.low_ram=true

PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=160

PRODUCT_PROPERTY_OVERRIDES += \
    persist.sys.timezone=Asia/Shanghai \
    persist.sys.country=CN \
    persist.sys.language=zh

# stoarge
PRODUCT_PROPERTY_OVERRIDES += \
    persist.fw.force_adoptable=true
PRODUCT_CHARACTERISTICS := tablet

PRODUCT_AAPT_CONFIG := mdpi
PRODUCT_AAPT_PREF_CONFIG := mdpi

$(call inherit-product-if-exists, vendor/google/products/gms_base.mk)

#for 8723bs-vq0,should setmacaddr
PRODUCT_PACKAGES += setmacaddr

#for 8723bs-vq0,should setbtmacaddr
PRODUCT_PACKAGES += setbtmacaddr
PRODUCT_BRAND := Allwinner
PRODUCT_NAME := astar_evb30
PRODUCT_DEVICE := astar-evb30
PRODUCT_MODEL := QUAD-CORE R16 evb30
PRODUCT_MANUFACTURER := Allwinner
