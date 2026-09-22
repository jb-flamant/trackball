/*
 * Driver Zephyr natif pour le capteur optique PixArt PMW3610.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Capteur SPI 3 fils (SDIO bidirectionnelle) piloté par interruption MOTION.
 * Le déplacement est publié dans le sous-système input (INPUT_REL_X/Y).
 *
 * Le mapping de registres et la séquence d'initialisation (power-up, self-test,
 * configuration CPI, lecture burst 12 bits) proviennent de la fiche technique
 * PMW3610 ; cette implémentation est écrite pour Zephyr pur, sans dépendance à
 * ZMK ni à l'API sensor.
 */

#define DT_DRV_COMPAT pixart_pmw3610

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmw3610, CONFIG_INPUT_LOG_LEVEL);

/* --- Registres --- */
#define PMW3610_REG_PRODUCT_ID    0x00
#define PMW3610_REG_MOTION        0x02
#define PMW3610_REG_MOTION_BURST  0x12
#define PMW3610_REG_OBSERVATION   0x2D
#define PMW3610_REG_POWER_UP_RESET 0x3A
#define PMW3610_REG_SPI_CLK_ON_REQ 0x41
#define PMW3610_REG_RES_STEP      0x85
#define PMW3610_REG_SPI_PAGE      0x7F

#define PMW3610_PRODUCT_ID        0x3E
#define PMW3610_POWERUP_CMD_RESET 0x5A
#define PMW3610_SPI_CLOCK_ENABLE  0xBA
#define PMW3610_SPI_CLOCK_DISABLE 0xB5

#define PMW3610_SPI_WRITE_BIT     BIT(7)
#define PMW3610_CLOCK_ON_DELAY_US 300

/* Lecture burst : 7 octets à partir de MOTION_BURST. Positions des deltas. */
#define PMW3610_BURST_SIZE        7
#define PMW3610_X_L_POS           1
#define PMW3610_Y_L_POS           2
#define PMW3610_XY_H_POS          3

#define PMW3610_MIN_CPI           200
#define PMW3610_MAX_CPI           3200

/* Séquence d'initialisation non bloquante (le MCU n'est pas mobilisé pendant
 * les temporisations imposées par le capteur). */
enum pmw3610_init_step {
	PMW3610_INIT_POWER_UP,   /* reset d'alimentation */
	PMW3610_INIT_CLEAR_OB1,  /* efface le registre d'observation (self-test) */
	PMW3610_INIT_CHECK_OB1,  /* vérifie self-test + identifiant produit */
	PMW3610_INIT_CONFIGURE,  /* efface les registres de mouvement, règle le CPI */
	PMW3610_INIT_COUNT,
};

/* Temporisations (ms) entre étapes, marges issues des retours terrain. */
static const int32_t pmw3610_init_delay_ms[PMW3610_INIT_COUNT] = {
	[PMW3610_INIT_POWER_UP]  = 10,
	[PMW3610_INIT_CLEAR_OB1] = 200,
	[PMW3610_INIT_CHECK_OB1] = 50,
	[PMW3610_INIT_CONFIGURE] = 0,
};

struct pmw3610_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec irq_gpio;
	uint16_t cpi;
	bool swap_xy;
	bool invert_x;
	bool invert_y;
};

struct pmw3610_data {
	const struct device *dev;
	struct gpio_callback irq_cb;
	struct k_work motion_work;
	struct k_work_delayable init_work;
	int init_step;
	bool ready;
};

/* --- Accès SPI bas niveau --- */

/* Lecture : émet l'adresse (MSB=0), ignore l'octet reçu pendant l'adresse,
 * puis lit `len` octets sur la ligne SDIO. */
