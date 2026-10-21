# HiSilicon 分支

`hisilicon-ws63-bs21` 基于 OpenOCD 0.12.0 的海思 `riscvcs` 扩展。

- [WS53 调试、构建、完整检查与已知复位限制](doc/ws53.md)
- [WS63 功能](doc/ws63.md)及[使用指南](doc/ws63-setup.md)

WS63 功能移植自 Padhia Luo 的 [HiSpark PR #22](https://gitcode.com/HiSpark/hs-openocd/pull/22)。
WS53 在 AP1 访问 RISC-V DM，不经过 SEGGER DLL 的 RISC-V 连接流程。
WS53 不提供 Flash 烧录。当前应用 attach 调试已验证，完整 ROM 重启后的 SWD WAIT 仍是已知问题。

实际硬件检查结果见 [验证记录](doc/ws53-validation.md)。所有硬件测试均未擦写 Flash。
