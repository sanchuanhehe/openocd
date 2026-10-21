# HiSilicon 分支

`hisilicon-ws63-bs21` 基于 OpenOCD 0.12.0 的海思 `riscvcs` 扩展。

- [WS53 调试、构建、完整检查与复位时钟修复](doc/ws53.md)
- [WS63 功能](doc/ws63.md)及[使用指南](doc/ws63-setup.md)

WS63 功能移植自 Padhia Luo 的 [HiSpark PR #22](https://gitcode.com/HiSpark/hs-openocd/pull/22)。
WS53 在 AP1 访问 RISC-V DM，不经过 SEGGER DLL 的 RISC-V 连接流程。
自动创建 AP0/AP1/AP2 原始访问目标，默认 `WS53.cpu` 保持 A 核 GDB 调试。
WS53 不提供 Flash 烧录。已验证应用 attach、复位启动及复位后 GDB 重连；复位前准备 XO 时钟以避免 ROM 关闭 PLL 后持续 WAIT。

实际硬件检查结果见 [验证记录](doc/ws53-validation.md)。所有硬件测试均未擦写 Flash。

已同步 GitCode 的 3322/FT2232H 和公共兼容变更，范围及保留差异见 [同步记录](doc/hisilicon-sync.md)。
