# SPDX-License-Identifier: GPL-2.0

load("//build/kernel/kleaf:common_kernels.bzl", "define_common_kernels")
load("//common:common_drivers/amlogic_utils.bzl", "define_common_amlogic")
load("//common:common_drivers/modules.bzl", "AMLOGIC_MODULES")
load("//common:common_drivers/project/project.bzl", "EXT_MODULES_ANDROID", "GKI_CONFIG", "KCONFIG_EXT_SRCS")
load("//common:common_drivers/project/dtb.bzl", "AMLOGIC_DTBS")

_AMLOGIC_DTBOS = [
    "android_overlay_dt.dtbo",
]

_AMLOGIC_OUTS = [
] + AMLOGIC_DTBS

_AMLOGIC_MODULES = [
] + AMLOGIC_MODULES

_EXT_MODULES = [
] + EXT_MODULES_ANDROID

def define_amlogic():
    define_common_amlogic(
        name = "amlogic",
        outs = _AMLOGIC_OUTS,
        dtbo_srcs = _AMLOGIC_DTBOS,
        define_abi_targets = True if GKI_CONFIG else False,
        kmi_symbol_list = "//common:android/abi_gki_aarch64_amlogic" if GKI_CONFIG else None,
        kmi_symbol_list_add_only = True if GKI_CONFIG else False,
        build_config = "common_drivers/build.config.amlogic.bazel",
        module_outs = _AMLOGIC_MODULES,
        ext_modules = _EXT_MODULES,
        module_grouping = False,
        kconfig_ext = "common_drivers/Kconfig.ext",
        kconfig_ext_srcs = KCONFIG_EXT_SRCS,
    )
