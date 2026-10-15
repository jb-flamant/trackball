/*
 * Contexte USB device (nouveau stack Zephyr, device_next).
 *
 * Adapté du code de référence des samples Zephyr
 * (samples/subsys/usb/common/sample_usbd_init.c), simplifié pour un seul
 * périphérique pleine vitesse (le RP2040 n'expose que du Full-Speed) et sans
 * dépendance aux options Kconfig.sample_usbd.
 */

#include "usb.h"

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_usb, LOG_LEVEL_INF);

/*
 * VID/PID de TEST (pid.codes 0x1209 / 0x0001) : à REMPLACER par une paire
 * propre au projet avant toute distribution. Ne jamais réutiliser le VID des
 * samples Zephyr en dehors des samples.
 */
#define APP_USBD_VID       0x1209
#define APP_USBD_PID       0x0001
#define APP_USBD_MAX_POWER 250   /* 500 mA */

USBD_DEVICE_DEFINE(app_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   APP_USBD_VID, APP_USBD_PID);

USBD_DESC_LANG_DEFINE(app_lang);
USBD_DESC_MANUFACTURER_DEFINE(app_mfr, "Trackball DIY");
USBD_DESC_PRODUCT_DEFINE(app_product, "Trackball RP2040-Zero");
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(app_sn)));

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");

/* Bus-powered ; une seule configuration pleine vitesse. */
USBD_CONFIGURATION_DEFINE(app_fs_config, 0, APP_USBD_MAX_POWER, &fs_cfg_desc);

/*
 * Périphérique composite : quand la classe CDC-ACM est présente, les
 * interfaces sont regroupées par un IAD, ce qui impose le triple de code
 * "Miscellaneous / Common Class / IAD".
 */
static void fix_code_triple(struct usbd_context *ctx, const enum usbd_speed speed)
{
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS)) {
		usbd_device_set_code_triple(ctx, speed,
					    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	} else {
		usbd_device_set_code_triple(ctx, speed, 0, 0, 0);
	}
}

struct usbd_context *app_usbd_init(void)
{
	int err;

	err = usbd_add_descriptor(&app_usbd, &app_lang);
	if (err) {
		LOG_ERR("Descripteur langue (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&app_usbd, &app_mfr);
	if (err) {
		LOG_ERR("Descripteur fabricant (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&app_usbd, &app_product);
	if (err) {
		LOG_ERR("Descripteur produit (%d)", err);
		return NULL;
	}

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(&app_usbd, &app_sn);
		if (err) {
			LOG_ERR("Descripteur numero de serie (%d)", err);
			return NULL;
		}
	))

	err = usbd_add_configuration(&app_usbd, USBD_SPEED_FS, &app_fs_config);
	if (err) {
		LOG_ERR("Ajout de la configuration Full-Speed (%d)", err);
		return NULL;
	}

	err = usbd_register_all_classes(&app_usbd, USBD_SPEED_FS, 1, NULL);
	if (err) {
		LOG_ERR("Enregistrement des classes (%d)", err);
		return NULL;
	}

	fix_code_triple(&app_usbd, USBD_SPEED_FS);
	usbd_self_powered(&app_usbd, false);

	err = usbd_init(&app_usbd);
	if (err) {
		LOG_ERR("usbd_init (%d)", err);
		return NULL;
	}

	return &app_usbd;
}
