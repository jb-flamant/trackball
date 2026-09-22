# Firmware — Trackball RP2040-Zero + PMW3610 (Zephyr)

Firmware Zephyr transformant le capteur optique **PMW3610** en **souris HID USB**
sur **RP2040-Zero**. Le déplacement de la bille est lu via le sous-système `input`
de Zephyr (évènements `INPUT_REL_X` / `INPUT_REL_Y` émis par le driver), converti
en rapports de souris HID, puis transmis à l'hôte par l'USB. Une console de debug
est exposée en parallèle sur une CDC-ACM (périphérique USB composite).

## Versions figées

| Composant | Révision |
| --- | --- |
| Zephyr | `v4.4.0` (dernière stable, avril 2026) |
| `zmk-pmw3610-driver` | commit `44b4a76` |

Le stack USB **legacy** (`CONFIG_USB_DEVICE_STACK`) étant déprécié et supprimé en
Zephyr 4.5, le firmware utilise directement le **nouveau stack** `device_next`
(`CONFIG_USB_DEVICE_STACK_NEXT`).

## Faisabilité de l'architecture

Vérifiée avant d'écrire le code :

| Élément | État | Détail |
| --- | --- | --- |
| RP2040-Zero sous Zephyr | ✅ supporté en amont | Carte `rp2040_zero` (Waveshare) intégrée à Zephyr. |
| Driver PMW3610 | ⚠️ hors-arbre | Pas de driver PMW3610 en amont ; on utilise le module communautaire ZMK ([`badjeff/zmk-pmw3610-driver`](https://github.com/badjeff/zmk-pmw3610-driver), Zephyr ≥ 3.5), qui alimente le sous-système `input`. |
| Liaison électrique | ⚠️ point d'attention | Le PMW3610 est un **SPI 3 fils half-duplex** (une seule broche de données SDIO) ; le RP2040 est full-duplex → MOSI et MISO à ponter sur SDIO (voir overlay). |

### Points à valider sur matériel

1. **Couplage ZMK du driver.** Le driver inclut `<zmk/keymap.h>` et
   `<zmk/events/activity_state_changed.h>` et enregistre un écouteur d'activité ZMK
   (endormissement automatique). Inexistants hors ZMK, ces symboles sont fournis par
   un **shim minimal** (`zmk_shim/`, ajouté au chemin d'include dans `CMakeLists.txt`)
   qui les réduit à des no-op : le capteur ne s'endort plus tout seul, sans effet sur
   la lecture. Alternative plus propre à terme : un fork « dé-ZMKifié » du driver.
2. **SPI half-duplex.** Le pont MOSI↔MISO sur la broche SDIO doit être fait côté
   carte. Alternative plus propre sur RP2040 : implémenter le SPI en **PIO**.
3. **VID/PID.** `src/usb.c` utilise une paire de TEST (`0x1209/0x0001`, pid.codes) —
   **à remplacer** avant toute distribution.
4. **Non compilé.** Le squelette a été rédigé sans SDK Zephyr disponible ; premier
   `west build` = première compilation. Le `sync` des évènements input du driver et
   le fonctionnement de la console CDC composite sont à confirmer sur cible.

## Câblage (overlay `boards/rp2040_zero.overlay`)

Reprend le `spi0_default` de la carte :

| Signal PMW3610 | RP2040-Zero | Fonction |
| --- | --- | --- |
| SCLK | GP6 | SPI0 SCK |
| SDIO | GP3 **+** GP4 pontés | SPI0 MOSI + MISO (half-duplex) |
| NCS | GP5 | Chip select (GPIO) |
| MOTION | GP7 | Interruption (actif bas, pull-up) |
| VDD / GND | 3V3 / GND | Alimentation |

## Arborescence

```
software/
├── CMakeLists.txt
├── prj.conf                     # Kconfig : SPI, INPUT, USB device_next (HID+CDC)
├── west.yml                     # manifeste : Zephyr v4.4.0 + module PMW3610
├── boards/
│   └── rp2040_zero.overlay      # câblage capteur + noeud HID + console CDC
└── src/
    ├── main.c                   # input → rapports souris HID
    ├── usb.c                    # contexte USB device_next (HID + CDC-ACM)
    └── usb.h
```

## Compilation

> ⚠️ Nécessite le SDK Zephyr et `west` (non disponibles dans l'environnement où ce
> squelette a été généré ; le code n'a donc pas encore été compilé).

```sh
# Espace de travail west basé sur ce dépôt d'application
west init -l software
west update
west zephyr-export

# Build pour le RP2040-Zero
west build -b rp2040_zero software

# Flash : maintenir BOOT en branchant l'USB (lecteur UF2), puis
west flash
```

Une fois flashé, la carte énumère comme souris USB *et* comme port série virtuel
(console de debug).

## Intégration continue

Le workflow `.github/workflows/build.yml` compile le firmware pour `rp2040_zero`
sur un runner GitHub (action officielle `zephyrproject-rtos/action-zephyr-setup`,
toolchain `arm-zephyr-eabi`) et publie les artefacts `zephyr.uf2` / `zephyr.elf`.
Le build est **bloquant** : le firmware compile (shim ZMK en place), tout push qui
casserait la compilation fera échouer la CI.

## Références

- [Driver PMW3610 — badjeff/zmk-pmw3610-driver](https://github.com/badjeff/zmk-pmw3610-driver)
- [Carte RP2040-Zero — documentation Zephyr](https://docs.zephyrproject.org/latest/boards/waveshare/rp2040_zero/doc/index.html)
- [Sample HID souris (device_next) — Zephyr](https://github.com/zephyrproject-rtos/zephyr/tree/v4.4.0/samples/subsys/usb/hid-mouse)
- [Breakout PMW3610 — siderakb/pmw3610-pcb](https://github.com/siderakb/pmw3610-pcb)
