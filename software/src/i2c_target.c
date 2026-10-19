/*
 * Sortie I2C target du trackball.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Le RP2040 s'enregistre comme peripherique I2C (mode target, driver DesignWare
 * `i2c_dw` via CONFIG_I2C_TARGET) sur le bus i2c0 (GP28=SDA, GP29=SCL). Un
 * controleur hote lit le pointage via une petite carte de registres a
 * auto-incrementation :
 *
 *   0x00  WHO_AM_I  constante d'identification (0x3C)
 *   0x01  BUTTONS   bit0 gauche, bit1 droit, bit2 milieu (niveau)
 *   0x02  DX        delta X (int8), read-to-clear
 *   0x03  DY        delta Y (int8), read-to-clear
 *
 * Protocole : l'hote ecrit 1 octet (index de registre de depart) puis lit N
 * octets (l'index s'auto-incremente). Les deltas DX/DY sont bornes a int8 et
 * decrementes de ce qui est renvoye (le reliquat est conserve pour la lecture
 * suivante : aucune perte si |delta| depasse 127).
 *
 * Les callbacks target s'executent en contexte ISR ; l'accumulation (feed) se
 * fait dans le thread input et protege sa lecture-modification par irq_lock().
 */

#include "i2c_target.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_i2c, LOG_LEVEL_INF);

#define I2C_TARGET_NODE DT_NODELABEL(i2c0)

/* Adresse 7 bits du target (a adapter au besoin cote hote). */
#define I2C_TARGET_ADDRESS 0x54

/* Carte de registres */
#define REG_WHO_AM_I 0x00
#define REG_BUTTONS  0x01
#define REG_DX       0x02
#define REG_DY       0x03
#define WHO_AM_I_VALUE 0x3C

/* Etat partagé thread input <-> ISR I2C. */
static int32_t acc_dx;
static int32_t acc_dy;
static uint8_t cur_buttons;

/* Index de registre courant de la transaction I2C en cours. */
static uint8_t reg_ptr;

/* Prend jusqu'a +/-127 counts de l'accumulateur et renvoie la valeur int8
 * correspondante (le reliquat reste pour la prochaine lecture). Contexte ISR. */
static int8_t take_delta(int32_t *acc)
{
	int32_t v = *acc;

	if (v > 127) {
		v = 127;
	} else if (v < -127) {
		v = -127;
	}
	*acc -= v;
	return (int8_t)v;
}

/* Lit l'octet du registre et avance l'index (contexte ISR). */
static uint8_t reg_read(void)
{
	uint8_t val;

	switch (reg_ptr) {
	case REG_WHO_AM_I:
		val = WHO_AM_I_VALUE;
		break;
	case REG_BUTTONS:
		val = cur_buttons;
		break;
	case REG_DX:
		val = (uint8_t)take_delta(&acc_dx);
		break;
	case REG_DY:
		val = (uint8_t)take_delta(&acc_dy);
		break;
	default:
		val = 0x00;
		break;
	}

	reg_ptr++;
	return val;
}

static int target_write_requested(struct i2c_target_config *config)
{
	ARG_UNUSED(config);
	return 0;
}

/* Premier (et unique) octet ecrit par l'hote = index de registre de depart. */
static int target_write_received(struct i2c_target_config *config, uint8_t val)
{
	ARG_UNUSED(config);
	reg_ptr = val;
	return 0;
}

static int target_read_requested(struct i2c_target_config *config, uint8_t *val)
{
	ARG_UNUSED(config);
	*val = reg_read();
	return 0;
}

static int target_read_processed(struct i2c_target_config *config, uint8_t *val)
{
	ARG_UNUSED(config);
	*val = reg_read();
	return 0;
}

static int target_stop(struct i2c_target_config *config)
{
	ARG_UNUSED(config);
	return 0;
}

static struct i2c_target_callbacks target_cbs = {
	.write_requested = target_write_requested,
	.write_received = target_write_received,
	.read_requested = target_read_requested,
	.read_processed = target_read_processed,
	.stop = target_stop,
};

static struct i2c_target_config target_cfg = {
	.address = I2C_TARGET_ADDRESS,
	.callbacks = &target_cbs,
};

void app_i2c_target_feed(int dx, int dy, uint8_t buttons)
{
	unsigned int key = irq_lock();

	acc_dx += dx;
	acc_dy += dy;
	cur_buttons = buttons;

	irq_unlock(key);
}

int app_i2c_target_init(void)
{
	const struct device *const dev = DEVICE_DT_GET(I2C_TARGET_NODE);
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("Bus I2C0 non pret");
		return -ENODEV;
	}

	ret = i2c_target_register(dev, &target_cfg);
	if (ret != 0) {
		LOG_ERR("Enregistrement I2C target echoue (%d)", ret);
		return ret;
	}

	LOG_INF("Sortie I2C target active @0x%02x (GP28=SDA, GP29=SCL)",
		target_cfg.address);
	return 0;
}
