/**
  **************************************************************************
  * @file     main.c
  * @brief    main program
  **************************************************************************
  *                       Copyright notice & Disclaimer
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */

#include "at32f402_405_board.h"
#include "at32f402_405_clock.h"
#include "usb_conf.h"
#include "usbh_core.h"
#include "FreeRTOS.h"
#include "task.h"
/** @addtogroup AT32F405_periph_examples
  * @{
  */

/** @addtogroup 405_USB_device_winusb USB_device_winusb
  * @{
  */

/* usb global struct define */

#if defined ( __ICCARM__ ) /* iar compiler */
  #pragma data_alignment=4
#endif
void usb_clock48m_select(usb_clk48_s clk_s);
void usb_gpio_config(void);
void usb_low_power_wakeup_config(void);

void usb_dc_low_level_init(uint8_t busid)
{
    /* enable otg clock */
  crm_periph_clock_enable(CRM_OTGFS1_PERIPH_CLOCK, TRUE);
    /* enable otg irq */
  nvic_irq_enable(OTGFS1_IRQn, 0, 0);

}

void usb_dc_low_level_deinit(uint8_t busid)
{


}

void usb_hc_low_level_init(uint8_t busid)
{
        /* enable otg clock */
  crm_periph_clock_enable(CRM_OTGHS_PERIPH_CLOCK, TRUE);
    /* enable otg irq */
  nvic_irq_enable(OTGHS_IRQn, 0, 0);
}

void usb_hc_low_level_deinit(uint8_t busid)
{
}
/**
  * @brief  main function.
  * @param  none
  * @retval none
  */
int main(void)
{
  uint16_t data_len;

  uint32_t timeout;

  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

  system_clock_config();

  at32_board_init();

  /* usb gpio config */
  usb_gpio_config();

#ifdef USB_LOW_POWER_WAKUP
  usb_low_power_wakeup_config();
#endif


  /* select usb 48m clcok source */
  usb_clock48m_select(USB_CLK_HEXT);

  uart_print_init(115200);

  printf("cherryusb demo\r\n");

  usbh_initialize(0, OTGHS_BASE);

  msc_ram_init(0, OTGFS1_BASE);

  vTaskStartScheduler();

  while(1)
  {
  }
}

/**
  * @brief  usb 48M clock select
  * @param  clk_s:USB_CLK_HICK, USB_CLK_HEXT
  * @retval none
  */
void usb_clock48m_select(usb_clk48_s clk_s)
{
  if(clk_s == USB_CLK_HICK)
  {
    crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_HICK);

    /* enable the acc calibration ready interrupt */
    crm_periph_clock_enable(CRM_ACC_PERIPH_CLOCK, TRUE);

    /* update the c1\c2\c3 value */
    acc_write_c1(7980);
    acc_write_c2(8000);
    acc_write_c3(8020);

    /* open acc calibration */
    acc_calibration_mode_enable(ACC_CAL_HICKTRIM, TRUE);
  }
  else
  {
    crm_pllu_output_set(TRUE);
    /* wait till pll is ready */
    while(crm_flag_get(CRM_PLLU_STABLE_FLAG) != SET)
    {
    }
    crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_PLLU);
  }
}

/**
  * @brief  this function config gpio.
  * @param  none
  * @retval none
  */
void usb_gpio_config(void)
{
  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

  crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
#ifdef USB_SOF_OUTPUT_ENABLE
  crm_periph_clock_enable(OTG_PIN_SOF_GPIO_CLOCK, TRUE);
  gpio_init_struct.gpio_pins = OTG_PIN_SOF;
  gpio_init(OTG_PIN_SOF_GPIO, &gpio_init_struct);
  gpio_pin_mux_config(OTG_PIN_SOF_GPIO, OTG_PIN_SOF_SOURCE, OTG_PIN_MUX);
#endif

  /* use vbus pin */
#ifndef USB_VBUS_IGNORE
  gpio_init_struct.gpio_pins = OTG_PIN_VBUS;
  gpio_init_struct.gpio_pull = GPIO_PULL_DOWN;
  gpio_pin_mux_config(OTG_PIN_GPIO, OTG_PIN_VBUS_SOURCE, OTG_PIN_MUX);
  gpio_init(OTG_PIN_GPIO, &gpio_init_struct);
#endif


}
#ifdef USB_LOW_POWER_WAKUP
/**
  * @brief  usb low power wakeup interrupt config
  * @param  none
  * @retval none
  */
void usb_low_power_wakeup_config(void)
{
  exint_init_type exint_init_struct;

  crm_periph_clock_enable(CRM_SCFG_PERIPH_CLOCK, TRUE);
  exint_default_para_init(&exint_init_struct);

  exint_init_struct.line_enable = TRUE;
  exint_init_struct.line_mode = EXINT_LINE_INTERRUPT;
  exint_init_struct.line_select = OTG_WKUP_EXINT_LINE;
  exint_init_struct.line_polarity = EXINT_TRIGGER_RISING_EDGE;
  exint_init(&exint_init_struct);

  nvic_irq_enable(OTG_WKUP_IRQ, 0, 0);
}

/**
  * @brief  this function handles otg wakup interrupt.
  * @param  none
  * @retval none
  */
void OTG_WKUP_HANDLER(void)
{
  exint_flag_clear(OTG_WKUP_EXINT_LINE);
}

#endif

/**
  * @brief  this function handles otg interrupt.
  * @param  none
  * @retval none
  */
void OTGFS1_IRQHandler(void)
{
  void USBD_IRQHandler(uint8_t busid);
  USBD_IRQHandler(0);
}

void OTGHS_IRQHandler(void)
{
  void USBH_IRQHandler(uint8_t busid);
  USBH_IRQHandler(0);
}
/**
  * @brief  usb delay millisecond function.
  * @param  ms: number of millisecond delay
  * @retval none
  */
void usb_delay_ms(uint32_t ms)
{
  /* user can define self delay function */
  delay_ms(ms);
}

/**
  * @brief  usb delay microsecond function.
  * @param  us: number of microsecond delay
  * @retval none
  */
void usb_delay_us(uint32_t us)
{
  delay_us(us);
}

/**
  * @}
  */

/**
  * @}
  */
