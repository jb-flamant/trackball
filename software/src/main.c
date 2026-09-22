/*
 * Trackball RP2040-Zero + PMW3610 — souris HID USB (Zephyr, stack device_next)
 *
 * Le driver PMW3610 (module hors-arbre) publie le deplacement de la bille dans le
 * sous-systeme `input` sous forme d'evenements relatifs INPUT_REL_X / INPUT_REL_Y.
 * On les convertit en rapports de souris HID (boutons, X, Y, molette) transmis a
 * l'hote par l'USB. Un lot d'evenements est clos par le drapeau `sync`, ce qui
 * permet de regrouper X et Y dans un meme rapport.
 */

#include "usb.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Descripteur de rapport : souris a 2 boutons + X/Y/molette relatifs. */
static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

enum mouse_report_idx {
	MOUSE_BTN_REPORT_IDX = 0,
	MOUSE_X_REPORT_IDX = 1,
	MOUSE_Y_REPORT_IDX = 2,
	MOUSE_WHEEL_REPORT_IDX = 3,
	MOUSE_REPORT_COUNT = 4,
};

/* File des rapports prets a emettre (producteur : input_cb ; conso : main). */
K_MSGQ_DEFINE(mouse_msgq, MOUSE_REPORT_COUNT, 8, 1);

static const struct device *hid_dev;
static bool mouse_ready;

/* Protocole HID courant (piloté par l'hôte via SET_PROTOCOL).
 * En mode boot (BIOS/UEFI), le rapport souris est figé à 3 octets
 * (boutons, X, Y) ; en mode report, on émet aussi la molette (4 octets).
 * Les 3 premiers octets de notre rapport correspondent déjà au format boot. */
static volatile uint8_t hid_protocol = HID_PROTOCOL_REPORT;
#define MOUSE_BOOT_REPORT_COUNT 3

/* Borne un delta au domaine signe 8 bits du rapport HID. */
static inline uint8_t clamp_delta(int32_t v)
{
	if (v > 127) {
		v = 127;
	} else if (v < -127) {
		v = -127;
	}
	return (uint8_t)(int8_t)v;
}

/* Callback du sous-systeme input : accumule X/Y puis emet a la cloture (sync). */
static void input_cb(struct input_event *evt, void *user_data)
{
	static uint8_t report[MOUSE_REPORT_COUNT];

	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_REL_X:
		report[MOUSE_X_REPORT_IDX] = clamp_delta(evt->value);
		break;
	case INPUT_REL_Y:
		report[MOUSE_Y_REPORT_IDX] = clamp_delta(evt->value);
		break;
	default:
		return;
	}

	if (!evt->sync) {
		return;   /* lot incomplet : on attend le dernier evenement */
	}

	if (k_msgq_put(&mouse_msgq, report, K_NO_WAIT) != 0) {
		LOG_WRN("File de rapports pleine, mouvement perdu");
	}

	report[MOUSE_X_REPORT_IDX] = 0U;
	report[MOUSE_Y_REPORT_IDX] = 0U;
}
INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

static void mouse_iface_ready(const struct device *dev, const bool ready)
{
	LOG_INF("Interface HID %s : %s", dev->name, ready ? "prete" : "non prete");
	mouse_ready = ready;
}

static int mouse_get_report(const struct device *dev, const uint8_t type,
			    const uint8_t id, const uint16_t len, uint8_t *const buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(id);
	ARG_UNUSED(len);
	ARG_UNUSED(buf);
	return 0;
}

static void mouse_set_protocol(const struct device *dev, const uint8_t proto)
{
	ARG_UNUSED(dev);
	hid_protocol = proto;
	LOG_INF("Protocole HID : %s", proto == HID_PROTOCOL_BOOT ? "boot" : "report");
}

static struct hid_device_ops mouse_ops = {
	.iface_ready = mouse_iface_ready,
	.get_report = mouse_get_report,
	.set_protocol = mouse_set_protocol,
};

int main(void)
{
	const struct device *const sensor = DEVICE_DT_GET(DT_NODELABEL(pmw3610));
	struct usbd_context *usbd;
	int ret;

	hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
	if (!device_is_ready(hid_dev)) {
		LOG_ERR("Peripherique HID non pret");
		return -EIO;
	}

	if (!device_is_ready(sensor)) {
		LOG_ERR("Capteur PMW3610 non pret");
		return -ENODEV;
	}

	ret = hid_device_register(hid_dev, hid_report_desc,
				  sizeof(hid_report_desc), &mouse_ops);
	if (ret != 0) {
		LOG_ERR("Enregistrement du peripherique HID (%d)", ret);
		return ret;
	}

	usbd = app_usbd_init();
	if (usbd == NULL) {
		LOG_ERR("Initialisation USB echouee");
		return -ENODEV;
	}

	ret = usbd_enable(usbd);
	if (ret != 0) {
		LOG_ERR("usbd_enable (%d)", ret);
		return ret;
	}

	LOG_INF("Trackball pret : souris HID USB active");

	while (true) {
		UDC_STATIC_BUF_DEFINE(report, MOUSE_REPORT_COUNT);

		k_msgq_get(&mouse_msgq, &report, K_FOREVER);

		if (!mouse_ready) {
			continue;   /* hote non connecte : on ignore le mouvement */
		}

		/* Mode boot : rapport de 3 octets (boutons, X, Y) sans molette. */
		const uint8_t len = (hid_protocol == HID_PROTOCOL_BOOT)
					    ? MOUSE_BOOT_REPORT_COUNT
					    : MOUSE_REPORT_COUNT;

		ret = hid_device_submit_report(hid_dev, len, report);
		if (ret != 0) {
			LOG_ERR("Envoi du rapport HID (%d)", ret);
		}
	}

	return 0;
}
