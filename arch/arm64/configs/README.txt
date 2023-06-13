This file describes the configs using in building different project.

1. There are some building situations with different configs combination as follows:
    (1) Building smarthome with clang or gcc:
        command:     ./common/common_drivers/scripts/amlogic/mk_smarthome64.sh or mk_smarthome64_clang.sh
        config:      common_drivers/arch/arm64/configs/meson64_a64_smarthome_defconfig
        command:     ./common/common_drivers/scripts/amlogic/mk_smarthome32.sh or mk_smarthome32_clang.sh
        config:      common_drivers/arch/arm/configs/meson64_a32_smarthome_defconfig
        savedefconfig cmd:
                     ./common/common_drivers/scripts/amlogic/mk_smarthome64.sh --savedefconfig
                     ./common/common_drivers/scripts/amlogic/mk_smarthome32.sh --savedefconfig

    (2) Building with mk_gx64.sh:
        command:     ./common/common_drivers/scripts/amlogic/mk_gx64.sh
        config:      gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug + amlogic_gcc64_defconfig
        config path：common/arch/arm64/configs/gki_defconfig
                     common/common_drivers/arch/arm64/configs/amlogic_gki.fragment or amlogic_gki.10 or amlogic_gki.debug or amlogic_gcc64_defconfig
        savedefconfig cmd:
                     ./common/common_drivers/scripts/amlogic/mk_gx64.sh --savedefconfig

    (3) Building with mk_gx32.sh:
        command:     ./common/common_drivers/scripts/amlogic/mk_gx32.sh
        config:      a32_base_defconfig + amlogic_a32.fragment + amlogic_gcc32_defconfig
        config path：(all the configs are in the same following path especially gki_defconfig in this situations)
                     common_drivers/arch/arm/configs/
        savedefconfig cmd:
                     ./common/common_drivers/scripts/amlogic/mk_gx32.sh --savedefconfig

    (4) Building with mk_c3_min.sh:
        command:     ./common/common_drivers/scripts/amlogic/mk_c3_min.sh
        config:      meson64_a32_C3_mini_defconfig
        config path：common/common_drivers/arch/arm/configs/
        savedefconfig cmd:
                     ./common/common_drivers/scripts/amlogic/mk_c3_min.sh --savedefconfig

        command:     ./common/common_drivers/scripts/amlogic/mk_c3_min.sh --c3_debug
        config:      meson64_a32_C3_mini_defconfig + C3_debug_defconfig
        config path：common/common_drivers/arch/arm/configs/
        savedefconfig cmd:
                     ./common/common_drivers/scripts/amlogic/mk_c3_min.sh --c3_debug --savedefconfig

    (5) Building with mk_riscv.sh:
        command:     ./common/common_drivers/scripts/amlogic/mk_riscv.sh
        config:      common_drivers/arch/arm64/configs/meson64_a64_smarthome_defconfig
        savedefconfig cmd:
                     ./common/common_drivers/scripts/amlogic/mk_riscv.sh --savedefconfig

    (6) Building zapper:
        command:     ./common/common_drivers/scripts/amlogic/mk_zapper32.sh
        config:      meson64_a32_zapper_defconfig
        config path：common/common_drivers/arch/arm/configs/
        savedefconfig cmd:
                     ./common/common_drivers/scripts/amlogic/mk_zapper32.sh --savedefconfig

    (7) Building with mk.sh with aarch64:
        ./mk.sh                 gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug
        ./mk.sh --gki_debug     gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug
        ./mk.sh --gki_20        gki_defconfig + amlogic_gki.fragment
        config path:            common/arch/arm64/configs/gki_defconfig
                                common_drivers/arch/arm64/configs/amlogic_gki.fragment or amlogic_gki.10 or amlogic_gki.debug
        NOTE:
            gki_defconfig           This is the gki common config file which is used to configure the Image and GKI Module. It can not be changed.
            amlogic_gki.fragment    This is the config file for amlogic only and it follow the GKI2.0.
            amlogic_gki.10          This is the config file for optimizing the product with GKI1.0 and it can change the config in gki kernel. It follows GKI1.0.
            amlogic_gki.debug       This is the config file for adding the function for debug. It follows GKI1.0.

    (8) Building with mk.sh with aarch32:
        ./mk.sh --arch arm      a32_base_defconfig + amlogic_a32.fragment
        config path:            (all the configs are in the same following path especially gki_defconfig in this situations)
                                common_drivers/arch/arm32/configs/
        NOTE:
            gki_defconfig       This config is different from common/arch/arm64/configs/gki_defconfig for aarch64 build. It can be changed by our needs.
            amlogic_gki.fragment  This config is link from common_drivers/arch/arm64/configs/amlogic_gki.fragment which is a common config for 64 and 32
            amlogic_a32.fragment This config is extra config file only for 32bit. if you want to change config for 32bit, please modify amlogic_a32.fragment

    (9)Building with mk.sh for Android p, r and s
        ./mk.sh --upgrade s     gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug + amlogic_upgrade_s.defconfig
        ./mk.sh --upgrade r     gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug + amlogic_upgrade_r.defconfig
        ./mk.sh --upgrade p     gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug + amlogic_upgrade_p.defconfig

        config path:            common/arch/arm64/configs/amlogic_upgrade_s.defconfig or amlogic_upgrade_r.defconfig or amlogic_upgrade_p.defconfig

        NOTE:
            amlogic_upgrade_p.defconfig     this config is for Android p + kernel 5.15
            amlogic_upgrade_r.defconfig     this config is for Android r + kernel 5.15
            amlogic_upgrade_s.defconfig     this config is for Android s + kernel 5.15

    NOTE:
        If you want to change the config in the situations (1)~(6). you must excute the savedefconfig cmd then you can submit config file.

2. If you wants to add new configs based on amlogic configs, you can add configs like this:
    (1) Adding a new config a_config in common_drivers/arch/arm64/configs/ and building with parameter --dev_config.
        Example:
            ./mk.sh --dev_config a_config

    (2) Adding some different config files in common_drivers/arch/arm64/configs/ like a_config b_config and c_config.
        Example:
            ./mk.sh --dev_config a_config+b_config+c_config
