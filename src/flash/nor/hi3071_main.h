#ifndef HI3071_MAIN_H
#define HI3071_MAIN_H


#define IDLE 0
#define TIMEOUT 1
typedef struct {
    volatile unsigned int           SYS_LOCK                         ; /* 0x0 */
    volatile unsigned int           SYSCTRL_ACCESS_TEST_REG          ; /* 0x4 */
} S_SYSCTRL_LOCK_REGS_TYPE;

typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: wdt_en
        /*
         * 看门狗使能（在wdt_cfg_en为1时可配置）：
         * 1：内部计数器正常计数，看门狗溢出时，使能产生一个高电平的wdt2crg_rst_flag信号，且使能WDT_RFR置位；
         * 0：内部计数器关闭，wdt2crg_rst_flag信号恒为0，WDT_RFR寄存器恒为0。
         */
        unsigned int    wdt_en                : 1   ; /* [0] */
        // Comment of field: wdt_rst_en
        /*
         * 看门狗系统复位使能信号（在wdt_cfg_en为1时可配置）：
         * 1：复位使能；内部计数器正常计数，看门狗溢出时，使能产生一个高电平的wdt2crg_rst_flag信号，且使能WDT_RFR置位；
         * 0：复位不使能，内部计数器正常计数，看门狗溢出时，不产生wdt2crg_rst_flag信号。
         */
        unsigned int    wdt_rst_en            : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_EN;

/* Define the union U_WDT_MOD */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: wdt_cfg_en
        /*
         * 看门狗配置使能位。软件通过写入1来使能看门狗配置功能，在配置使能下，方可对WDT_TCCR进行时间常数的配置。
         */
        unsigned int    wdt_cfg_en            : 1   ; /* [0] */
        // Comment of field: half_int_mask
        /*
         * 看门狗计数到计数周期的1/2中断屏蔽信号。
         * 1：中断屏蔽（默认）
         * 0：中断不屏蔽
         */
        unsigned int    half_int_mask         : 1   ; /* [1] */
        // Comment of field: full_int_mask
        /*
         * 看门狗计数溢出中断屏蔽信号。
         * 1：中断屏蔽(默认)
         * 0：中断不屏蔽
         */
        unsigned int    full_int_mask         : 1   ; /* [2] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 29  ; /* [31..3] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_MOD;

/* Define the union U_WDT_TCCR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: wdt_tccr
        /*
         * （在wdt_cfg_en为1时可配置）
         * 看门狗超时时间设置寄存器。复位值0x0002_7100，用户配置值不能小于0x0000_03E8,一切小于该值的值都被认为是写入0x0000_03E8。
         */
        unsigned int    wdt_tccr              : 21  ; /* [20..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 11  ; /* [31..21] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_TCCR;

/* Define the union U_WDT_FEED */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: wdt_feed
        /*
         * 喂狗值应当为写入一次0xAA。向该寄存器写入一次0xAAh，就完成了一次正确的喂狗操作，这时，看门狗定时器的值清0，重新开始计数。
         */
        unsigned int    wdt_feed              : 8   ; /* [7..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 24  ; /* [31..8] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_FEED;

/* Define the union U_WDT_RFR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: wdt_rfr
        /*
         * 当看门狗溢出，且复位输出使能时，该位置1，软件写1清零。
         * 1：有溢出复位；
         * 0：无溢出复位。
         */
        unsigned int    wdt_rfr               : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_RFR;

/* Define the union U_WDT_ITV */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: wdt_itv
        /*
         * 存储了当前内部定时器值。
         */
        unsigned int    wdt_itv               : 21  ; /* [20..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 11  ; /* [31..21] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_ITV;

/* Define the union U_WDT_RAW_INTR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: half_int_raw
        /*
         * 看门狗计数到计数周期的1/2，软件写1清除中断。
         * 1：计数到周期的1/2
         * 0：计数未到周期的1/2
         */
        unsigned int    half_int_raw          : 1   ; /* [0] */
        // Comment of field: full_int_raw
        /*
         * 看门狗计数溢出，软件写1清除中断。
         * 1：计数到周期
         * 0：计数未到周期
         */
        unsigned int    full_int_raw          : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_RAW_INTR;

/* Define the union U_WDT_INTR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: half_int
        /*
         * 看门狗计数到计数周期的1/2中断屏蔽信号。
         * 1：有中断
         * 0：无中断或中断被屏蔽
         */
        unsigned int    half_int              : 1   ; /* [0] */
        // Comment of field: full_int
        /*
         * 看门狗计数溢出中断。
         * 1：有中断
         * 0：无中断或中断被屏蔽
         */
        unsigned int    full_int              : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_INTR;

/* Define the union U_WDT_TCCR_HALF */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: wdt_tccr_half
        /*
         * （在wdt_cfg_en为1时可配置）
         * 看门狗超时时间设置寄存器。复位值0x0001_3880，用户配置值不能小于0x0000_01F4,一切小于该值的值都被认为是写入0x0000_01F4。
         */
        unsigned int    wdt_tccr_half         : 21  ; /* [20..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 11  ; /* [31..21] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_WDT_TCCR_HALF;
typedef struct {
    volatile U_WDT_EN               WDT_EN                           ; /* 0x0 */
    volatile U_WDT_MOD              WDT_MOD                          ; /* 0x4 */
    volatile U_WDT_TCCR             WDT_TCCR                         ; /* 0x8 */
    volatile U_WDT_FEED             WDT_FEED                         ; /* 0xc */
    volatile U_WDT_RFR              WDT_RFR                          ; /* 0x10 */
    volatile U_WDT_ITV              WDT_ITV                          ; /* 0x14 */
    volatile U_WDT_RAW_INTR         WDT_RAW_INTR                     ; /* 0x18 */
    volatile U_WDT_INTR             WDT_INTR                         ; /* 0x1c */
    volatile U_WDT_TCCR_HALF        WDT_TCCR_HALF                    ; /* 0x20 */
} S_WDT_REGS_TYPE;

#define S_SYSCTRL_LOCK_BASE      0x40000000
#define S_WDT_BASE               0x40002000
#define reg_sysCtrlLock    (*(volatile S_SYSCTRL_LOCK_REGS_TYPE *)S_SYSCTRL_LOCK_BASE)
#define reg_watchDog       (*(volatile S_WDT_REGS_TYPE *)S_WDT_BASE)
#define reg_irqClr         (*(volatile U32*)0xE000E180) 
#define  __STATIC_INLINE           static inline
#define __ASM                __asm
__attribute__((always_inline)) __STATIC_INLINE void __DSB(void)
{
    __ASM volatile ("dsb 0xF":::"memory");
}

#define PFLASH_ENABLE 0x40000214 // BIT2 BIT3   0 开始
#define PFLASH_STATE 0x40000264 // BIT4         0 开始
#define PFLASH_MEM_CTRL_ADDR 0x00100000 // 基地址

#define BIT(x)                       ((U32)((U32)0x01U << (x)))
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#ifndef UNLOCK_PFLASH_WRITE
#define UNLOCK_PFLASH_WRITE(cmdAddr) {  \
    __DSB(); \
    *(U32 *)(cmdAddr + 0xaa8) = 0xAA; \
    __DSB(); \
    *(U32 *)(cmdAddr + 0xaa8) = 0xA0; \
    __DSB(); \
}
#endif


#ifndef UNLOCK_PFLASH_WRITE_NVR
#define UNLOCK_PFLASH_WRITE_NVR(cmdAddr) {  \
    __DSB(); \
    *(U32 *)(cmdAddr + 0xaa8) = 0xAA; \
    __DSB(); \
    *(U32 *)(cmdAddr + 0xaa8) = 0xC0; \
    __DSB(); \
}
#endif

#ifndef UNLOCK_PFLASH_ERASE_ALL
#define UNLOCK_PFLASH_ERASE_ALL(cmdAddr) {  \
    __DSB(); \
    *(U32 *)(cmdAddr + 0xaa8) = 0xAA; \
    __DSB(); \
    *(U32 *)(cmdAddr + 0xaa8) = 0x80; \
    __DSB(); \
    *(U32 *)(cmdAddr + 0x554) = 0x55; \
    __DSB(); \
    *(U32 *)(cmdAddr + 0xaa8) = 0x10; \
    __DSB(); \
}
#endif

typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_glb_sft_clk_sel
        /*
         * 时钟切换过程时钟选择：
         * 110：100K；
         * 101：内部25Mhz时钟；
         * 011：PLL输出给SOC的时钟；
         * 其它：禁止配置
         */
        unsigned int    cfg_glb_sft_clk_sel   : 3   ; /* [2..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 29  ; /* [31..3] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_CLK_SWITCH;

/* Define the union U_SYSCTRL_REF_CLK_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_pll_ref_sel
        /*
         * PLL的参考时钟选择配置寄存器：
         * 00,10：选择内部osc25m；
         * 01：选择外部晶振；
         * 11：选择从外部测试管脚送入的时钟.(测试使用)。
         */
        unsigned int    cfg_pll_ref_sel       : 2   ; /* [1..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_REF_CLK_SEL;

/* Define the union U_SYSCTRL_CLK_DETECT_EN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_clk_detect_en
        /*
         * 时钟检测使能，bit1对应片内25Mhz，bit0对应外部晶振：
         * 0：不使能；
         * 1：使能。
         */
        unsigned int    cfg_clk_detect_en     : 2   ; /* [1..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_CLK_DETECT_EN;

/* Define the union U_SYSCTRL_CPU_FREQ_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_cpu_freq_sel
        /*
         * cpu核主频率选择指示：
         * 00：1分频；
         * 01：2分频；
         * 10：4分频，
         * 11：8分频
         */
        unsigned int    cfg_cpu_freq_sel      : 2   ; /* [1..0] */
        // Comment of field: cfg_apb_freq_sel
        /*
         * APB频率选择指示：
         * 00：1分频；
         * 01：2分频；
         * 10：4分频，
         * 11：8分频
         */
        unsigned int    cfg_apb_freq_sel      : 2   ; /* [3..2] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 28  ; /* [31..4] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_CPU_FREQ_SEL;

/* Define the union U_SYSCTRL_PERI_FREQ_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_saradc_freq_sel
        /*
         * SARADC频率选择指示：
         * bit[2:0]对应关系为：
         * 000：4分频；
         * 001：8分频:
         * 010：16分频；
         * 011：24分频；
         * 100：32分频；
         * bit[3]对应为：
         * 0：为25%占空比；
         * 1：固定2ns的脉宽
         */
        unsigned int    cfg_saradc_freq_sel   : 4   ; /* [3..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 28  ; /* [31..4] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PERI_FREQ_SEL;

/* Define the union U_SYSCTRL_LOOP_FREQ_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_loop_freq_sel
        /*
         * LOOP频率选择指示：
         * 00：1分频；
         * 01：2分频；
         * 10：4分频，
         * 11：8分频
         */
        unsigned int    cfg_loop_freq_sel     : 2   ; /* [1..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_LOOP_FREQ_SEL;

/* Define the union U_SYSCTRL_SOC_PERI_CLK_EN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_soc_slave_clk_en
        /*
         * SOC外设时钟使能：
         * 0：不使能；
         * 1：使能。
         * 其中：
         * bit[10]为clk_spi时钟输出使能信号。
         * bit[9] 为clk_pwm时钟输出使能信号；
         * bit[8] 为clk_gpio_qual时钟输出使能信号；
         * bit[7] 为clk_ecap时钟输出使能信号；
         * bit[6] 为clk_uart1时钟输出使能信号；
         * bit[5] 为clk_uart0时钟输出使能信号；
         * bit[4] 为clk_gpio时钟输出使能信号；
         * bit[3] 为clk_can时钟输出使能信号；
         * bit[2] 为clk_pmbus1时钟输出使能信号；
         * bit[1] 为clk_pmbus0时钟输出使能信号；
         * bit[0] 为clk_timer时钟输出使能信号；
         */
        unsigned int    cfg_soc_slave_clk_en  : 16  ; /* [15..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 16  ; /* [31..16] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_SOC_PERI_CLK_EN;

/* Define the union U_SYSCTRL_ANA_CLK_EN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_ana_clk_en
        /*
         * 输出给模拟IP的时钟使能（对应关系带CRG提供）：
         * 0：不使能；
         * 1：使能。
         */
        unsigned int    cfg_ana_clk_en        : 12  ; /* [11..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 20  ; /* [31..12] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_ANA_CLK_EN;

/* Define the union U_SYSCTRL_GLB_SRST_EN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_glb_sft_rst
        /*
         * 全局软复位配置寄存器：
         * 0：复位；
         * 1：不复位。
         */
        unsigned int    cfg_glb_sft_rst       : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_GLB_SRST_EN;

/* Define the union U_SYSCTRL_SOC_SRST_EN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_soc_sft_rst
        /*
         * soc外设软复位：
         * 0：复位；
         * 1：不复位。
         * 其中：
         * bit[10]为rst_spi_n软复位使能信号。
         * bit[9] 为rst_pwm_n软复位使能信号；
         * bit[8] 为rst_gpio_qual_n软复位使能信号；
         * bit[7] 为rst_ecap_n软复位使能信号；
         * bit[6] 为rst_uart1_n软复位使能信号；
         * bit[5] 为rst_uart0_n软复位使能信号；
         * bit[4] 为rst_gpio_n软复位使能信号；
         * bit[3] 为rst_can_n软复位使能信号；
         * bit[2] 为rst_pmbus1_n软复位使能信号；
         * bit[1] 为rst_pmbus0_n软复位使能信号；
         * bit[0] 为rst_timer_n软复位使能信号；
         * 其余暂未使用。
         */
        unsigned int    cfg_soc_sft_rst       : 16  ; /* [15..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 16  ; /* [31..16] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_SOC_SRST_EN;

/* Define the union U_SYSCTRL_FLASH_CHKSUM_SADDR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_flash_chksum_saddr
        /*
         * checksum计算起始地址，按flash macro物理地址进行配置，逻辑直接使用。 注：约束cfg_flash_chksum_eaddr>= cfg_flash_chksum_saddr
         */
        unsigned int    cfg_flash_chksum_saddr : 17  ; /* [16..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 15  ; /* [31..17] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FLASH_CHKSUM_SADDR;

/* Define the union U_SYSCTRL_FLASH_CHKSUM_EADDR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_flash_chksum_eaddr
        /*
         * checksum计算结束地址，按flash macro物理地址进行配置，逻辑直接使用。注：逻辑内部真实读取flash macro物理地址空间个数为：cfg_flash_chksum_eaddr-cfg_flash_chksum_saddr+1
         */
        unsigned int    cfg_flash_chksum_eaddr : 17  ; /* [16..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 15  ; /* [31..17] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FLASH_CHKSUM_EADDR;

/* Define the union U_SYSCTRL_FLASH_CHKSUM_START */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_flash_chksum_start
        /*
         * checksum计算启动指示，软件写0、写1触发一次启动。
         */
        unsigned int    cfg_flash_chksum_start : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FLASH_CHKSUM_START;

/* Define the union U_FLASH_CHKSUM__STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: rpt_flash_chksum_over
        /*
         * flash上报checksum完成指示，软件根据该指示值为1时取对应的 pflash_chksum_out结果。
         * 0：未启动或计算未完成；
         * 1：chksum计算完成。
         */
        unsigned int    rpt_flash_chksum_over : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_FLASH_CHKSUM__STATUS;

