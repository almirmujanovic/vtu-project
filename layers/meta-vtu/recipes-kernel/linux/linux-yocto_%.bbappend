# Append CAN support to the kernel configuration

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://can-support.cfg"