# 使用海思 OpenOCD 调试 WS63

> 来源：以下 WS63 功能与硬件验证记录引自 [PR #22](https://gitcode.com/HiSpark/hs-openocd/pull/22)。本次移植仅完成其自动化测试，未重新进行 WS63 板卡或 Flash/RTOS 实测。

本指南面向已经开启 SWD 的 WS63/GD25Q32 开发板，使用新增的 `ws63.cfg`。
它使用海思的 `riscvcs` 后端。原有 `vendorhm/WS63-swd.cfg` 和
`vendorhm/WS63-jlink.cfg` 也使用修复后的目标实现，保留自动 init/halt 和默认不启用 Flash bank 的行为；不要同时加载两个 WS63 目标配置。
功能边界见[WS63 功能参考](ws63.md)。

## 准备开发板和探针

FlashBoot 必须将 GPIO13、GPIO14 设置为 SWD 功能（mode 4），并由启动固件初始化 SFC。
尚未开启 SWD 的板卡，需要先通过串口烧录工具安装相应 FlashBoot。
本驱动不会初始化空片的 SWD 或 SFC。

连接 SWDIO→GPIO13、SWCLK→GPIO14、GND→GND。使用 J-Link 时连接板卡参考电压到 VTref。
一次只连接一只驱动 SWD 信号的探针。

## 构建和连接

使用标准 OpenOCD 构建流程，见 [WS53 文档的构建步骤](ws53.md#构建)。
本仓库使用 `tcl/interface` 和 `tcl/target` 配置，下面平铺发布包的命令应相应改为源码树路径。
本功能的实机验证使用 Linux、CMSIS-DAP v2 和 J-Link；其他平台的验证范围见功能参考。

在发布包根目录运行：

```sh
./openocd -s . -f cmsis-dap.cfg \
  -c 'adapter speed 4000; bindto 127.0.0.1; set WS63_FLASH_BACKUP_DIR ./ws63-flash-backups' \
  -f ws63.cfg -c 'init; halt'
```

源码树内运行时，把 `-s .` 换为 `-s tcl`，配置文件分别使用
`interface/cmsis-dap.cfg` 和 `target/ws63.cfg`。
J-Link 使用 `jlink.cfg` / `interface/jlink.cfg`；其余目标参数相同。
J-Link 可从 `adapter speed 1000` 开始连接，再按探针和板卡条件调整。
连接成功后输出包含 `SWD DPIDR 0x5ba02477` 和 `Examined RISC-V core`。

## 暂停、备份和复位

在 OpenOCD Telnet 控制台执行：

```tcl
halt
flash probe WS63.flash
flash info WS63.flash
dump_image before.bin 0x200000 0x400000
reset halt
WS63.cpu get_reg -force {pc}
```

Flash 探测输出包含 `WS63 GD25Q32: 4 MiB, 4096-byte sectors`。
`reset halt` 后 PC 为 `0x00100000`。
完整备份必须保留在工程和待提交的源码之外。

## 烧录应用并调试

使用与当前 SDK 分区布局匹配的签名 BIN。SDK 在链接后补入 ROM patch 表，
所以直接用 GDB `load` 加载 ELF 不等同于安装可启动的签名应用。
本次使用的 WS63 SDK 1.10.108 应用地址为 `0x230000`；其他布局须先核对打包配置。

先备份完整 Flash，再按镜像覆盖范围擦除、写入和读回核对。
擦除按 4 KiB 扇区进行，边界扇区中未被镜像覆盖的字节也必须保存和恢复。
普通 Flash 编程后执行 `reset halt`，再继续运行。

使用 SDK GDB 加载配套 ELF，并连接：

```gdb
target extended-remote localhost:3333
monitor halt
monitor gdb_breakpoint_override hard
hbreak main
monitor reset halt
maintenance flush register-cache
continue
```

海思版本使用 `gdb_breakpoint_override`，不是新版上游的 `gdb breakpoint_override`。
断开前删除断点；Flash 软件断点会实际改写签名镜像。

## 准备 LiteOS 符号和回溯

运行 `python3 contrib/ws63/prepare_elf.py --help` 查看工具参数。
向工具提供与固件配套的 ELF 和 SDK 工具链；工具输出补充 CFI 的 ELF 及 RTOS Tcl 配置。
将生成的 RTOS 配置放在 `-f ws63.cfg` 后加载，再使用生成的 ELF 连接 GDB。
工具检查结构体布局并确认可加载节内容没有改变，不生成签名镜像。

## 运行回归检查

在无其他调试客户端、CPU 运行于应用且 SRAM 栈有效时执行：

```sh
python3 contrib/ws63/check_core.py --port 6666 --output core-recovery
```

输出目录必须不存在。测试临时借用栈下方 64 字节并保存恢复信息，
检查 RAM、AP1、寄存器、软件断点及单步。测试成功后恢复状态；失败时保留暂停以便恢复。

新增 Flash 操作前必须保存完整镜像，完成测试后核对 Flash/ROM 与预期镜像，
并确认没有残留断点或观察点。