/* Define the union U_SYSCTRL_FLASH_RWEN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: dflash_nvr_wen
        /*
         * DFLASH NVR空间写\擦除使能：
         * 0：不使能，禁止写\擦除操作；
         * 1：使能，允许写\擦除操作；
         * 注：软件固定配置为0，禁止软件修改该值（sysctrl2dflash_wen=1且sysctrl2dflash_nvr_wen=1时，nvr空间才允许写\擦除。）
         */
        unsigned int    dflash_nvr_wen        : 1   ; /* [0] */
        // Comment of field: dflash_wen
        /*
         * DFLASH MAIN及NVR空间写\擦除使能：
         * 0：不使能，禁止写\擦除操作；
         * 1：使能，允许写\擦除操作；
         */
        unsigned int    dflash_wen            : 1   ; /* [1] */
        // Comment of field: pflash_nvr_wen
        /*
         * PFLASH NVR空间写\擦除使能：
         * 0：不使能，禁止写操作；
         * 1：使能，允许写操作；
         * 注1：sysctrl2pflash_wen=1且sysctrl2pflash_nvr_wen=1时，nvr空间才允许写。
         * 注2：真实写\擦除还受安全启动模式控制。
         */
        unsigned int    pflash_nvr_wen        : 1   ; /* [2] */
        // Comment of field: pflash_wen
        /*
         * PFLASH MAIN及NVR空间写\擦除使能：
         * 0：不使能，禁止写\擦除操作；
         * 1：使能，允许写\擦除操作；
         * 注：真实写\擦除还受安全启动模式控制。
         */
        unsigned int    pflash_wen            : 1   ; /* [3] */
        // Comment of field: dflash_nvr_cfg_ren
        /*
         * dflash nvr_cfg 读使能信号。
         * 0：不使能；
         * 1：使能。
         * 注：读nvr cfg只在调试下使用，读之前需将该使能拉高，按正常main空间方式读取，读完后必须拉低该使能；同时对DFLASH在读取前还需拉低ecc_en，读完后在按当前ecc_en实际应用场景进行拉高处理。
         */
        unsigned int    dflash_nvr_cfg_ren    : 1   ; /* [4] */
        // Comment of field: pflash_nvr_cfg_ren
        /*
         * pflash nvr_cfg 读使能信号。
         * 0：不使能；
         * 1：使能。
         * 注：读nvr cfg只在调试下使用，读之前需将该使能拉高，按正常main空间方式读取，读完后必须拉低该使能。
         */
        unsigned int    pflash_nvr_cfg_ren    : 1   ; /* [5] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 26  ; /* [31..6] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FLASH_RWEN;

/* Define the union U_SYSCTRL_P_DFLASH_TIMING */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2pflash_tacc_set
        /*
         * PFLASH LVCTL=0，flash 读1个数据返回稳定时间：(25ns)，配置原则为cpu工作频率下生效，配置值=数据返回稳定时间+1个cpu时钟周期；
         * 25Mhz固定配置为0x2；
         * 31.25Mhz固定配置为0x2；
         * 62.5Mhz固定配置为0x3;
         * 125MHz固定配置为0x4；
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_tacc_set   : 4   ; /* [3..0] */
        // Comment of field: sys2dflash_tacc_set
        /*
         * DFLASH LVCTL=0，flash 读1个数据返回稳定时间：(25ns)，配置原则为cpu工作频率下生效，配置值=数据返回稳定时间+1个cpu时钟周期；
         * 25Mhz固定配置为0x2；
         * 31.25Mhz固定配置为0x2；
         * 62.5Mhz固定配置为0x3;
         * 125MHz固定配置为0x4；
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_tacc_set   : 4   ; /* [7..4] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 24  ; /* [31..8] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_P_DFLASH_TIMING;

/* Define the union U_SYSCTRL_PFLASH_TIMING0 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: syspflash_tnvs_set
        /*
         * PFLASH Tnvs(>4us)时间设定：
         * 25Mhz固定配置为0x07D(~5us)；
         * 31.25Mhz固定配置为0x09C；
         * 62.5 Mhz固定配置为0x138；
         * 125MHz固定配置为0x271(~5us)。
         * 禁止配置为其它值。
         */
        unsigned int    syspflash_tnvs_set    : 12  ; /* [11..0] */
        // Comment of field: sys2pflash_tprog_set
        /*
         * PFLASH Tprog(8us~10us)时间设定：
         * 25Mhz固定配置为0x0E1(~9us)；
         * 31.25Mhz固定配置为0x119；
         * 62.5 Mhz固定配置为0x232；
         * 125MHz固定配置为0x465(~9us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_tprog_set  : 12  ; /* [23..12] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 8   ; /* [31..24] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PFLASH_TIMING0;

/* Define the union U_SYSCTRL_PFLASH_TIMING1 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2pflash_tpgs_set
        /*
         * PFLASH Tpgs(>8us)时间设定：
         * 25Mhz固定配置为0x0E1(~9us) ；
         * 31.25Mhz固定配置为0x119；
         * 62.5 Mhz固定配置为：0x232；
         * 125MHz固定配置为0xh465(~9us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_tpgs_set   : 12  ; /* [11..0] */
        // Comment of field: sys2pflash_trcv_prog_set
        /*
         * PFLASH Program Trcv(>5us)时间设定：
         * 25Mhz固定配置为0x096(~6us)；
         * 31.25Mhz固定配置为0x0BB；
         * 62.5 Mhz固定配置为0x177；
         * 125MHz固定配置为0x2EE(~6us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_trcv_prog_set : 12  ; /* [23..12] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 8   ; /* [31..24] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PFLASH_TIMING1;

/* Define the union U_SYSCTRL_PFLASH_TIMING2 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2pflash_trcv_sector_ers_set
        /*
         * PFLASH Sector-Erase Trcv(>50us)时间设定：
         * 25Mhz固定配置为0x05dc(~60us)；
         * 31.25Mhz固定配置为0x0753；
         * 62.5 Mhz固定配置为0x0EA6；
         * 125MHz固定配置为0x1D4C(~60us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_trcv_sector_ers_set : 14  ; /* [13..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 2   ; /* [15..14] */
        // Comment of field: sys2pflash_rcv_chip_ers_set
        /*
         * PFLASH Chip-Erase Trcv(>200us)时间设定：
         * 25Mhz固定配置为0x1482(~210us)；
         * 31.25Mhz固定配置为0x19A2；
         * 62.5 Mhz固定配置为0x3345；
         * 125MHz固定配置为0x668A(~210us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_rcv_chip_ers_set : 16  ; /* [31..16] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PFLASH_TIMING2;

/* Define the union U_SYSCTRL_PFLASH_TIMING3 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2pflash_terase_chip_ers_set
        /*
         * PFLASH Chip Terase(8ms~10ms)时间设定：
         * 25Mhz固定配置为0x036EE8 (~9ms)；
         * 31.25Mhz固定配置为0x044AA2；
         * 62.5 Mhz固定配置为0x089544；
         * 125MHz固定配置为0x112A88(~9ms)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_terase_chip_ers_set : 22  ; /* [21..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 10  ; /* [31..22] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PFLASH_TIMING3;

/* Define the union U_SYSCTRL_PFLASH_TIMING4 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2pflash_terase_sec_ers_set
        /*
         * PFLASH sector Terase(8ms~10ms)时间设定：
         * 25Mhz固定配置为0x036EE8 (~9ms)；
         * 31.25Mhz固定配置为0x044AA2；
         * 62.5 Mhz固定配置为0x089544；
         * 125MHz固定配置为0x112A88(~9ms)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2pflash_terase_sec_ers_set : 22  ; /* [21..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 10  ; /* [31..22] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PFLASH_TIMING4;

/* Define the union U_SYSCTRL_DFLASH_TIMING0 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2dflash_tnvs_set
        /*
         * DFLASH Tnvs(>4us)时间设定：
         * 25Mhz固定配置为0x07D(~5us)；
         * 31.25Mhz固定配置为0x09C；
         * 62.5 Mhz固定配置为0x138；
         * 125MHz固定配置为0x271(~5us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_tnvs_set   : 12  ; /* [11..0] */
        // Comment of field: sys2dflash_tprog_set
        /*
         * DFLASH Tprog(8us~10us)时间设定：
         * 25Mhz固定配置为0x0E1(~9us)；
         * 31.25Mhz固定配置为0x119；
         * 62.5 Mhz固定配置为0x232；
         * 125MHz固定配置为0x465(~9us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_tprog_set  : 12  ; /* [23..12] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 8   ; /* [31..24] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_DFLASH_TIMING0;

/* Define the union U_SYSCTRL_DFLASH_TIMING1 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2dflash_tpgs_set
        /*
         * DFLASH Tpgs(>8us)时间设定：
         * 25Mhz固定配置为0x0E1(~9us) ；
         * 31.25Mhz固定配置为0x119；
         * 62.5 Mhz固定配置为：0x232；
         * 125MHz固定配置为0xh465(~9us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_tpgs_set   : 12  ; /* [11..0] */
        // Comment of field: sys2dflash_trcv_prog_set
        /*
         * DPFLASH Program Trcv(>5us)时间设定：
         * 25Mhz固定配置为0x096(~6us)；
         * 31.25Mhz固定配置为0x0BB；
         * 62.5 Mhz固定配置为0x177；
         * 125MHz固定配置为0x2EE(~6us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_trcv_prog_set : 12  ; /* [23..12] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 8   ; /* [31..24] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_DFLASH_TIMING1;

/* Define the union U_SYSCTRL_DFLASH_TIMING2 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2dflash_trcv_sector_ers_set
        /*
         * DFLASH Sector-Erase Trcv(>50us)时间设定：
         * 25Mhz固定配置为0x05dc(~60us)；
         * 31.25Mhz固定配置为0x0753；
         * 62.5 Mhz固定配置为0x0EA6；
         * 125MHz固定配置为0x1D4C(~60us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_trcv_sector_ers_set : 14  ; /* [13..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 2   ; /* [15..14] */
        // Comment of field: sys2dflash_rcv_chip_ers_set
        /*
         * DFLASH Chip-Erase Trcv(>200us)时间设定：
         * 25Mhz固定配置为0x1482(~210us)；
         * 31.25Mhz固定配置为0x19A2；
         * 62.5 Mhz固定配置为0x3345；
         * 125MHz固定配置为0x668A(~210us)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_rcv_chip_ers_set : 16  ; /* [31..16] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_DFLASH_TIMING2;

/* Define the union U_SYSCTRL_DFLASH_TIMING3 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2dflash_terase_chip_ers_set
        /*
         * DFLASH Chip Terase(8ms~10ms)时间设定：
         * 25Mhz固定配置为0x036EE8 (~9ms)；
         * 31.25Mhz固定配置为0x044AA2；
         * 62.5 Mhz固定配置为0x089544；
         * 125MHz固定配置为0x112A88(~9ms)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_terase_chip_ers_set : 22  ; /* [21..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 10  ; /* [31..22] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_DFLASH_TIMING3;

/* Define the union U_SYSCTRL_DFLASH_TIMING4 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2dflash_terase_sec_ers_set
        /*
         * DFLASH sector Terase(8ms~10ms)时间设定：
         * 25Mhz固定配置为0x036EE8 (~9ms)；
         * 31.25Mhz固定配置为0x044AA2；
         * 62.5 Mhz固定配置为0x089544；
         * 125MHz固定配置为0x112A88(~9ms)。
         * 禁止配置为其它值。
         */
        unsigned int    sys2dflash_terase_sec_ers_set : 22  ; /* [21..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 10  ; /* [31..22] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_DFLASH_TIMING4;

/* Define the union U_SYSCTRL_TIMING_LOAD */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pflash_timing_load
        /*
         * pflash时序加载配置寄存器，写0写1完成一次加载。
         */
        unsigned int    pflash_timing_load    : 1   ; /* [0] */
        // Comment of field: dflash_timing_load
        /*
         * dflash时序加载配置寄存器，写0写1完成一次加载。
         */
        unsigned int    dflash_timing_load    : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_TIMING_LOAD;

