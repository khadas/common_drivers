"""Functions that are useful in the common kernel package (usually `//common`)."""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load(
    "//build/kernel/kleaf:kernel.bzl",
    "kernel_abi",
    "kernel_abi_dist",
    "kernel_build",
    "kernel_build_config",
    "kernel_compile_commands",
    "kernel_filegroup",
    "kernel_images",
    "kernel_kythe",
    "kernel_modules_install",
    "kernel_unstripped_modules_archive",
    "merged_kernel_uapi_headers",
)
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load("//build/kernel/kleaf/impl:gki_artifacts.bzl", "gki_artifacts", "gki_artifacts_prebuilts")
load("//build/kernel/kleaf:print_debug.bzl", "print_debug")
load("@kernel_toolchain_info//:dict.bzl", "BRANCH", "common_kernel_package")

# Always collect_unstripped_modules for common kernels.
_COLLECT_UNSTRIPPED_MODULES = True

def define_common_amlogic(
        name,
        outs,
        dtbo_srcs,
        build_config = None,
        module_outs = None,
        make_goals = None,
        define_abi_targets = None,
        kmi_symbol_list = None,
        kmi_symbol_list_add_only = None,
        module_grouping = None,
        unstripped_modules_archive = None,
        dist_dir = None,
        ext_modules = None,
	kconfig_ext = None,
	kconfig_ext_srcs = None):
    """Define target for amlogic.

    Note: This is a mixed build.

    Requires [`define_common_kernels`](#define_common_kernels) to be called in the same package.

    Args:
        name: name of target. Usually `"amlogic"`.
        build_config: See [kernel_build.build_config](#kernel_build-build_config). If `None`,
          default to `"common_drivers/build.config.amlogic.bazel"`.
        outs: See [kernel_build.outs](#kernel_build-outs).
        module_outs: See [kernel_build.module_outs](#kernel_build-module_outs). The list of
          in-tree kernel modules.
        make_goals: See [kernel_build.make_goals](#kernel_build-make_goals).  A list of strings
          defining targets for the kernel build.
        define_abi_targets: See [kernel_abi.define_abi_targets](#kernel_abi-define_abi_targets).
        kmi_symbol_list: See [kernel_build.kmi_symbol_list](#kernel_build-kmi_symbol_list).
        kmi_symbol_list_add_only: See [kernel_abi.kmi_symbol_list_add_only](#kernel_abi-kmi_symbol_list_add_only).
        module_grouping: See [kernel_abi.module_grouping](#kernel_abi-module_grouping).
        unstripped_modules_archive: See [kernel_abi.unstripped_modules_archive](#kernel_abi-unstripped_modules_archive).
        dist_dir: Argument to `copy_to_dist_dir`. If `None`, default is `"out/{BRANCH}/dist"`.
    """

    if build_config == None:
        build_config = "common_drivers/build.config.amlogic.bazel"

    if kmi_symbol_list == None:
        kmi_symbol_list = "//common:android/abi_gki_aarch64_amlogic" if define_abi_targets else None

    if kmi_symbol_list_add_only == None:
        kmi_symbol_list_add_only = True if define_abi_targets else None

    if dist_dir == None:
        dist_dir = "out/{branch}/dist".format(branch = BRANCH)

    # Also refer to the list of ext modules for ABI monitoring targets
    _kernel_modules = ext_modules;

    kernel_build(
        name = name,
        outs = outs,
        srcs = [":common_kernel_sources"] + kconfig_ext_srcs,
        # List of in-tree kernel modules.
        module_outs = module_outs,
        build_config = build_config,
        # Enable mixed build.
        base_kernel = ":kernel_aarch64_download_or_build",
        kmi_symbol_list = kmi_symbol_list,
        collect_unstripped_modules = _COLLECT_UNSTRIPPED_MODULES,
        strip_modules = True,
        make_goals = make_goals,
	kconfig_ext = kconfig_ext,
    )

    # enable ABI Monitoring
    # based on the instructions here:
    # https://android.googlesource.com/kernel/build/+/refs/heads/main/kleaf/docs/abi_device.md
    # https://android-review.googlesource.com/c/kernel/build/+/2308912
    kernel_abi(
        name = name + "_abi",
        kernel_build = name,
        define_abi_targets = define_abi_targets,
        kernel_modules = _kernel_modules,
        kmi_symbol_list_add_only = kmi_symbol_list_add_only,
        module_grouping = module_grouping,
        unstripped_modules_archive = unstripped_modules_archive,
    )

    kernel_modules_install(
        name = name + "_modules_install",
        kernel_build = name,
        # List of external modules.
        kernel_modules = _kernel_modules,
    )

    merged_kernel_uapi_headers(
        name = name + "_merged_kernel_uapi_headers",
        kernel_build = name,
        kernel_modules = _kernel_modules,
    )

    kernel_images(
        name = name + "_images",
        build_dtbo = True,
        dtbo_srcs = [":" + name + "/" + e for e in dtbo_srcs],
        build_initramfs = True,
        kernel_build = name,
        kernel_modules_install = name + "_modules_install",
    )

    dist_targets = [
        name,
        name + "_images",
        name + "_modules_install",
        # Mixed build: Additional GKI artifacts.
        ":kernel_aarch64_download_or_build",
        ":kernel_aarch64_additional_artifacts_download_or_build",
        name + "_merged_kernel_uapi_headers",
    ]

    copy_to_dist_dir(
        name = name + "_dist",
        data = dist_targets,
        dist_dir = dist_dir,
        flat = True,
        log = "info",
    )
