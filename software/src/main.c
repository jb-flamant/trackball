/*
 * Trackball RP2040-Zero + PMW3610 — squelette firmware Zephyr
 *
 * Le driver PMW3610 (module hors-arbre) publie le deplacement de la bille dans le
 * sous-systeme `input` sous forme d'evenements relatifs INPUT_REL_X / INPUT_REL_Y.
 * Ce main se contente, pour cette premiere etape, d'accumuler ces deltas et de les
 * journaliser sur la console USB : de quoi valider la liaison MCU <-> capteur.
 *
 * Etape suivante (non incluse) : exposer une souris HID USB a partir de ces deltas.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usb_device.h>

LOG_MODULE_REGISTER(trackball, LOG_LEVEL_INF);

#define PMW3610_NODE DT_NODELABEL(pmw3610)
BUILD_ASSERT(DT_NODE_EXISTS(PMW3610_NODE),
	     "Noeud pmw3610 absent : verifier l'overlay de la carte.");

/* Deltas accumules entre deux releves du fil principal. */
static atomic_t rel_x;
static atomic_t rel_y;

/* Callback appele par le sous-systeme input pour chaque evenement du capteur. */
static void pmw3610_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_REL_X:
		atomic_add(&rel_x, evt->value);
		break;
	case INPUT_REL_Y:
		atomic_add(&rel_y, evt->value);
		break;
	default:
		break;
	}
}
INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(PMW3610_NODE), pmw3610_cb, NULL);

int main(void)
{
	const struct device *const sensor = DEVICE_DT_GET(PMW3610_NODE);

	/* Console USB CDC-ACM (voir chosen zephyr,console dans l'overlay). */
	if (usb_enable(NULL)) {
		LOG_ERR("Echec de l'initialisation USB");
		return -EIO;
	}
	/* Laisse le temps a l'hote d'ouvrir le port serie virtuel. */
	k_sleep(K_SECONDS(2));

	LOG_INF("Trackball RP2040-Zero + PMW3610 : demarrage");

	if (!device_is_ready(sensor)) {
		LOG_ERR("Capteur PMW3610 non pret");
		return -ENODEV;
	}
	LOG_INF("Capteur PMW3610 pret ; en attente de mouvement");

	while (1) {
		const int dx = (int)atomic_set(&rel_x, 0);
		const int dy = (int)atomic_set(&rel_y, 0);

		if (dx != 0 || dy != 0) {
			LOG_INF("dx=%d dy=%d", dx, dy);
		}
		k_sleep(K_MSEC(50));
	}

	return 0;
}