/* Define the union U_SYSCTRL_TIMING_LOAD_OVER */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pflash_timing_load_over
        /*
         * pflash时序加载完成指示：
         * 0：未发起时序参数加载或时序参数加载未完成；
         * 1：已完成
         */
        unsigned int    pflash_timing_load_over : 1   ; /* [0] */
        // Comment of field: dflash_timing_load_over
        /*
         * dflash时序加载完成指示：
         * 0：未发起时序参数加载或时序参数加载未完成；
         * 1：已完成
         */
        unsigned int    dflash_timing_load_over : 1   ; /* [1] */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_TIMING_LOAD_OVER;

/* Define the union U_DFLASH_ECC_EN_CFG */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: dflash_ecc_en
        /*
         * DFLASH ECC使能：
         * 0：不使能；
         * 1：使能；
         * 注：在开启使能后，读取的数据必须为ECC使能开启后写入的数据，否则将报ECC错误（只有MAIN空间才支持ECC）。
         */
        unsigned int    dflash_ecc_en         : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_DFLASH_ECC_EN_CFG;

/* Define the union U_SYSCTRL_FLASH_STATE_RPT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pflash_main_state
        /*
         * PFLASH状态机当前状态上报
         * 0：初始态；
         * 1：UNLOCK1，写、擦除解锁状态1（等待判断进入PROG、 SECT_ERASE、 CHIP_ERASE的条件）；
         * 2：NVR读状态（等待CPU下发读FLASH具体的读使能及读地址，下发后状态机回到初始态）；
         * 3：PROG，写状态，此时FLASH颗粒正在进行写操作；
         * 4：指示当前即将进入擦除模式，等待解锁状态2；
         * 5：完成解锁状态2，等待CPU下发真实的SECTOR\CHIP擦除指令；
         * 6：真实FLASH颗粒进行SECT_ERASE\CHIP_ERASE擦除。
         * 其余值为非法状态。
         */
        unsigned int    pflash_main_state     : 4   ; /* [3..0] */
        // Comment of field: pflash2sys_busy
        /*
         * PFLASH写、擦除状态指示：
         * 0：未进行写、擦除操作，或已完成写擦操作；
         * 1：正在进行写、擦除操作；
         */
        unsigned int    pflash2sys_busy       : 1   ; /* [4] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 3   ; /* [7..5] */
        // Comment of field: dflash_main_state
        /*
         * DFLASH状态机当前状态上报
         * 0：初始态；
         * 1：UNLOCK1，写、擦除解锁状态1（等待判断进入PROG、 SECT_ERASE、 CHIP_ERASE的条件）；
         * 2：NVR读状态（等待CPU下发读FLASH具体的读使能及读地址，下发后状态机回到初始态）；
         * 3：PROG，写状态，此时FLASH颗粒正在进行写操作；
         * 4：指示当前即将进入擦除模式，等待解锁状态2；
         * 5：完成解锁状态2，等待CPU下发真实的SECTOR\CHIP擦除指令；
         * 6：真实FLASH颗粒进行SECT_ERASE\CHIP_ERASE擦除。
         * 其余值为非法状态。
         */
        unsigned int    dflash_main_state     : 4   ; /* [11..8] */
        // Comment of field: dflash2sys_busy
        /*
         * DFLASH写、擦除状态指示：
         * 0：未进行写、擦除操作，或已完成写擦操作；
         * 1：正在进行写、擦除操作；
         */
        unsigned int    dflash2sys_busy       : 1   ; /* [12] */
        // Comment of field: reserved_1
        /*
         * 保留
         */
        unsigned int    reserved_1            : 19  ; /* [31..13] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FLASH_STATE_RPT;

/* Define the union U_SYSCTRL_DFLASH_ECC_ERR_RPT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: dflash2sys_ecc_single_err
        /*
         * DFLASH数据32bit中1bit出错
         */
        unsigned int    dflash2sys_ecc_single_err : 1   ; /* [0] */
        // Comment of field: dflash2sys_ecc_multi_err
        /*
         * DFLASH数据32bit中多bit出错
         */
        unsigned int    dflash2sys_ecc_multi_err : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_DFLASH_ECC_ERR_RPT;

