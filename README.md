# ch592_usbcdc — USB CDC ACM library for WCH CH592

Self-contained USB CDC ACM (Virtual COM Port) device stack + stdio
retargeting (`printf`/`scanf`) for the CH592 (QingKe V4 RISC-V).
No BLE stack, no TMOS, no app code — two files, drop into any project.

Designed for minimalist boards (e.g. [biemster/ch592-nano](https://oshwlab.com/biemster/ch592-nano))
that expose only USB D+/D- and BOOT, no UART pins.

## Files

- `USB/usb_cdc.c` / `USB/include/usb_cdc.h` — the whole library.

## Requirements (provided by your project, not vendored here)

- WCH `CH59x` StdPeriphDriver headers (`CH59x_common.h`, `CH59x_sys.h`,
  `CH59x_gpio.h`, `ISP592.h`) and `CH59x_sys.c` compiled in
  (provides `mDelaymS()` used during init).
- RISC-V GCC + `nano.specs`/`nosys.specs` link (stdio retargeting hooks
  `_write`/`_read` inside the library).
- 60 MHz-ish sysclk recommended (USB full-speed needs a stable clock).

## Integration (5 steps)

1. Compile `USB/usb_cdc.c` into your firmware.
2. Add `-I<path-to>/USB/include` to your includes.
3. Call `usb_cdc_init()` once at boot (it forces a disconnect/reconnect
   cycle so the host re-enumerates after flashing).
4. Optionally wait for the host: `while (!usb_cdc_is_configured());`
5. Use `printf()`/`scanf()` directly, or the `usb_cdc_*` API below.
   Disable stdio buffering for interactive use:
   `setvbuf(stdout, NULL, _IONBF, 0); setvbuf(stdin, NULL, _IONBF, 0);`

Example `Makefile` fragment (this repo's own `Makefile` is the reference):

```make
SRCS_C = APP/cdc_main.c USB/usb_cdc.c ...
INCLUDES = -IUSB/include ...
```

The stock pattern rule `$(BUILD_DIR)/%.o: %.c` handles the `USB/`
subdirectory with no special cases.

## API

| Function | Purpose |
|---|---|
| `usb_cdc_init()` | init HW, endpoints, ring buffers; re-enumerate |
| `usb_cdc_is_configured()` | host enumerated us |
| `usb_cdc_is_connected()` | terminal open (DTR asserted) |
| `usb_cdc_get_baud_rate()` | host's `SET_LINE_CODING` baud (informational) |
| `usb_cdc_write/read/available()` | bulk transfer + RX level |
| `usb_cdc_getchar_nowait/timeout/putchar()` | char I/O |
| `usb_cdc_flush()` | block until TX drained (with timeout, never hangs) |
| `usb_cdc_readline()` | interactive line input (echo + backspace) |
| `usb_cdc_set_echo/line_mode()` | input modes for `_read`/`scanf` |
| `usb_cdc_enter_bootloader()` | programmatically jump to WCH USB ISP bootloader |
| `usb_cdc_enable_1200bps_bootloader()` | toggle automatic 1200-baud touch (default: on) |

`printf()` never blocks forever: TX has a 50 ms timeout and the host is
NAKed when the RX ring is full, resuming automatically on read.

## Buttonless Bootloader Entry (1200-Baud Touch & Programmatic)

No physical button or pin bridging required after initial flashing:

- **1200-baud touch (Arduino style)**: Opening the CDC port at 1200 baud
  signals the firmware to erase sector 0, write `R16_INT32K_TUNE = 0xFFFF`,
  and perform a software reset into the WCH ROM ISP bootloader (`4348:55e0`).
- **Programmatic API**: Call `usb_cdc_enter_bootloader()` from any command
  line, button handler, or telemetry packet.
- **Automated `./flash.sh`**: Automatically detects the active CDC port,
  reboots into ISP mode buttonlessly, and flashes via `wchisp`.

## Tuning

Buffer sizes and timeouts are `#define`s at the top of `USB/usb_cdc.c`
(`CDC_TX_BUF_SIZE` 512 / `CDC_RX_BUF_SIZE` 256, powers of two).
USB VID:PID is `1A86:8040`; serial string is derived from the chip's
unique MAC (`CH592-AABBCCDDEEFF`).

## Host side

Linux `/dev/ttyACM*`, macOS `/dev/cu.usbmodem*`, Windows `usbser.sys`
— no custom drivers. 115200 8N1 works; baud is virtual (ignored by HW).

---

## Reference example: interactive debug console (`APP/cdc_main.c`)

The other half of this repo. A thin shell demonstrating the library:
banner with chip info/MAC/baud, `help`, `info`, `scanf` test, `boot`
(PB22 state + press counter), `bench` (100-line throughput), `echo`,
`reboot` — plus an async `[EVENT]` line whenever the BOOT button is
pressed while the console is open.

### Build & flash

```bash
make          # -> build/ch592_usb_cdc.bin
./flash.sh    # bridge BOOT(PB22) to GND while plugging in USB
```

### Open the console

```bash
picocom -b 115200 /dev/ttyACM0   # or tio / minicom
```

Baud is virtual (ignored by hardware); DTR open/close is visible to
firmware via `usb_cdc_is_connected()`.
