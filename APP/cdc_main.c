/*********************************************************************
 * File Name          : cdc_main.c
 * Description        : CH592 USB CDC ACM interactive debug demo.
 *                      Demonstrates printf() and scanf() over native USB CDC,
 *                      system information inspection, and PB22 (BOOT) monitoring.
 *********************************************************************/

#include "CH59x_common.h"
#include "CH59x_sys.h"
#include "CH59x_gpio.h"
#include "ISP592.h"
#include "usb_cdc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BOOT_PIN  GPIO_Pin_22

/* Print banner once USB enumeration completes */
static void print_banner(void)
{
    uint8_t mac[6] = {0};
    GetMACAddress(mac);

    printf("\r\n");
    printf("====================================================\r\n");
    printf("     CH592 USB CDC ACM Debug Console (Nano Board)   \r\n");
    printf("====================================================\r\n");
    printf(" Chip      : WCH CH592 (QingKe V4 RISC-V)\r\n");
    printf(" SysClock  : %lu MHz\r\n", (unsigned long)(GetSysClock() / 1000000));
    printf(" MAC Addr  : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    printf(" Baud Rate : %lu bps (virtual COM)\r\n", (unsigned long)usb_cdc_get_baud_rate());
    printf(" BOOT Pin  : PB22 (Active-Low)\r\n");
    printf(" StdIO     : Native printf() & scanf() over USB CDC\r\n");
    printf("====================================================\r\n");
    printf("Type 'help' for available commands.\r\n\r\n");
}

/* Print help menu */
static void cmd_help(void)
{
    printf("Available commands:\r\n");
    printf("  help           - Show this help message\r\n");
    printf("  info           - Show hardware and clock information\r\n");
    printf("  scanf          - Test stdio scanf() input (integers & strings)\r\n");
    printf("  boot           - Read current state of PB22 (BOOT button)\r\n");
    printf("  bench          - Benchmark USB CDC transmission speed\r\n");
    printf("  echo <text>    - Echo input back to console\r\n");
    printf("  reboot         - Software reset MCU\r\n");
    printf("  isp            - Erase sector 0 & enter ISP bootloader\r\n");
}

/* Show hardware info */
static void cmd_info(void)
{
    uint8_t mac[6] = {0};
    GetMACAddress(mac);

    printf("--- Hardware Info ---\r\n");
    printf("CPU Clock       : %lu Hz\r\n", (unsigned long)GetSysClock());
    printf("MAC Address     : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    printf("USB Configured  : %s\r\n", usb_cdc_is_configured() ? "Yes" : "No");
    printf("Terminal DTR    : %s\r\n", usb_cdc_is_connected() ? "Connected (DTR=1)" : "Idle (DTR=0)");
    printf("CDC Baud Rate   : %lu bps\r\n", (unsigned long)usb_cdc_get_baud_rate());
    printf("BOOT Pin (PB22) : %s\r\n", (GPIOB_ReadPortPin(BOOT_PIN) == 0) ? "PRESSED (0)" : "RELEASED (1)");
}

/* Test scanf capabilities */
static void cmd_scanf(void)
{
    int num = 0;
    char name[32] = {0};

    printf("\r\n--- stdio scanf() Test ---\r\n");
    printf("Enter an integer: ");
    if (scanf("%d", &num) == 1) {
        printf(">> Parsed integer: %d (0x%X)\r\n", num, (unsigned int)num);
    } else {
        printf(">> Failed to parse integer!\r\n");
    }

    printf("Enter a word/name: ");
    if (scanf("%31s", name) == 1) {
        printf(">> Parsed string : '%s' (len=%d)\r\n", name, (int)strlen(name));
    } else {
        printf(">> Failed to parse string!\r\n");
    }
    printf("--- scanf() Test Complete ---\r\n\r\n");
}

/* Benchmark USB CDC TX throughput */
static void cmd_bench(void)
{
    printf("Benchmarking 100 lines transmission...\r\n");
    for (int i = 1; i <= 100; i++) {
        printf("[%03d] USB CDC speed test line: 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n", i);
    }
    usb_cdc_flush();
    printf("Benchmark complete.\r\n");
}

int main(void)
{
    /* Use PLL 60MHz system clock */
    SetSysClock(CLK_SOURCE_PLL_60MHz);

    /* Configure PB22 (BOOT button) as input with pull-up */
    GPIOB_ModeCfg(BOOT_PIN, GPIO_ModeIN_PU);

    /* Initialize USB CDC ACM subsystem */
    usb_cdc_init();

    /* Disable stdio stream buffering so printf/scanf flush immediately */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Wait for host USB enumeration */
    while (!usb_cdc_is_configured()) {
        mDelaymS(10);
    }

    /* Small delay for terminal program (e.g. minicom/picocom) to open port */
    mDelaymS(200);

    print_banner();

    char line[64];
    uint8_t prev_btn_state = 1;
    uint32_t btn_counter = 0;

    while (1)
    {
        /* Prompt user for command */
        printf("ch592> ");

        /* Read command line interactively (with echo and backspace) */
        int len = usb_cdc_readline(line, sizeof(line));
        if (len < 0) {
            mDelaymS(50);
            continue;
        }

        /* Check for empty input */
        if (len == 0) {
            continue;
        }

        /* Command dispatch */
        if (strcmp(line, "help") == 0) {
            cmd_help();
        }
        else if (strcmp(line, "info") == 0) {
            cmd_info();
        }
        else if (strcmp(line, "scanf") == 0) {
            cmd_scanf();
        }
        else if (strcmp(line, "boot") == 0) {
            uint8_t btn = GPIOB_ReadPortPin(BOOT_PIN);
            printf("BOOT Button (PB22): %s (raw pin=%d, presses=%lu)\r\n",
                   btn ? "RELEASED" : "PRESSED", btn, (unsigned long)btn_counter);
        }
        else if (strcmp(line, "bench") == 0) {
            cmd_bench();
        }
        else if (strncmp(line, "echo ", 5) == 0) {
            printf("%s\r\n", line + 5);
        }
        else if (strcmp(line, "echo") == 0) {
            printf("\r\n");
        }
        else if (strcmp(line, "reboot") == 0) {
            printf("Rebooting MCU...\r\n");
            usb_cdc_flush();
            mDelaymS(50);
            SYS_ResetExecute();
        }
        else if (strcmp(line, "isp") == 0 || strcmp(line, "bootloader") == 0) {
            printf("Erasing sector 0 and rebooting into ISP bootloader...\r\n");
            usb_cdc_flush();
            mDelaymS(50);
            enter_isp_bootloader();
        }
        else {
            printf("Unknown command: '%s'. Type 'help' for commands.\r\n", line);
        }

        /* Poll button state */
        uint8_t curr_btn = (GPIOB_ReadPortPin(BOOT_PIN) != 0);
        if (prev_btn_state == 1 && curr_btn == 0) {
            btn_counter++;
            printf("\r\n[EVENT] BOOT button pressed! (Count: %lu)\r\n", (unsigned long)btn_counter);
        }
        prev_btn_state = curr_btn;
    }
}
