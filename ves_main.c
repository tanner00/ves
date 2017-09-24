#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ARRLEN(a) (sizeof((a)) / sizeof((*(a))))

extern void put32(uint32_t, uint32_t);
extern uint32_t get32(uint32_t);

#define MAILBOX_BASE      0x2000B880
#define MAILBOX_READ      0x00
#define MAILBOX_STATUS    0x18
#define MAILBOX_WRITE     0x20
#define MAILBOX_EMPTY     0b1000000000000000000000000000000
#define MAILBOX_FULL      0b10000000000000000000000000000000

void write_mailbox(uint8_t channel, uint32_t data) {
	while ((get32(MAILBOX_BASE + MAILBOX_STATUS) & MAILBOX_FULL) != 0);
	put32(MAILBOX_BASE + MAILBOX_WRITE, data + channel);
}

uint32_t read_mailbox(uint8_t channel) {
	uint32_t r = 0;
	while (true) {
		while ((get32(MAILBOX_BASE + MAILBOX_STATUS) & MAILBOX_EMPTY) != 0);

		uint32_t r = get32(MAILBOX_BASE + MAILBOX_READ);
		if ((r & 0xF) == channel) {
			break;
		}
	}
	return r;
}

uint32_t rom[] = { 0x6a, 0x02, 0x6b, 0x0c, 0x6c, 0x3f, 
		   0x6d, 0x0c, 0xa2, 0xea, 0xda, 0xb6, 0xdc, 0xd6, 0x6e, 
		   0x00, 0x22, 0xd4, 0x66, 0x03, 0x68, 0x02, 0x60, 0x60, 
		   0xf0, 0x15, 0xf0, 0x07, 0x30, 0x00, 0x12, 0x1a, 0xc7, 
		   0x17, 0x77, 0x08, 0x69, 0xff, 0xa2, 0xf0, 0xd6, 0x71, 
		   0xa2, 0xea, 0xda, 0xb6, 0xdc, 0xd6, 0x60, 0x01, 0xe0, 
		   0xa1, 0x7b, 0xfe, 0x60, 0x04, 0xe0, 0xa1, 0x7b, 0x02, 
		   0x60, 0x1f, 0x8b, 0x02, 0xda, 0xb6, 0x60, 0x0c, 0xe0, 
		   0xa1, 0x7d, 0xfe, 0x60, 0x0d, 0xe0, 0xa1, 0x7d, 0x02, 
		   0x60, 0x1f, 0x8d, 0x02, 0xdc, 0xd6, 0xa2, 0xf0, 0xd6, 
		   0x71, 0x86, 0x84, 0x87, 0x94, 0x60, 0x3f, 0x86, 0x02, 
		   0x61, 0x1f, 0x87, 0x12, 0x46, 0x02, 0x12, 0x78, 0x46, 
		   0x3f, 0x12, 0x82, 0x47, 0x1f, 0x69, 0xff, 0x47, 0x00, 
		   0x69, 0x01, 0xd6, 0x71, 0x12, 0x2a, 0x68, 0x02, 0x63, 
		   0x01, 0x80, 0x70, 0x80, 0xb5, 0x12, 0x8a, 0x68, 0xfe, 
		   0x63, 0x0a, 0x80, 0x70, 0x80, 0xd5, 0x3f, 0x01, 0x12, 
		   0xa2, 0x61, 0x02, 0x80, 0x15, 0x3f, 0x01, 0x12, 0xba, 
		   0x80, 0x15, 0x3f, 0x01, 0x12, 0xc8, 0x80, 0x15, 0x3f, 
		   0x01, 0x12, 0xc2, 0x60, 0x20, 0xf0, 0x18, 0x22, 0xd4, 
		   0x8e, 0x34, 0x22, 0xd4, 0x66, 0x3e, 0x33, 0x01, 0x66, 
		   0x03, 0x68, 0xfe, 0x33, 0x01, 0x68, 0x02, 0x12, 0x16, 
		   0x79, 0xff, 0x49, 0xfe, 0x69, 0xff, 0x12, 0xc8, 0x79, 
		   0x01, 0x49, 0x02, 0x69, 0x01, 0x60, 0x04, 0xf0, 0x18, 
		   0x76, 0x01, 0x46, 0x40, 0x76, 0xfe, 0x12, 0x6c, 0xa2, 
		   0xf2, 0xfe, 0x33, 0xf2, 0x65, 0xf1, 0x29, 0x64, 0x14, 
		   0x65, 0x00, 0xd4, 0x55, 0x74, 0x15, 0xf2, 0x29, 0xd4, 
		   0x55, 0x00, 0xee, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 
		   0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

#define GPU_BASE 0x40040000
#define GPIO_LEV0 0x20200034
#define GPIO_4 0x10

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int32_t ves_main(void) {

	const uint32_t properties[] = {
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		0,   32,
		0,   0,
		0,   0,
		0,   0,
	};

	for (size_t i = 0; i < ARRLEN(properties); ++i) {
		put32(GPU_BASE + (i * 4), properties[i]);
	}

	write_mailbox(1, GPU_BASE);
	read_mailbox(1);
 
	uint32_t *fb = (uint32_t *) get32(GPU_BASE + 0x20);

	uint8_t  gp_registers[15];
	uint8_t  flags_register;
	uint8_t  timer_registers[2];

	uint16_t pc = 0;

	uint16_t stack[16];
	uint8_t  sp = 0;

	uint8_t memory[0xFFF];
	
	while (true) {		
		uint16_t instruction = rom[pc] << 8 | rom[pc + 1];

		if (instruction == 0xe0) {
			for (size_t i = 0; i < properties[0] * properties[1]; ++i) {
				fb[i] = 0;
			}
		} else if (instruction == 0xee) {
			// ret;
		}
		
		pc += 2;
	}

	return 0;
}

/*
  uint32_t button_lev = get32(GPIO_LEV0);
  if (button_lev & GPIO_4) {
  for (size_t i = 0; i < properties[0] * properties[1]; ++i) {
  fb[i] = 0xFF0000;
  }
  } else {
  for (size_t i = 0; i < properties[0] * properties[1]; ++i) {
  fb[i] = 0xFFFFFF;
  }
  }
 */
