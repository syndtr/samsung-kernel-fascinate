/*
 * arch/arm/mach-s5pv210/atlas.h
 */

#ifndef __ATLAS_H__
#define __ATLAS_H__

#define S5PV210_LCD_WIDTH 480
#define S5PV210_LCD_HEIGHT 800

struct uart_port;

void atlas_fimc_init(void);
void atlas_panel_init(void);
void aries_bt_uart_wake_peer(struct uart_port *port);
extern void s3c_setup_uart_cfg_gpio(unsigned char port);

extern struct platform_device s3c_device_spi_gpio;

#ifdef __LINUX_MFD_MAX8998_H
extern struct max8998_charger_data atlas_charger;
extern struct max8998_charger_callbacks *max8998_callbacks;
extern enum cable_type_t set_cable_status;
#endif
void max8998_init(void);

#ifdef __LINUX_MTD_ONENAND_H
extern struct onenand_platform_data onenand_pdata;
void s5pc110_onenand_set_platdata(struct onenand_platform_data *pdata);
#endif

extern struct class *sec_class;
void fsa9480_init(void);
void atlas_switch_init(void);

extern struct platform_device sec_device_wifi;
void atlas_wifi_init(void);

#endif