/* Define the union U_SYSCTRL_DFLASH_ECC_ERR_ADDR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: dflash2sys_ecc_err_addr
        /*
         * DFLASH ECC错误地址；
         * ECC错误时对应地址上报（需在ECC错误时查看该地址值（只能上报ECC错误后最近的一次错误地址）。
         */
        unsigned int    dflash2sys_ecc_err_addr : 17  ; /* [16..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 15  ; /* [31..17] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_DFLASH_ECC_ERR_ADDR;

/* Define the union U_SYSCTRL_FLASH_PROT_ALERT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pflash2sys_prot_nvr_alert
        /*
         * PFLASH NVR保护区间异常写擦操作告警
         * 0：安全保护条件下，未收到写擦操作指示；
         * 1：安全保护条件下，收到写擦操作指示；
         */
        unsigned int    pflash2sys_prot_nvr_alert : 1   ; /* [0] */
        // Comment of field: pflash2sys_prot_addr_alert
        /*
         * PFLASH 安全启动密钥及算法保护区间异常写擦操作告警
         * 0：安全保护条件下，未收到写擦操作指示；
         * 1：安全保护条件下，收到写擦操作指示；
         */
        unsigned int    pflash2sys_prot_addr_alert : 1   ; /* [1] */
        // Comment of field: dflash2sys_prot_nvr_alert
        /*
         * DFLASH NVR保护区间异常写擦操作告警
         * 0：安全保护条件下，未收到写擦操作指示；
         * 1：安全保护条件下，收到写擦操作指示；
         */
        unsigned int    dflash2sys_prot_nvr_alert : 1   ; /* [2] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 29  ; /* [31..3] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FLASH_PROT_ALERT;

/* Define the union U_SYSCTRL_PROT_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pflash2sys_prot_en
        /*
         * pflash安全启动功能使能状态上报
         * 0：已启动安全功能；
         * 1：未启动安全功能。
         */
        unsigned int    pflash2sys_prot_en    : 1   ; /* [0] */
        // Comment of field: pflash2sys_nvr_prot
        /*
         * 安全模式下产生pflash nvr存储空间保护使能上报
         * 0：pflash/dflash 未保护，可进行擦写操作；
         * 1：pflash/dflash 被保护，不可进行擦写操作（只在模式2、3下受到安全模式的保护）。
         */
        unsigned int    pflash2sys_nvr_prot   : 1   ; /* [1] */
        // Comment of field: arm_jtag_prot
        /*
         * arm串行调试口屏蔽指示上报
         * 0：arm串行调试口正常开启。
         * 1：arm串行调试口输入恒为0。
         */
        unsigned int    arm_jtag_prot         : 1   ; /* [2] */
        // Comment of field: chip_jtag_prot
        /*
         * 芯片jtag口屏蔽指示上报
         * 0：芯片jtag口正常开启（模式0~1正常开启，模式2下jtag正常开启，但无法通过jtag访问内部寄存器）。
         * 1：芯片jtag口输入恒为0，此时无法通过JTAG访问芯片内部寄存器，也无法作DFT SCAN测试（模式3）
         */
        unsigned int    chip_jtag_prot        : 1   ; /* [3] */
        // Comment of field: pflash2sys_prot_addr_en
        /*
         * 安全启动 pflash保护密钥、算法空间保护使能指示上报
         * 0：指定的pflash保护密钥、算法空间不做地址保护；
         * 1：指定的pflash保护密钥、算法空间做地址保护。
         */
        unsigned int    pflash2sys_prot_addr_en : 1   ; /* [4] */
        // Comment of field: pflash2sys_prot_sel
        /*
         * 当要做安全时，逻辑判断是否正在进行安全启动过程中：
         * 0：未启动或已做完；
         * 1：正在进行安全启动。
         * 注：5003软件禁止使用。
         */
        unsigned int    pflash2sys_prot_sel   : 1   ; /* [5] */
        // Comment of field: pflash2sys_prot_mode
        /*
         * pflash安全启动模式上报：
         * 2’d0：模式0，该模式下不做任何保护，可通过ARM串行口及芯片JTAG操作所有空间；
         * 2’d1：模式1，该模式下不能通过ARM串行调试口访问芯片所有空间、芯片jtag可以访问芯片所有空间，片内程序可以访问所有flash空间，以及可以通过芯片FLASH测试管脚进行访问FLASH；
         * 2’d2：模式2，该模式下不能通过ARM串行调试口及芯片JTAG访问FLASH，但可以通过JTAG进行DFT SCAN测试，同时ARM程序也不能对保护空间进行修改，保护空间为：PFLASH的加密算法和秘钥存储空间；
         * 2’d3：模式3，该模式下不能通过ARM串行调试口及芯片JTAG访问FLASH，也不能由JTAG进行DFT SCAN测试，同时ARM程序也不能对保护空间进行修改。保护空间为PFLASH的加密算法和秘钥存储空间；
         */
        unsigned int    pflash2sys_prot_mode  : 2   ; /* [7..6] */
        // Comment of field: flash_ip_reg_prot
        /*
         * flash_bist测试口及jtest_covert屏蔽指示
         * 0：flash_bist的6bit串口正常开启，并且可以通过JTAG访问芯片内部寄存器；
         * 1：flash_bist的6bit串口固接为0，且不能通过JTAG访问芯片内部寄存器。
         * 注：此6bit为pflash、dflash共用的sclk、si，独立的psce、pso、dsce、dso。
         */
        unsigned int    flash_ip_reg_prot     : 1   ; /* [8] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 23  ; /* [31..9] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PROT_STATUS;

/* Define the union U_SYSCTRL_PFLASH_PROT_ADDR_BEGIN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pflash2sys_prot_addr_begin
        /*
         * PFLASH安全启动保护区间起始地址（需按每个sector的基地址进行配置，0表示第0块sector的基地址，512表示第1块sector的基地址…，130560表示第255块sector的基地址），此地址为PFLASH颗粒实际的偏移地址，起始地址为起始sector的基地址。
         * bit[17]: 起始地址有效指示
         * 0：有效；
         * 1：无效：
         * bit16~0：起始地址
         * 注：起始地址有效指示与结束地址有效指示均有效时，保护区间地址才能生效。
         */
        unsigned int    pflash2sys_prot_addr_begin : 18  ; /* [17..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 14  ; /* [31..18] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PFLASH_PROT_ADDR_BEGIN;

/* Define the union U_SYSCTRL_PFLASH_PROT_ADDR_END */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pflash2sys_prot_addr_end
        /*
         * PFLASH安全启动保护区间结束地址，结束地址为对应保护区间最后1个sector的最大地址。
         * bit[17]: 结束地址有效指示
         * 0：有效；
         * 1：无效：
         * bit[16:0]：结束地址
         * 注：起始地址有效指示与结束地址有效指示均有效时，保护区间地址才能生效。
         */
        unsigned int    pflash2sys_prot_addr_end : 18  ; /* [17..0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 14  ; /* [31..18] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_PFLASH_PROT_ADDR_END;

/* Define the union U_FLASH_BIST_IO_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: flash_bist_io_sel
        /*
         * FLASH BIST电路管脚选择：
         * 0：CP阶段；
         * 1：FT阶段，需dft复用管脚；
         */
        unsigned int    flash_bist_io_sel     : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_FLASH_BIST_IO_SEL;

/* Define the union U_SYSCTRL_TCM_ECC_BP */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_tcm_ecc_bypass
        /*
         * ecc功能旁路信号：
         * 0：ecc功能不旁路
         * 1：ecc功能旁滤。
         */
        unsigned int    cfg_tcm_ecc_bypass    : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_TCM_ECC_BP;

/* Define the union U_SYSCTRL_TCM_ECC_CLR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_tcm_ecc_err_clr
        /*
         * tcm ecc err上报清除指令，写0写1完成1次清除。
         */
        unsigned int    cfg_tcm_ecc_err_clr   : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_TCM_ECC_CLR;

