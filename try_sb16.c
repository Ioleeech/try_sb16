#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Use it to avoid the warnings about unused variables */
#define UNUSED(x) x = x

/* delay() in TurboC works with milliseconds */
#define mdelay(ms) delay(ms)

#define SB16_ADDR_MIXER_REG(BASE)  (BASE + 0x04)
#define SB16_ADDR_MIXER_DATA(BASE) (BASE + 0x05)
#define SB16_ADDR_DSP_RESET(BASE)  (BASE + 0x06)
#define SB16_ADDR_DSP_READ(BASE)   (BASE + 0x0A)
#define SB16_ADDR_DSP_WRITE(BASE)  (BASE + 0x0C)
#define SB16_ADDR_DSP_INT8(BASE)   (BASE + 0x0E)
#define SB16_ADDR_DSP_INT16(BASE)  (BASE + 0x0F)

#define SB16_MIXER_REG_IRQ_SELECT 0x80
#define SB16_MIXER_REG_DMA_SELECT 0x81

#define SB16_STATE_READY 0x80

#define BITMASK_IRQ2  0x01
#define BITMASK_IRQ5  0x02
#define BITMASK_IRQ7  0x04
#define BITMASK_IRQ10 0x08

#define BITMASK_DMA0  0x01
#define BITMASK_DMA1  0x02
#define BITMASK_DMA2  0x04
#define BITMASK_DMA3  0x08
#define BITMASK_DMA4  0x10
#define BITMASK_DMA5  0x20
#define BITMASK_DMA6  0x40
#define BITMASK_DMA7  0x80

