FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://0001-device-tree.patch"

# prepend to patch file
# arch/arm/boot/dts/