/* Define the union U_SYSCTRL_INIT_RPT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: rpt_l2m_init_over
        /*
         * L2M初始化完成上报（bit3对应data_bank1；
         * bit2对应data_bank0；
         * bit1对应ecc_bank1；
         * bit0对应ecc_bank0）：
         * 0：未完成；
         * 1：已完成。
         */
        unsigned int    rpt_l2m_init_over     : 4   ; /* [3..0] */
        // Comment of field: rpt_tcm_init_over
        /*
         * TCM初始化完成上报：
         * 0：未完成；
         * 1：已完成。
         */
        unsigned int    rpt_tcm_init_over     : 1   ; /* [4] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 27  ; /* [31..5] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_INIT_RPT;

/* Define the union U_SYSCTRL_CACHE_MEM_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_icache_sp_ram_tmod
        /*
         * icache ram tmod信号
         */
        unsigned int    cfg_icache_sp_ram_tmod : 5   ; /* [4..0] */
        // Comment of field: cfg_icache_mem_power_mode
        /*
         * icache ram power mode信号
         */
        unsigned int    cfg_icache_mem_power_mode : 1   ; /* [5] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 2   ; /* [7..6] */
        // Comment of field: cfg_dcache_sp_ram_tmod
        /*
         * dcache ram tmod信号
         */
        unsigned int    cfg_dcache_sp_ram_tmod : 5   ; /* [12..8] */
        // Comment of field: cfg_dcache_mem_power_mode
        /*
         * dcache ram power mode信号
         */
        unsigned int    cfg_dcache_mem_power_mode : 1   ; /* [13] */
        // Comment of field: reserved_1
        /*
         * 保留。
         */
        unsigned int    reserved_1            : 18  ; /* [31..14] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_CACHE_MEM_CTRL;

/* Define the union U_SYSCTRL_L2M_MEM_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_l2m_sp_ram_tmod
        /*
         * L2M sram  tmod信号
         */
        unsigned int    cfg_l2m_sp_ram_tmod   : 5   ; /* [4..0] */
        // Comment of field: cfg_l2m_mem_power_mode
        /*
         * L2M sram  power mode信号
         */
        unsigned int    cfg_l2m_mem_power_mode : 1   ; /* [5] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 2   ; /* [7..6] */
        // Comment of field: cfg_l2m_ecc_sp_ram_tmod
        /*
         * L2M ECC ram tmod信号
         */
        unsigned int    cfg_l2m_ecc_sp_ram_tmod : 5   ; /* [12..8] */
        // Comment of field: cfg_l2m_ecc_mem_power_mode
        /*
         * L2M ECC ram  power mode信号
         */
        unsigned int    cfg_l2m_ecc_mem_power_mode : 1   ; /* [13] */
        // Comment of field: reserved_1
        /*
         * 保留。
         */
        unsigned int    reserved_1            : 18  ; /* [31..14] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_L2M_MEM_CTRL;

/* Define the union U_SYSCTRL_TCM_MEM_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: cfg_itcm_sp_ram_tmod
        /*
         * itcm ram tmod信号
         */
        unsigned int    cfg_itcm_sp_ram_tmod  : 5   ; /* [4..0] */
        // Comment of field: cfg_itcm_mem_power_mode
        /*
         * itcm ram power mode信号
         */
        unsigned int    cfg_itcm_mem_power_mode : 1   ; /* [5] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 2   ; /* [7..6] */
        // Comment of field: cfg_d0tcm_sp_ram_tmod
        /*
         * d0tcm ram tmod信号
         */
        unsigned int    cfg_d0tcm_sp_ram_tmod : 5   ; /* [12..8] */
        // Comment of field: cfg_d0tcm_mem_power_mode
        /*
         * d0tcm ram power mode信号
         */
        unsigned int    cfg_d0tcm_mem_power_mode : 1   ; /* [13] */
        // Comment of field: reserved_1
        /*
         * 保留。
         */
        unsigned int    reserved_1            : 2   ; /* [15..14] */
        // Comment of field: cfg_d1tcm_sp_ram_tmod
        /*
         * d1tcm ram tmod信号
         */
        unsigned int    cfg_d1tcm_sp_ram_tmod : 5   ; /* [20..16] */
        // Comment of field: cfg_d1tcm_mem_power_mode
        /*
         * d1tcm ram power mode信号
         */
        unsigned int    cfg_d1tcm_mem_power_mode : 1   ; /* [21] */
        // Comment of field: reserved_2
        /*
         * 保留。
         */
        unsigned int    reserved_2            : 10  ; /* [31..22] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_TCM_MEM_CTRL;

/* Define the union U_FUNC_MBIST_CLK_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2crg_func_mbist_clk_sel
        /*
         * FUNCTION MBIST时钟选择：
         * 0：100K时钟；
         * 1：500K时钟；
         */
        unsigned int    sys2crg_func_mbist_clk_sel : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_FUNC_MBIST_CLK_SEL;

/* Define the union U_SYSCTRL_FUNC_MBIST_RESETN */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sysctrl_func_mbist_reset
        /*
         * function mbsit复位信号：
         * 0：复位；
         * 1：不复位
         */
        unsigned int    sysctrl_func_mbist_reset : 1   ; /* [0] */
        // Comment of field: sysctrl_func_mbist_mode
        /*
         * function mbist的模式信号：
         */
        unsigned int    sysctrl_func_mbist_mode : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FUNC_MBIST_RESETN;

/* Define the union U_SYSCTRL_FUNC_MBIST_CFG */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sysctrl_func_mbist_enable
        /*
         * function mbist使能信号
         */
        unsigned int    sysctrl_func_mbist_enable : 1   ; /* [0] */
        // Comment of field: sysctrl_func_mbist_writeen
        /*
         * function mbist写使能信号
         */
        unsigned int    sysctrl_func_mbist_writeen : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FUNC_MBIST_CFG;

/* Define the union U_SYSCTRL_BOD_MASK */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: v33_bod_mask
        /*
         * 3.3V电源BOD中断屏蔽寄存器
         * 0：不屏蔽；
         * 1：屏蔽；
         */
        unsigned int    v33_bod_mask          : 1   ; /* [0] */
        // Comment of field: v33_ov_mask
        /*
         * 3.3V电源OV中断屏蔽寄存器
         * 0：不屏蔽；
         * 1：屏蔽；
         */
        unsigned int    v33_ov_mask           : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_BOD_MASK;

/* Define the union U_SYSCTRL_BOD_INT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: ad_v33_bod_rpt
        /*
         * 3.3V电源BOD
         * 0：V33>BOD设置值或中断被屏蔽。
         * 1：V33<BOD设置值；
         */
        unsigned int    ad_v33_bod_rpt        : 1   ; /* [0] */
        // Comment of field: ad_v33_ov_rpt
        /*
         * 3.3V电源过压检测
         * 0：V33<3.8V或中断被屏蔽；
         * 1：V33>3.8V
         */
        unsigned int    ad_v33_ov_rpt         : 1   ; /* [1] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 30  ; /* [31..2] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_BOD_INT;

/* Define the union U_SYSCTRL_M7_NMI_CFG */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2m7_nmi
        /*
         * 不可屏蔽中断：
         * 0：无中断；
         * 1：产生中断；
         */
        unsigned int    sys2m7_nmi            : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_M7_NMI_CFG;

/* Define the union U_SYS_M7_CFGSTCALIB */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2m7_cfgstcalib
        /*
         * bit25：Indicates that no alternative reference clock source has been integrated. Tie HIGH if 
         * STCLKEN has been tied off.
         * Bit24:Tie this LOW if the system timer clock, the external reference clock, or fclk as indicated by 
         * CFGSTCALIB[25], can guarantee an exact multiple of 10ms. Otherwise, tie this signal HIGH.
         * bi23~bit0:Provides an integer value to compute a 10ms, 100Hz, delay from either the reference clock, or fclk if the reference clock is not implemented.
         */
        unsigned int    sys2m7_cfgstcalib     : 26  ; /* [25..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 6   ; /* [31..26] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYS_M7_CFGSTCALIB;

/* Define the union U_SYS_M7_PPBLOCK */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: arm_ctlppblock
        /*
         * When HIGH, disables PPB writes to ITCMCR, DTCMCR, 
         * AHBPCR or VTOR registers. Bit mappings are:
         * [0]  ITCMCR.
         * [1]  DTCMCR.
         * [2]  AHBPCR.
         * [3]  VTOR.
         */
        unsigned int    arm_ctlppblock        : 4   ; /* [3..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 28  ; /* [31..4] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYS_M7_PPBLOCK;

/* Define the union U_SYSCTRL_M7_ICE_RPT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: m72sys_icdet
        /*
         * Instuction cache error detection signaling.
         */
        unsigned int    m72sys_icdet          : 4   ; /* [3..0] */
        // Comment of field: m72sys_icerr
        /*
         * Instruction cache error bank signaling.
         */
        unsigned int    m72sys_icerr          : 22  ; /* [25..4] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 6   ; /* [31..26] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_M7_ICE_RPT;

/* Define the union U_SYSCTRL_M7_DCE_RPT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: m72sys_dcdet
        /*
         * Data cache error detection signaling.
         */
        unsigned int    m72sys_dcdet          : 4   ; /* [3..0] */
        // Comment of field: m72sys_dcerr
        /*
         * Data cache error bank signaling.
         */
        unsigned int    m72sys_dcerr          : 22  ; /* [25..4] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 6   ; /* [31..26] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_M7_DCE_RPT;

/* Define the union U_SYSCTRL_FPU_STA_RPT */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: m72sys_fpixc
        /*
         * Masked floating-point inexact exception
         */
        unsigned int    m72sys_fpixc          : 1   ; /* [0] */
        // Comment of field: m72sys_fpidc
        /*
         * Masked floating-point input denormal exception
         */
        unsigned int    m72sys_fpidc          : 1   ; /* [1] */
        // Comment of field: m72sys_fpofc
        /*
         * Masked floating-point overflow exception
         */
        unsigned int    m72sys_fpofc          : 1   ; /* [2] */
        // Comment of field: m72sys_fpufc
        /*
         * Masked floating-point underflow exception
         */
        unsigned int    m72sys_fpufc          : 1   ; /* [3] */
        // Comment of field: m72sys_fpdzc
        /*
         * Masked floating-point divide-by-zero exception
         */
        unsigned int    m72sys_fpdzc          : 1   ; /* [4] */
        // Comment of field: m72sys_fpioc
        /*
         * Invalid operation
         */
        unsigned int    m72sys_fpioc          : 1   ; /* [5] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 26  ; /* [31..6] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_FPU_STA_RPT;

/* Define the union U_SYS_M7_IADDR */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: m72sys_iaddr
        /*
         * Instruction fetch address, 64-bit aligned.
         */
        unsigned int    m72sys_iaddr          : 29  ; /* [28..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 2   ; /* [30..29] */
        // Comment of field: arm_sysresetreq
        /*
         * M7复位请求。
         */
        unsigned int    arm_sysresetreq       : 1   ; /* [31] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYS_M7_IADDR;

/* Define the union U_SYSCTR_I2C_STA */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: pmbus0_if_busy
        /*
         * PMBUS master忙闲指示信号，高电平有效
         */
        unsigned int    pmbus0_if_busy        : 1   ; /* [0] */
        // Comment of field: pmbus0_tx_fifo_status
        /*
         * PMBUS 发送fifo非空指示信号，高电平有效
         */
        unsigned int    pmbus0_tx_fifo_status : 1   ; /* [1] */
        // Comment of field: pmbus0_rx_fifo_status
        /*
         * PMBUS 接收fifo非空指示信号，高电平有效
         */
        unsigned int    pmbus0_rx_fifo_status : 1   ; /* [2] */
        // Comment of field: pmbus0_cmd_finish
        /*
         * PMBUS 命令完成指示脉冲信号(i2c_clk时钟域)
         */
        unsigned int    pmbus0_cmd_finish     : 1   ; /* [3] */
        // Comment of field: pmbus0_cmd_finish_err
        /*
         * PMBUS 异常结束指示信号，高电平有效
         */
        unsigned int    pmbus0_cmd_finish_err : 1   ; /* [4] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 3   ; /* [7..5] */
        // Comment of field: pmbus1_if_busy
        /*
         * PMBUS master忙闲指示信号，高电平有效
         */
        unsigned int    pmbus1_if_busy        : 1   ; /* [8] */
        // Comment of field: pmbus1_tx_fifo_status
        /*
         * PMBUS 发送fifo非空指示信号，高电平有效
         */
        unsigned int    pmbus1_tx_fifo_status : 1   ; /* [9] */
        // Comment of field: pmbus1_rx_fifo_status
        /*
         * PMBUS 接收fifo非空指示信号，高电平有效
         */
        unsigned int    pmbus1_rx_fifo_status : 1   ; /* [10] */
        // Comment of field: pmbus1_cmd_finish
        /*
         * PMBUS 命令完成指示脉冲信号(i2c_clk时钟域)
         */
        unsigned int    pmbus1_cmd_finish     : 1   ; /* [11] */
        // Comment of field: pmbus1_cmd_finish_err
        /*
         * PMBUS 异常结束指示信号，高电平有效
         */
        unsigned int    pmbus1_cmd_finish_err : 1   ; /* [12] */
        // Comment of field: reserved_1
        /*
         * 保留。
         */
        unsigned int    reserved_1            : 19  ; /* [31..13] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTR_I2C_STA;

/* Define the union U_SYS_M7_SLEEP_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys2m7_wicreqen
        /*
         * M7 WIC使能寄存器。
         * 0：不使能；
         * 1：使能。
         */
        unsigned int    sys2m7_wicreqen       : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYS_M7_SLEEP_CTRL;