static int pmw3610_read(const struct device *dev, uint8_t addr, uint8_t *value, uint8_t len)
{
	const struct pmw3610_config *cfg = dev->config;
	const struct spi_buf tx_buf = { .buf = &addr, .len = 1 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	struct spi_buf rx_buf[] = {
		{ .buf = NULL, .len = 1 },
		{ .buf = value, .len = len },
	};
	const struct spi_buf_set rx = { .buffers = rx_buf, .count = ARRAY_SIZE(rx_buf) };

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int pmw3610_read_reg(const struct device *dev, uint8_t addr, uint8_t *value)
{
	return pmw3610_read(dev, addr, value, 1);
}

/* Écriture brute d'un registre (adresse avec MSB=1). */
static int pmw3610_write_reg(const struct device *dev, uint8_t addr, uint8_t value)
{
	const struct pmw3610_config *cfg = dev->config;
	uint8_t buf[] = { addr | PMW3610_SPI_WRITE_BIT, value };
	const struct spi_buf tx_buf = { .buf = buf, .len = sizeof(buf) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	return spi_write_dt(&cfg->spi, &tx);
}

/* Écriture « propre » : active l'horloge SPI du capteur, écrit, puis la coupe. */
static int pmw3610_write(const struct device *dev, uint8_t reg, uint8_t value)
{
	int err;

	err = pmw3610_write_reg(dev, PMW3610_REG_SPI_CLK_ON_REQ, PMW3610_SPI_CLOCK_ENABLE);
	if (err) {
		return err;
	}
	k_sleep(K_USEC(PMW3610_CLOCK_ON_DELAY_US));

	err = pmw3610_write_reg(dev, reg, value);
	if (err) {
		return err;
	}

	return pmw3610_write_reg(dev, PMW3610_REG_SPI_CLK_ON_REQ, PMW3610_SPI_CLOCK_DISABLE);
}

/* --- Configuration --- */

static int pmw3610_set_cpi(const struct device *dev, uint32_t cpi,
			   bool swap_xy, bool invert_x, bool invert_y)
{
	const uint8_t addr[] = { PMW3610_REG_SPI_PAGE, PMW3610_REG_RES_STEP, PMW3610_REG_SPI_PAGE };
	uint8_t value = 0;
	int err;

	if (cpi < PMW3610_MIN_CPI || cpi > PMW3610_MAX_CPI) {
		LOG_ERR("CPI %u hors plage [%u, %u]", cpi, PMW3610_MIN_CPI, PMW3610_MAX_CPI);
		return -EINVAL;
	}

	/* bits 4-0 : pas de résolution (cpi / 200) ; bits 7/6/5 : swap/inv X/inv Y */
	value = (cpi / 200) & 0x1F;
	if (swap_xy) {
		value |= BIT(7);
	}
	if (invert_x) {
		value |= BIT(6);
	}
	if (invert_y) {
		value |= BIT(5);
	}

	err = pmw3610_write_reg(dev, PMW3610_REG_SPI_CLK_ON_REQ, PMW3610_SPI_CLOCK_ENABLE);
	if (err) {
		return err;
	}
	k_sleep(K_USEC(PMW3610_CLOCK_ON_DELAY_US));

	const uint8_t data[] = { 0xFF, value, 0x00 };

	for (size_t i = 0; i < ARRAY_SIZE(addr); i++) {
		err = pmw3610_write_reg(dev, addr[i], data[i]);
		if (err) {
			LOG_ERR("Écriture CPI échouée (%d)", err);
			break;
		}
	}

	(void)pmw3610_write_reg(dev, PMW3610_REG_SPI_CLK_ON_REQ, PMW3610_SPI_CLOCK_DISABLE);
	return err;
}

/* --- Étapes d'initialisation asynchrone --- */

static int pmw3610_init_power_up(const struct device *dev)
{
	return pmw3610_write_reg(dev, PMW3610_REG_POWER_UP_RESET, PMW3610_POWERUP_CMD_RESET);
}

static int pmw3610_init_clear_ob1(const struct device *dev)
{
	return pmw3610_write(dev, PMW3610_REG_OBSERVATION, 0x00);
}

static int pmw3610_init_check_ob1(const struct device *dev)
{
	uint8_t value;
	int err;

	err = pmw3610_read_reg(dev, PMW3610_REG_OBSERVATION, &value);
	if (err) {
		return err;
	}
	if ((value & 0x0F) != 0x0F) {
		LOG_ERR("Échec du self-test (observation 0x%02x)", value);
		return -EIO;
	}

	err = pmw3610_read_reg(dev, PMW3610_REG_PRODUCT_ID, &value);
	if (err) {
		return err;
	}
	if (value != PMW3610_PRODUCT_ID) {
		LOG_ERR("Identifiant produit incorrect 0x%02x (attendu 0x%02x)",
			value, PMW3610_PRODUCT_ID);
		return -EIO;
	}

	return 0;
}

static int pmw3610_init_configure(const struct device *dev)
{
	const struct pmw3610_config *cfg = dev->config;
	uint8_t dummy;
	int err = 0;

	/* Effacer les registres de mouvement (requis par la fiche technique). */
	for (uint8_t reg = 0x02; reg <= 0x05 && !err; reg++) {
		err = pmw3610_read_reg(dev, reg, &dummy);
	}
	if (err) {
		return err;
	}

	return pmw3610_set_cpi(dev, cfg->cpi, cfg->swap_xy, cfg->invert_x, cfg->invert_y);
}

static int (*const pmw3610_init_fn[PMW3610_INIT_COUNT])(const struct device *dev) = {
	[PMW3610_INIT_POWER_UP]  = pmw3610_init_power_up,
	[PMW3610_INIT_CLEAR_OB1] = pmw3610_init_clear_ob1,
	[PMW3610_INIT_CHECK_OB1] = pmw3610_init_check_ob1,
	[PMW3610_INIT_CONFIGURE] = pmw3610_init_configure,
};

static int pmw3610_set_interrupt(const struct device *dev, bool enable)
{
	const struct pmw3610_config *cfg = dev->config;

	return gpio_pin_interrupt_configure_dt(
		&cfg->irq_gpio, enable ? GPIO_INT_LEVEL_ACTIVE : GPIO_INT_DISABLE);
}

static void pmw3610_init_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct pmw3610_data *data = CONTAINER_OF(dwork, struct pmw3610_data, init_work);
	const struct device *dev = data->dev;
	int err;

	err = pmw3610_init_fn[data->init_step](dev);
	if (err) {
		LOG_ERR("Init PMW3610 : échec à l'étape %d (%d)", data->init_step, err);
		return;
	}

	data->init_step++;
	if (data->init_step == PMW3610_INIT_COUNT) {
		data->ready = true;
		LOG_INF("PMW3610 initialisé");
		(void)pmw3610_set_interrupt(dev, true);
	} else {
		k_work_schedule(&data->init_work,
				K_MSEC(pmw3610_init_delay_ms[data->init_step]));
	}
}

/* --- Lecture du mouvement --- */

/* Convertit un entier signé sur `bits` bits (complément à deux). */
#define PMW3610_TOINT16(val, bits) (((struct { int16_t v : (bits); }){ .v = (val) }).v)

static void pmw3610_report(const struct device *dev)
{
	uint8_t buf[PMW3610_BURST_SIZE];
	int16_t x, y;
	int err;

	err = pmw3610_read(dev, PMW3610_REG_MOTION_BURST, buf, sizeof(buf));
	if (err) {
		LOG_ERR("Lecture burst échouée (%d)", err);
		return;
	}

	/* Deltas sur 12 bits : octet bas + demi-octet haut partagé (X:haut, Y:bas). */
	x = PMW3610_TOINT16(buf[PMW3610_X_L_POS] | ((buf[PMW3610_XY_H_POS] & 0xF0) << 4), 12);
	y = PMW3610_TOINT16(buf[PMW3610_Y_L_POS] | ((buf[PMW3610_XY_H_POS] & 0x0F) << 8), 12);

	if (x != 0) {
		input_report_rel(dev, INPUT_REL_X, x, (y == 0), K_FOREVER);
	}
	if (y != 0) {
		input_report_rel(dev, INPUT_REL_Y, y, true, K_FOREVER);
	}
}

static void pmw3610_motion_work_handler(struct k_work *work)
{
	struct pmw3610_data *data = CONTAINER_OF(work, struct pmw3610_data, motion_work);
	const struct device *dev = data->dev;

	if (data->ready) {
		pmw3610_report(dev);
	}
	(void)pmw3610_set_interrupt(dev, true);
}

static void pmw3610_gpio_callback(const struct device *port, struct gpio_callback *cb,
				  uint32_t pins)
{
	struct pmw3610_data *data = CONTAINER_OF(cb, struct pmw3610_data, irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	/* Masque l'interruption le temps de traiter (ré-armée dans le work). */
	(void)pmw3610_set_interrupt(data->dev, false);
	k_work_submit(&data->motion_work);
}

static int pmw3610_init(const struct device *dev)
{
	const struct pmw3610_config *cfg = dev->config;
	struct pmw3610_data *data = dev->data;
	int err;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("Bus SPI non prêt");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
		LOG_ERR("GPIO d'interruption non prêt");
		return -ENODEV;
	}

	data->dev = dev;
	data->init_step = 0;
	data->ready = false;

	k_work_init(&data->motion_work, pmw3610_motion_work_handler);
	k_work_init_delayable(&data->init_work, pmw3610_init_work_handler);

	err = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("Configuration GPIO d'interruption échouée (%d)", err);
		return err;
	}

	gpio_init_callback(&data->irq_cb, pmw3610_gpio_callback, BIT(cfg->irq_gpio.pin));
	err = gpio_add_callback(cfg->irq_gpio.port, &data->irq_cb);
	if (err) {
		LOG_ERR("Ajout du callback GPIO échoué (%d)", err);
		return err;
	}

	/* Initialisation matérielle poursuivie en tâche de fond. */
	k_work_schedule(&data->init_work, K_MSEC(pmw3610_init_delay_ms[0]));

	return 0;
}

/* SPI mode 3 (CPOL=1, CPHA=1), MSB en tête, mots de 8 bits. */
#define PMW3610_SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | \
			  SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

#define PMW3610_DEFINE(inst)                                                        \
	static struct pmw3610_data pmw3610_data_##inst;                             \
	static const struct pmw3610_config pmw3610_config_##inst = {                \
		.spi = SPI_DT_SPEC_INST_GET(inst, PMW3610_SPI_MODE, 0),             \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                \
		.cpi = DT_INST_PROP(inst, cpi),                                    \
		.swap_xy = DT_INST_PROP(inst, swap_xy),                            \
		.invert_x = DT_INST_PROP(inst, invert_x),                          \
		.invert_y = DT_INST_PROP(inst, invert_y),                          \
	};                                                                         \
	DEVICE_DT_INST_DEFINE(inst, pmw3610_init, NULL,                            \
			      &pmw3610_data_##inst, &pmw3610_config_##inst,        \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PMW3610_DEFINE)
