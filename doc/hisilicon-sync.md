# GitCode 功能同步（2026-10-21）

来源：`HiSpark/hs-openocd` 的 `sanchuanhehe/hs-openocd:ws53-swd-debug`，
提交 `cce517b3f55a16a196935a8f8525950003c9fffd`。
GitCode 的 `opensource/openocd/` 映射到本仓库根目录。

## 本次补齐

- 3322 非 APB MEM-AP 回退及冷启动 vendor DP bank 0xE/value 3 调试使能。
  WS53 仍单独使用 value 1/AP0 使能序列，不进入 3322 回退。
- `-jlink.cfg` 芯片名识别，平铺文件名、嵌套目录、Windows/Linux 路径处理，
  以及 3322 的 GDB 硬件断点路由。
- 3322 J-Link 目标配置、3322/WS63 的 FT2232H 接口配置。
- CMSIS-DAP USB 枚举结束时释放 libusb context；同时补齐 device list 释放。
- WS63 源码分发清单，并显式列入 WS53 回归脚本、使用说明及版本元数据。
- 上游 `package.json` 和原始外部编译指南。该指南描述 GitCode 布局，
  本仓库的实际构建命令仍以 `doc/ws53.md` 为准。

同步时将芯片名的反向路径扫描改为先判断边界再递减，避免生成缓冲区起点之前的指针。
上述 USB 列表释放和边界修正是合并后的局部修复；因此不宣称整个源码树逐字节相同。
WS53/WS63 既有功能、AP0/AP1/AP2 自动创建和复位时钟修复继续保留。

## 保留的仓库差异

本仓库保留标准 OpenOCD 根目录、锁定的 Jim Tcl/libjaylink 子模块、可执行脚本权限、
GitHub 使用说明和已有示例。GitCode 外层的跨平台打包工程仍保留在其原仓库；
其打包脚本依赖 `opensource/`、`opensrcbin/` 布局，未直接移入本仓库覆盖标准构建。
纯 CRLF/LF 差异及权限差异不作为功能补丁导入。

## 验证

- Linux x64 构建通过；8 项芯片名路由、10 项目标/接口配置和 9 项 ELF 工具测试全部通过。
- 新引入的 J-Link/FT2232H 配置仅做无硬件加载验证。
- WS53 新 A 板完整回归本轮未完成：探针 23121310 在线、VTref 3.3 V，
  但在 SWD 初始化阶段超时，未进入 AP/CPU/GDB 测试。合并版本重试及原 GitCode
  已有二进制对照都停在相同阶段，不能据此判断是本次合并导致；也不宣称本轮实板通过。
  每次退出后均执行探针复位恢复；全程未擦写 Flash。
- 本轮没有 3322、WS63 或 FT2232H 硬件验证，不把配置加载成功当作实机验证。
- `make distdir` 未完成：本地缺少 `tools/git2cl` 子模块，下载源无响应；
  同时提示缺少 `makeinfo`。二进制编译不受这两项源码包/文档依赖影响。

复现命令：

```sh
python3 contrib/hisilicon/test_chip_routing.py /path/to/openocd
python3 contrib/ws53/test_config.py /path/to/openocd
python3 contrib/ws63/test_prepare_elf.py
```