/* Define the union U_SYS_M7_WICSENSE0 */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: m72sys_wicsense0_2
        /*
         * Active HIGH signal. These indicate which input events cause the WIC to generate the WAKEUP signal. 
         * WICSENSE[2]  EDBGRQ.
         * WICSENSE[1]  NMI.
         * WICSENSE[0]  RXEV.
         */
        unsigned int    m72sys_wicsense0_2    : 3   ; /* [2..0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 1   ; /* [3] */
        // Comment of field: m72sys_wicacken
        /*
         * m7返回的WIC使能有效状态：
         * 0：无效；
         * 1：有效。
         */
        unsigned int    m72sys_wicacken       : 1   ; /* [4] */
        // Comment of field: reserved_1
        /*
         * 保留。
         */
        unsigned int    reserved_1            : 27  ; /* [31..5] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYS_M7_WICSENSE0;

/* Define the union U_SYSCTRL_NOR_RSV_REG */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys_nor_rsv0_reg
        /*
         * 系统控制器NOR空间保留寄存器，复位值0。
         */
        unsigned int    sys_nor_rsv0_reg      : 16  ; /* [15..0] */
        // Comment of field: sys_nor_rsv1_reg
        /*
         * 系统控制器NOR空间保留寄存器，复位值1。
         */
        unsigned int    sys_nor_rsv1_reg      : 16  ; /* [31..16] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_NOR_RSV_REG;

/* Define the union U_SYSCTRL_NOR_ICG_BYPASS */
typedef union {
    /* Define the struct bits */
    struct {
        // Comment of field: sys_nor_para_icg_bypass
        /*
         * para 动态ICG开关
         * 0：时钟关断；
         * 1：时钟不关断。
         */
        unsigned int    sys_nor_para_icg_bypass : 1   ; /* [0] */
        // Comment of field: reserved_0
        /*
         * 保留。
         */
        unsigned int    reserved_0            : 31  ; /* [31..1] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;
} U_SYSCTRL_NOR_ICG_BYPASS;

//==============================================================================
/* Define the global struct */
typedef struct {
    volatile unsigned int           reserved_44[3]                   ; /* 0x0~0x8 */
    volatile unsigned int           SYS_VERSION                      ; /* 0xc */
    volatile unsigned int           reserved_45[60]                  ; /* 0x10~0xfc */
    volatile U_SYSCTRL_CLK_SWITCH   SYSCTRL_CLK_SWITCH               ; /* 0x100 */
    volatile U_SYSCTRL_REF_CLK_SEL   SYSCTRL_REF_CLK_SEL              ; /* 0x104 */
    volatile U_SYSCTRL_CLK_DETECT_EN   SYSCTRL_CLK_DETECT_EN            ; /* 0x108 */
    volatile U_SYSCTRL_CPU_FREQ_SEL   SYSCTRL_CPU_FREQ_SEL             ; /* 0x10c */
    volatile U_SYSCTRL_PERI_FREQ_SEL   SYSCTRL_PERI_FREQ_SEL            ; /* 0x110 */
    volatile U_SYSCTRL_LOOP_FREQ_SEL   SYSCTRL_LOOP_FREQ_SEL            ; /* 0x114 */
    volatile U_SYSCTRL_SOC_PERI_CLK_EN   SYSCTRL_SOC_PERI_CLK_EN          ; /* 0x118 */
    volatile unsigned int           SYSCTRL_HAC_CLK_EN               ; /* 0x11c */
    volatile U_SYSCTRL_ANA_CLK_EN   SYSCTRL_ANA_CLK_EN               ; /* 0x120 */
    volatile U_SYSCTRL_GLB_SRST_EN   SYSCTRL_GLB_SRST_EN              ; /* 0x124 */
    volatile U_SYSCTRL_SOC_SRST_EN   SYSCTRL_SOC_SRST_EN              ; /* 0x128 */
    volatile unsigned int           SYSCTRL_HAC_SRST_EN              ; /* 0x12c */
    volatile unsigned int           reserved_46[52]                  ; /* 0x130~0x1fc */
    volatile U_SYSCTRL_FLASH_CHKSUM_SADDR   SYSCTRL_FLASH_CHKSUM_SADDR       ; /* 0x200 */
    volatile U_SYSCTRL_FLASH_CHKSUM_EADDR   SYSCTRL_FLASH_CHKSUM_EADDR       ; /* 0x204 */
    volatile U_SYSCTRL_FLASH_CHKSUM_START   SYSCTRL_FLASH_CHKSUM_START       ; /* 0x208 */
    volatile unsigned int           FLASH_CHKSUM__RESULT             ; /* 0x20c */
    volatile U_FLASH_CHKSUM__STATUS   FLASH_CHKSUM__STATUS             ; /* 0x210 */
    volatile U_SYSCTRL_FLASH_RWEN   SYSCTRL_FLASH_RWEN               ; /* 0x214 */
    volatile unsigned int           reserved_47                      ; /* 0x218 */
    volatile U_SYSCTRL_P_DFLASH_TIMING   SYSCTRL_P_DFLASH_TIMING          ; /* 0x21c */
    volatile U_SYSCTRL_PFLASH_TIMING0   SYSCTRL_PFLASH_TIMING0           ; /* 0x220 */
    volatile U_SYSCTRL_PFLASH_TIMING1   SYSCTRL_PFLASH_TIMING1           ; /* 0x224 */
    volatile U_SYSCTRL_PFLASH_TIMING2   SYSCTRL_PFLASH_TIMING2           ; /* 0x228 */
    volatile U_SYSCTRL_PFLASH_TIMING3   SYSCTRL_PFLASH_TIMING3           ; /* 0x22c */
    volatile U_SYSCTRL_PFLASH_TIMING4   SYSCTRL_PFLASH_TIMING4           ; /* 0x230 */
    volatile U_SYSCTRL_DFLASH_TIMING0   SYSCTRL_DFLASH_TIMING0           ; /* 0x234 */
    volatile U_SYSCTRL_DFLASH_TIMING1   SYSCTRL_DFLASH_TIMING1           ; /* 0x238 */
    volatile U_SYSCTRL_DFLASH_TIMING2   SYSCTRL_DFLASH_TIMING2           ; /* 0x23c */
    volatile U_SYSCTRL_DFLASH_TIMING3   SYSCTRL_DFLASH_TIMING3           ; /* 0x240 */
    volatile U_SYSCTRL_DFLASH_TIMING4   SYSCTRL_DFLASH_TIMING4           ; /* 0x244 */
    volatile U_SYSCTRL_TIMING_LOAD   SYSCTRL_TIMING_LOAD              ; /* 0x248 */
    volatile U_SYSCTRL_TIMING_LOAD_OVER   SYSCTRL_TIMING_LOAD_OVER         ; /* 0x24c */
    volatile unsigned int           reserved_48[4]                   ; /* 0x250~0x25c */
    volatile U_DFLASH_ECC_EN_CFG    DFLASH_ECC_EN_CFG                ; /* 0x260 */
    volatile U_SYSCTRL_FLASH_STATE_RPT   SYSCTRL_FLASH_STATE_RPT          ; /* 0x264 */
    volatile U_SYSCTRL_DFLASH_ECC_ERR_RPT   SYSCTRL_DFLASH_ECC_ERR_RPT       ; /* 0x268 */
    volatile U_SYSCTRL_DFLASH_ECC_ERR_ADDR   SYSCTRL_DFLASH_ECC_ERR_ADDR      ; /* 0x26c */
    volatile U_SYSCTRL_FLASH_PROT_ALERT   SYSCTRL_FLASH_PROT_ALERT         ; /* 0x270 */
    volatile U_SYSCTRL_PROT_STATUS   SYSCTRL_PROT_STATUS              ; /* 0x274 */
    volatile unsigned int           SYSCTRL_FLASH_PROT_CFG_RPT       ; /* 0x278 */
    volatile U_SYSCTRL_PFLASH_PROT_ADDR_BEGIN   SYSCTRL_PFLASH_PROT_ADDR_BEGIN   ; /* 0x27c */
    volatile U_SYSCTRL_PFLASH_PROT_ADDR_END   SYSCTRL_PFLASH_PROT_ADDR_END     ; /* 0x280 */
    volatile unsigned int           PFLASH_NVR_CFG_WR_EN             ; /* 0x284 */
    volatile unsigned int           DFLASH_NVR_CFG_WR_EN             ; /* 0x288 */
    volatile U_FLASH_BIST_IO_SEL    FLASH_BIST_IO_SEL                ; /* 0x28c */
    volatile unsigned int           reserved_49[28]                  ; /* 0x290~0x2fc */
    volatile U_SYSCTRL_TCM_ECC_BP   SYSCTRL_TCM_ECC_BP               ; /* 0x300 */
    volatile U_SYSCTRL_TCM_ECC_CLR   SYSCTRL_TCM_ECC_CLR              ; /* 0x304 */
    volatile U_SYSCTRL_INIT_RPT     SYSCTRL_INIT_RPT                 ; /* 0x308 */
    volatile U_SYSCTRL_CACHE_MEM_CTRL   SYSCTRL_CACHE_MEM_CTRL           ; /* 0x30c */
    volatile U_SYSCTRL_L2M_MEM_CTRL   SYSCTRL_L2M_MEM_CTRL             ; /* 0x310 */
    volatile U_SYSCTRL_TCM_MEM_CTRL   SYSCTRL_TCM_MEM_CTRL             ; /* 0x314 */
    volatile unsigned int           reserved_50[58]                  ; /* 0x318~0x3fc */
    volatile U_FUNC_MBIST_CLK_SEL   FUNC_MBIST_CLK_SEL               ; /* 0x400 */
    volatile U_SYSCTRL_FUNC_MBIST_RESETN   SYSCTRL_FUNC_MBIST_RESETN        ; /* 0x404 */
    volatile U_SYSCTRL_FUNC_MBIST_CFG   SYSCTRL_FUNC_MBIST_CFG           ; /* 0x408 */
    volatile unsigned int           SYSCTRL_FUNC_MBIST_DATAIN        ; /* 0x40c */
    volatile unsigned int           SYSCTRL_FUNC_MBIST_DATAOUT       ; /* 0x410 */
    volatile unsigned int           reserved_51[59]                  ; /* 0x414~0x4fc */
    volatile U_SYSCTRL_BOD_MASK     SYSCTRL_BOD_MASK                 ; /* 0x500 */
    volatile U_SYSCTRL_BOD_INT      SYSCTRL_BOD_INT                  ; /* 0x504 */
    volatile U_SYSCTRL_M7_NMI_CFG   SYSCTRL_M7_NMI_CFG               ; /* 0x508 */
    volatile unsigned int           reserved_52                      ; /* 0x50c */
    volatile U_SYS_M7_CFGSTCALIB    SYS_M7_CFGSTCALIB                ; /* 0x510 */
    volatile U_SYS_M7_PPBLOCK       SYS_M7_PPBLOCK                   ; /* 0x514 */
    volatile U_SYSCTRL_M7_ICE_RPT   SYSCTRL_M7_ICE_RPT               ; /* 0x518 */
    volatile U_SYSCTRL_M7_DCE_RPT   SYSCTRL_M7_DCE_RPT               ; /* 0x51c */
    volatile U_SYSCTRL_FPU_STA_RPT   SYSCTRL_FPU_STA_RPT              ; /* 0x520 */
    volatile U_SYS_M7_IADDR         SYS_M7_IADDR                     ; /* 0x524 */
    volatile U_SYSCTR_I2C_STA       SYSCTR_I2C_STA                   ; /* 0x528 */
    volatile unsigned int           reserved_53[53]                  ; /* 0x52c~0x5fc */
    volatile U_SYS_M7_SLEEP_CTRL    SYS_M7_SLEEP_CTRL                ; /* 0x600 */
    volatile U_SYS_M7_WICSENSE0     SYS_M7_WICSENSE0                 ; /* 0x604 */
    volatile unsigned int           SYS_M7_WICSENSE1                 ; /* 0x608 */
    volatile unsigned int           SYS_M7_WICSENSE2                 ; /* 0x60c */
    volatile unsigned int           reserved_54[122]                 ; /* 0x610~0x7f4 */
    volatile U_SYSCTRL_NOR_RSV_REG   SYSCTRL_NOR_RSV_REG              ; /* 0x7f8 */
    volatile U_SYSCTRL_NOR_ICG_BYPASS   SYSCTRL_NOR_ICG_BYPASS           ; /* 0x7fc */
} S_SYSCTRL_NOR_REGS_TYPE;

#define S_SYSCTRL_NOR_BASE       0x40000000
#define reg_sysCtrlNor     (*(volatile S_SYSCTRL_NOR_REGS_TYPE *)S_SYSCTRL_NOR_BASE)
#define FLASH_TIMERG0_TPROG   (0x465 << 12)    // 9us * SOC时钟125Mhz
#define FLASH_TIMERG0_TNVS    0x271          // 5us * SOC时钟125Mhz
#define FLASH_TIMERG1_TRCV    (0x2EE << 12)    // 6us * SOC时钟125Mhz
#define FLASH_TIMERG1_TPGS    0x465          // 9us * SOC时钟125Mhz
#define FLASH_TIMERG2_RCV     (0x668A << 16)   // 210us * SOC时钟125Mhz
#define FLASH_TIMERG2_TRCV    0x1D4C         // 60us * SOC时钟125Mhz
#define FLASH_TIMERG3_TERASE  0x112A88       // 9ms * SOC时钟125Mhz
#define FLASH_TIMERG4_TERASE  0x112A88       // 9ms * SOC时钟125Mhz

#endif