static int base_addr_list [] = { 0x220, 0x240, 0x260, 0x280 };
/*
static int dsp_write(int base_addr, int value)
{
  int i;
  for (i = 0; i < 100; i ++)
  {
    int busy = inportb(SB16_ADDR_DSP_WRITE(base_addr)) & SB16_STATE_READY;
    if (busy)
    {
      mdelay(10);
      continue;
    }

    outportb(SB16_ADDR_DSP_WRITE(base_addr), value);
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
*/
static int dsp_read(int base_addr, int* p_value)
{
  int i;
  for (i = 0; i < 100; i ++)
  {
    int ready = inportb(SB16_ADDR_DSP_INT8(base_addr)) & SB16_STATE_READY;
    if (! ready)
    {
      mdelay(10);
      continue;
    }

    if (p_value)
      *p_value = inportb(SB16_ADDR_DSP_READ(base_addr));

    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

/****************************************************************************
* DSP reset:
* 1. Write 0x01 to reset port
* 2. Wait 3 us
* 3. Write 0x00 to reset port
* 4. Wait about 100 us
* 5. Read byte from DSP
* 6. DSP must return 0xAA
****************************************************************************/
static int dsp_reset(int base_addr)
{
  int value = 0x00;
  printf("Trying SB16 at 0x%X...\n", base_addr);

  outportb(SB16_ADDR_DSP_RESET(base_addr), 0x01);
  mdelay(10);
  outportb(SB16_ADDR_DSP_RESET(base_addr), 0x00);
  mdelay(10);

  if ((dsp_read(base_addr, &value) != EXIT_SUCCESS)
  ||  (value != 0xAA))
  {
    printf("SB16 is not found\n");
    return EXIT_FAILURE;
  }

  printf("SB16 is ready\n");
  return EXIT_SUCCESS;
}

static void mixer_read(int base_addr, int reg, int* p_value)
{
  int value;

  outportb(SB16_ADDR_MIXER_REG(base_addr), reg);
  mdelay(10);
  value = inportb(SB16_ADDR_MIXER_DATA(base_addr));

  if (p_value)
    *p_value = value;
}

static void mixer_write(int base_addr, int reg, int value)
{
  outportb(SB16_ADDR_MIXER_REG(base_addr), reg);
  mdelay(10);
  outportb(SB16_ADDR_MIXER_DATA(base_addr), value);
}

static int print_irq(int base_addr)
{
  int irq_mask = 0x00;
  mixer_read(base_addr, SB16_MIXER_REG_IRQ_SELECT, &irq_mask);

  if (irq_mask & BITMASK_IRQ2)
  {
    printf("IRQ : 2\n");
    return EXIT_SUCCESS;
  }

  if (irq_mask & BITMASK_IRQ5)
  {
    printf("IRQ : 5\n");
    return EXIT_SUCCESS;
  }

  if (irq_mask & BITMASK_IRQ7)
  {
    printf("IRQ : 7\n");
    return EXIT_SUCCESS;
  }

  if (irq_mask & BITMASK_IRQ10)
  {
    printf("IRQ : 10\n");
    return EXIT_SUCCESS;
  }

  printf("Wrong IRQ mask: 0x%02X\n", irq_mask);
  return EXIT_FAILURE;
}

static int set_irq(int base_addr, int irq_num)
{
  int irq_mask = 0x00;

  switch (irq_num)
  {
    case 2:
      irq_mask |= BITMASK_IRQ2;
      break;

    case 5:
      irq_mask |= BITMASK_IRQ5;
      break;

    case 7:
      irq_mask |= BITMASK_IRQ7;
      break;

    case 10:
      irq_mask |= BITMASK_IRQ10;
      break;

    default:
      printf("Wrong IRQ num: %d\n", irq_num);
      return EXIT_FAILURE;
  }

  printf("IRQ : %d\n", irq_num);
  mixer_write(base_addr, SB16_MIXER_REG_IRQ_SELECT, irq_mask);

  return EXIT_SUCCESS;
}

static int print_dma(int base_addr)
{
  int found_8  = 0x00;
  int found_16 = 0x00;
  int dma_mask = 0x00;
  mixer_read(base_addr, SB16_MIXER_REG_DMA_SELECT, &dma_mask);

  if ((! found_8) && (dma_mask & BITMASK_DMA0))
  {
    printf("DMA : 0\n");
    found_8 = 0x01;
  }

  if ((! found_8) && (dma_mask & BITMASK_DMA1))
  {
    printf("DMA : 1\n");
    found_8 = 0x01;
  }

  if ((! found_8) && (dma_mask & BITMASK_DMA2))
  {
    printf("DMA : 2\n");
    found_8 = 0x01;
  }

  if ((! found_8) && (dma_mask & BITMASK_DMA3))
  {
    printf("DMA : 3\n");
    found_8 = 0x01;
  }

  if ((! found_16) && (dma_mask & BITMASK_DMA4))
  {
    printf("HDMA : 4\n");
    found_16 = 0x01;
  }

  if ((! found_16) && (dma_mask & BITMASK_DMA5))
  {
    printf("HDMA : 5\n");
    found_16 = 0x01;
  }

  if ((! found_16) && (dma_mask & BITMASK_DMA6))
  {
    printf("HDMA : 6\n");
    found_16 = 0x01;
  }

  if ((! found_16) && (dma_mask & BITMASK_DMA7))
  {
    printf("HDMA : 7\n");
    found_16 = 0x01;
  }

  if ((! found_8) && (! found_16))
  {
    printf("Wrong DMA mask: 0x%02X\n", dma_mask);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static int set_dma(int base_addr, int dma8_num, int dma16_num)
{
  int dma_mask = 0x00;

  switch (dma8_num)
  {
    case 0:
      dma_mask |= BITMASK_DMA0;
      break;

    case 1:
      dma_mask |= BITMASK_DMA1;
      break;

    case 2:
      dma_mask |= BITMASK_DMA2;
      break;

    case 3:
      dma_mask |= BITMASK_DMA3;
      break;

    default:
      printf("Wrong DMA num: %d\n", dma8_num);
      return EXIT_FAILURE;
  }

  switch (dma16_num)
  {
    case 0:
      /* Do nothing, no 16-bit DMA */
      break;

    case 4:
      dma_mask |= BITMASK_DMA4;
      break;

    case 5:
      dma_mask |= BITMASK_DMA5;
      break;

    case 6:
      dma_mask |= BITMASK_DMA6;
      break;

    case 7:
      dma_mask |= BITMASK_DMA7;
      break;

    default:
      printf("Wrong HDMA num: %d\n", dma16_num);
      return EXIT_FAILURE;
  }

  printf("DMA : %d\n", dma8_num);
  printf("HDMA : %d\n", dma16_num);
  mixer_write(base_addr, SB16_MIXER_REG_DMA_SELECT, dma_mask);

  return EXIT_SUCCESS;
}

static int check_base_addr(int base_addr)
{
  int i, i_max = sizeof(base_addr_list) / sizeof(base_addr_list[0]);

  for (i = 0; i < i_max; i ++)
  {
    if (base_addr == base_addr_list[i])
    {
      printf("Base addr : 0x%X\n", base_addr);
      return EXIT_SUCCESS;
    }
  }

  printf("Wrong base address: 0x%X\n", base_addr);
  return EXIT_FAILURE;
}

int main(int argc, char* argv[], char* env[])
{
  int ret_value = EXIT_SUCCESS;
  int base_addr = 0x00;
  int irq_num   = 0x00;
  int dma8_num  = 0x00;
  int dma16_num = 0x00;

  /* TODO: Get parameters from BLASTER env. variable */
  UNUSED(env);

  if (argc < 4)
  {
    char* app_name = strrchr(argv[0], '\\');
    if (app_name)
      app_name ++;
    else
      app_name = argv[0];

    printf("Some parameters are required:\n");
    printf("%s <BASE_ADDR> <IRQ_NUM> <DMA_NUM> [HDMA_NUM]\n", app_name);
    printf("Examples:\n");
    printf("%s 220 7 1\n", app_name);
    printf("%s 220 7 1 5\n", app_name);

    return EXIT_FAILURE;
  }

  /* Required parameters */
  if (sscanf(argv[1], "%x", &base_addr) != 1)
    ret_value = EXIT_FAILURE;

  if (sscanf(argv[2], "%d", &irq_num) != 1)
    ret_value = EXIT_FAILURE;

  if (sscanf(argv[3], "%d", &dma8_num) != 1)
    ret_value = EXIT_FAILURE;

  /* Optional parameters */
  if ((argc > 4) && (sscanf(argv[4], "%d", &dma16_num) != 1))
    ret_value = EXIT_FAILURE;

  /* Reset */
  if (ret_value == EXIT_SUCCESS)
    ret_value = check_base_addr(base_addr);

  if (ret_value == EXIT_SUCCESS)
    ret_value = dsp_reset(base_addr);

  /* Set parameters */
  if (ret_value == EXIT_SUCCESS)
    printf("Configuring SB16...\n");

  if (ret_value == EXIT_SUCCESS)
    ret_value = set_irq(base_addr, irq_num);

  if (ret_value == EXIT_SUCCESS)
    ret_value = set_dma(base_addr, dma8_num, dma16_num);

  /* Get parameters */
  if (ret_value == EXIT_SUCCESS)
    printf("Checking SB16 settings...\n");

  if (ret_value == EXIT_SUCCESS)
    ret_value = print_irq(base_addr);

  if (ret_value == EXIT_SUCCESS)
    ret_value = print_dma(base_addr);

  /* Done */
  return ret_value;
}
