"""Functions that are useful in the common kernel package (usually `//common`)."""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")
load(
    "//build/kernel/kleaf:kernel.bzl",
    "kernel_abi",
    "kernel_abi_dist",
    "kernel_build",
    "kernel_compile_commands",
    "kernel_filegroup",
    "kernel_images",
    "kernel_kythe",
    "kernel_modules_install",
    "kernel_unstripped_modules_archive",
    "merged_kernel_uapi_headers",
)
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load("//build/kernel/kleaf/artifact_tests:kernel_test.bzl", "initramfs_modules_options_test")
load("//build/kernel/kleaf/artifact_tests:device_modules_test.bzl", "device_modules_test")
load("//build/kernel/kleaf/impl:gki_artifacts.bzl", "gki_artifacts")
load("//build/kernel/kleaf/impl:out_headers_allowlist_archive.bzl", "out_headers_allowlist_archive")
load(
    "//build/kernel/kleaf/impl:constants.bzl",
    "MODULE_OUTS_FILE_OUTPUT_GROUP",
    "MODULE_OUTS_FILE_SUFFIX",
    "TOOLCHAIN_VERSION_FILENAME",
)
load(
    "//build/kernel/kleaf:constants.bzl",
    "CI_TARGET_MAPPING",
    "DEFAULT_GKI_OUTS",
    "GKI_DOWNLOAD_CONFIGS",
    "X86_64_OUTS",
)
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
        define_abi_targets = None,
        kmi_symbol_list = None,
        kmi_symbol_list_add_only = None,
        module_grouping = None,
        unstripped_modules_archive = None,
        gki_modules_list = None,
        dist_dir = None,
        ext_modules = None):
    """Define target for db845c.

    Note: This is a mixed build.

    Requires [`define_common_kernels`](#define_common_kernels) to be called in the same package.

    Args:
        name: name of target. Usually `"db845c"`.
        build_config: See [kernel_build.build_config](#kernel_build-build_config). If `None`,
          default to `"build.config.db845c"`.
        outs: See [kernel_build.outs](#kernel_build-outs).
        module_outs: See [kernel_build.module_outs](#kernel_build-module_outs). The list of
          in-tree kernel modules.
        define_abi_targets: See [kernel_abi.define_abi_targets](#kernel_abi-define_abi_targets).
        kmi_symbol_list: See [kernel_build.kmi_symbol_list](#kernel_build-kmi_symbol_list).
        kmi_symbol_list_add_only: See [kernel_abi.kmi_symbol_list_add_only](#kernel_abi-kmi_symbol_list_add_only).
        module_grouping: See [kernel_abi.module_grouping](#kernel_abi-module_grouping).
        unstripped_modules_archive: See [kernel_abi.unstripped_modules_archive](#kernel_abi-unstripped_modules_archive).
        gki_modules_list: List of gki modules to be copied to the dist directory.
          If `None`, all gki kernel modules will be copied.
        dist_dir: Argument to `copy_to_dist_dir`. If `None`, default is `"out/{BRANCH}/dist"`.
    """

    if build_config == None:
        build_config = "build.config.db845c"

    if kmi_symbol_list == None:
        kmi_symbol_list = "//common:android/abi_gki_aarch64_db845c" if define_abi_targets else None

    if kmi_symbol_list_add_only == None:
        kmi_symbol_list_add_only = True if define_abi_targets else None

    if gki_modules_list == None:
        gki_modules_list = [":kernel_aarch64_modules"]

    if dist_dir == None:
        dist_dir = "out/{branch}/dist".format(branch = BRANCH)

    # Also refer to the list of ext modules for ABI monitoring targets
    _kernel_modules = ext_modules;

    kernel_build(
        name = name,
        outs = outs,
        srcs = [":common_kernel_sources"],
        # List of in-tree kernel modules.
        module_outs = module_outs,
        build_config = build_config,
        # Enable mixed build.
        base_kernel = ":kernel_aarch64",
        kmi_symbol_list = kmi_symbol_list,
        collect_unstripped_modules = _COLLECT_UNSTRIPPED_MODULES,
        strip_modules = True,
    )

    # enable ABI Monitoring
    # based on the instructions here:
    # https://android.googlesource.com/kernel/build/+/refs/heads/master/kleaf/docs/abi_device.md
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
        ":kernel_aarch64",
        ":kernel_aarch64_additional_artifacts",
        name + "_merged_kernel_uapi_headers",
    ]

    copy_to_dist_dir(
        name = name + "_dist",
        data = dist_targets + gki_modules_list,
        dist_dir = dist_dir,
        flat = True,
        log = "info",
    )
