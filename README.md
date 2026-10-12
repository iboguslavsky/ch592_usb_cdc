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

Example `Makefile` fragment (see `../ch592_usb_cdc` for a full example):

```make
LIBUSBCDC_DIR = ../ch592_usbcdc
INCLUDES += -I$(LIBUSBCDC_DIR)/USB/include

$(BUILD_DIR)/usb_cdc.o: $(LIBUSBCDC_DIR)/USB/usb_cdc.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
```

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

`printf()` never blocks forever: TX has a 50 ms timeout and the host is
NAKed when the RX ring is full, resuming automatically on read.

## Tuning

Buffer sizes and timeouts are `#define`s at the top of `USB/usb_cdc.c`
(`CDC_TX_BUF_SIZE` 512 / `CDC_RX_BUF_SIZE` 256, powers of two).
USB VID:PID is `1A86:8040`; serial string is derived from the chip's
unique MAC (`CH592-AABBCCDDEEFF`).

## Host side

Linux `/dev/ttyACM*`, macOS `/dev/cu.usbmodem*`, Windows `usbser.sys`
— no custom drivers. 115200 8N1 works; baud is virtual (ignored by HW).
