cmd_arch/arm/boot/dts/qcom/../msm8208-qrd-skuc.dtb := ../prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-gcc -E -Wp,-MD,arch/arm/boot/dts/qcom/../.msm8208-qrd-skuc.dtb.d.pre.tmp -nostdinc -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/boot/dts -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/qcom/../.msm8208-qrd-skuc.dtb.dts.tmp arch/arm/boot/dts/qcom/msm8208-qrd-skuc.dts ; /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/qcom/../msm8208-qrd-skuc.dtb -b 0 -i arch/arm/boot/dts/qcom/  -d arch/arm/boot/dts/qcom/../.msm8208-qrd-skuc.dtb.d.dtc.tmp arch/arm/boot/dts/qcom/../.msm8208-qrd-skuc.dtb.dts.tmp ; cat arch/arm/boot/dts/qcom/../.msm8208-qrd-skuc.dtb.d.pre.tmp arch/arm/boot/dts/qcom/../.msm8208-qrd-skuc.dtb.d.dtc.tmp > arch/arm/boot/dts/qcom/../.msm8208-qrd-skuc.dtb.d

source_arch/arm/boot/dts/qcom/../msm8208-qrd-skuc.dtb := arch/arm/boot/dts/qcom/msm8208-qrd-skuc.dts

deps_arch/arm/boot/dts/qcom/../msm8208-qrd-skuc.dtb := \
  arch/arm/boot/dts/qcom/msm8909-qrd-skuc.dtsi \
  arch/arm/boot/dts/qcom/msm8909-qrd.dtsi \
  arch/arm/boot/dts/qcom/msm8909.dtsi \
  arch/arm/boot/dts/qcom/skeleton64.dtsi \
  /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/boot/dts/include/dt-bindings/clock/msm-clocks-8909.h \
  /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/boot/dts/include/dt-bindings/clock/msm-clocks-a7.h \
  arch/arm/boot/dts/qcom/msm8909-ion.dtsi \
  arch/arm/boot/dts/qcom/msm8909-smp2p.dtsi \
  arch/arm/boot/dts/qcom/msm8909-camera.dtsi \
  arch/arm/boot/dts/qcom/msm8909-ipcrouter.dtsi \
  arch/arm/boot/dts/qcom/msm-gdsc-8916.dtsi \
  arch/arm/boot/dts/qcom/msm8909-iommu.dtsi \
  arch/arm/boot/dts/qcom/msm-iommu-v2.dtsi \
  arch/arm/boot/dts/qcom/msm8909-iommu-domains.dtsi \
  arch/arm/boot/dts/qcom/msm8909-gpu.dtsi \
  arch/arm/boot/dts/qcom/msm8909-coresight.dtsi \
  arch/arm/boot/dts/qcom/msm8909-bus.dtsi \
  /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/boot/dts/include/dt-bindings/msm/msm-bus-ids.h \
    $(wildcard include/config/noc.h) \
  /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/boot/dts/include/dt-bindings/msm/msm-bus-rule-ops.h \
  arch/arm/boot/dts/qcom/msm8909-mdss.dtsi \
  arch/arm/boot/dts/qcom/dsi-panel-sim-video.dtsi \
  arch/arm/boot/dts/qcom/dsi-panel-sim-cmd.dtsi \
  arch/arm/boot/dts/qcom/msm8909-mdss-pll.dtsi \
  arch/arm/boot/dts/qcom/msm8909-pm8909.dtsi \
  arch/arm/boot/dts/qcom/msm-pm8909-rpm-regulator.dtsi \
  arch/arm/boot/dts/qcom/msm-pm8909.dtsi \
  arch/arm/boot/dts/qcom/msm8909-regulator.dtsi \
  arch/arm/boot/dts/qcom/msm8909-pm8909-pm.dtsi \
  arch/arm/boot/dts/qcom/msm8909-pinctrl.dtsi \
  arch/arm/boot/dts/qcom/msm8909-camera-sensor-skuc.dtsi \
  arch/arm/boot/dts/qcom/batterydata-qrd-skuc-4v2-2000mah.dtsi \
  arch/arm/boot/dts/qcom/dsi-panel-ili9806e-fwvga-video.dtsi \
  arch/arm/boot/dts/qcom/msm8208.dtsi \

arch/arm/boot/dts/qcom/../msm8208-qrd-skuc.dtb: $(deps_arch/arm/boot/dts/qcom/../msm8208-qrd-skuc.dtb)

$(deps_arch/arm/boot/dts/qcom/../msm8208-qrd-skuc.dtb):
