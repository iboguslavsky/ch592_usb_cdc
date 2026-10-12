CROSS_COMPILE ?= /home/igor/esp32/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin/riscv-none-elf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

TARGET = ch592_usb_cdc
BUILD_DIR = build

# Reference example (APP/cdc_main.c) + USB CDC library (USB/usb_cdc.c).
# The library is plain in-tree source: no out-of-tree paths, no sub-makes.
SRCS_C = \
	APP/cdc_main.c \
	USB/usb_cdc.c \
	StdPeriphDriver/CH59x_sys.c \
	StdPeriphDriver/CH59x_gpio.c \
	StdPeriphDriver/CH59x_clk.c

SRCS_ASM = \
	Startup/startup_CH592.S

INCLUDES = \
	-IUSB/include \
	-IStdPeriphDriver/inc \
	-IRVMSIS

CFLAGS = -march=rv32imac_zicsr_zifencei -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore \
         -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common \
         -std=gnu99 -DDEBUG=Debug_USB -DINT_SOFT $(INCLUDES)

ASFLAGS = -march=rv32imac_zicsr_zifencei -mabi=ilp32 $(INCLUDES) -x assembler-with-cpp

LDFLAGS = -march=rv32imac_zicsr_zifencei -mabi=ilp32 -mcmodel=medany \
          -T Ld/Link.ld -nostartfiles -Wl,--gc-sections \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
          --specs=nano.specs --specs=nosys.specs \
          -LStdPeriphDriver -lISP592

OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS_C:.c=.o)) $(addprefix $(BUILD_DIR)/, $(SRCS_ASM:.S=.o))

all: $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@

flash: $(BUILD_DIR)/$(TARGET).bin
	./flash.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash
