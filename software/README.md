# Firmware — Trackball RP2040-Zero + PMW3610 (Zephyr)

Firmware Zephyr transformant le capteur optique **PMW3610** en **souris HID USB**
sur **RP2040-Zero**. Le capteur est piloté par le **driver PMW3610 fourni en amont
par Zephyr** (`drivers/input`), qui publie le déplacement de la bille dans le
sous-système `input` (`INPUT_REL_X` / `INPUT_REL_Y`) ; l'application convertit ces
évènements en rapports de souris HID transmis à l'hôte par l'USB. L'interface est
annoncée comme **souris boot-capable** (`protocol-code = "mouse"`) : elle fonctionne
donc aussi avant l'OS (BIOS/UEFI), le firmware émettant un rapport de 3 octets en
mode boot et de 4 octets (avec molette) en mode report. Une console de debug est
exposée en parallèle sur une CDC-ACM (périphérique USB composite).

Aucune dépendance à ZMK ni à un module hors-arbre : tout est du **Zephyr pur**, et
le driver du capteur est **in-tree** (maintenu par le projet Zephyr).

## Versions figées

| Composant | Révision |
| --- | --- |
| Zephyr | `v4.4.0` (dernière stable, avril 2026) |

Le stack USB **legacy** (`CONFIG_USB_DEVICE_STACK`) étant déprécié et supprimé en
Zephyr 4.5, le firmware utilise directement le **nouveau stack** `device_next`
(`CONFIG_USB_DEVICE_STACK_NEXT`).

## Driver PMW3610

Fourni par Zephyr (`CONFIG_INPUT_PMW3610`, compatible `pixart,pmw3610`). Configuré
via le devicetree (overlay) :

- `motion-gpios` : ligne d'interruption MOTION (active bas).
- `res-cpi` : résolution (défaut 800, plage 200-3200).
- `zephyr,axis-x` / `zephyr,axis-y` : codes d'évènement publiés (`INPUT_REL_X/Y`).
- `invert-x` / `invert-y`, `force-awake`, `smart-mode` : options facultatives.

Le driver expose aussi des API runtime (`pmw3610_set_resolution()`,
`pmw3610_force_awake()`) non utilisées ici.

## Faisabilité de l'architecture

| Élément | État | Détail |
| --- | --- | --- |
| RP2040-Zero sous Zephyr | ✅ supporté en amont | Carte `rp2040_zero` (Waveshare) intégrée à Zephyr. |
| Driver PMW3610 | ✅ en amont | Driver `drivers/input` officiel (Zephyr ≥ 4.4), sans ZMK ni module externe. |
| Liaison électrique | ⚠️ point d'attention | Le PMW3610 est un **SPI 3 fils half-duplex** (une seule broche de données SDIO) ; le RP2040 est full-duplex → MOSI et MISO à ponter sur SDIO (voir overlay). |

### Points à valider sur matériel

1. **SPI half-duplex.** Le pont MOSI↔MISO sur la broche SDIO doit être fait côté
   carte. Alternative plus propre sur RP2040 : implémenter le SPI en **PIO**.
2. **VID/PID.** `src/usb.c` utilise une paire de TEST (`0x1209/0x0001`, pid.codes) —
   **à remplacer** avant toute distribution.
3. **Non testé sur cible.** Le firmware compile en CI mais n'a pas encore tourné sur
   un PMW3610 réel : sens des axes et `res-cpi` à confirmer (puis ajuster
   `invert-x` / `invert-y`).

## Câblage (overlay `boards/rp2040_zero.overlay`)

Reprend le `spi0_default` de la carte :

| Signal | RP2040-Zero | Fonction |
| --- | --- | --- |
| SCLK | GP6 | SPI0 SCK |
| SDIO | GP4 direct **+** GP3 via ~1kΩ | SPI0 MISO direct, MOSI en série (half-duplex) |
| NCS | GP5 | Chip select (GPIO) |
| MOTION | GP7 | Interruption (`motion-gpios`, actif bas, pull-up) |
| NRESET | GP2 | Reset capteur (`reset-gpios`, actif bas) |
| Bouton gauche | GP8 | `INPUT_BTN_LEFT` |
| Bouton droit | GP9 | `INPUT_BTN_RIGHT` |
| Bouton milieu | GP10 | `INPUT_BTN_MIDDLE` |
| Bouton scroll | GP11 | Bascule mode molette (interne, non transmis) |
| I2C SDA | GP28 | Sortie I2C target (`i2c0`) |
| I2C SCL | GP29 | Sortie I2C target (`i2c0`) |
| VDD / GND | 3V3 / GND | Alimentation |

Boutons via `gpio-keys`, actifs bas avec pull-up interne : une entrée non câblée
est lue « relâchée » (pas de clic parasite). Le bouton **scroll** est en logique
**bascule** (toggle) : une pression active/désactive le mode molette, dans lequel
l'axe Y du capteur alimente la molette (`SCROLL_DIV` counts par cran). Le scroll
n'opère qu'en mode report (la molette est absente du rapport boot).

## Sortie I2C target

En plus de la souris USB, le trackball s'enregistre comme **périphérique I2C target**
(`CONFIG_I2C_TARGET`, driver DesignWare du RP2040) sur `i2c0` — **GP28 (SDA) / GP29
(SCL)** — pour servir de module lu par un contrôleur hôte. Pull-ups externes requis
(~4,7 kΩ vers 3V3), généralement côté hôte. Adresse **0x54** (modifiable dans
`src/i2c_target.c`).

Carte de registres (l'hôte écrit l'index de départ, puis lit N octets avec
auto-incrément) :

| Reg | Nom | Contenu |
| --- | --- | --- |
| 0x00 | WHO_AM_I | `0x3C` |
| 0x01 | BUTTONS | bit0 gauche, bit1 droit, bit2 milieu (niveau) |
| 0x02 | DX | delta X (int8), **remis à zéro à la lecture** |
| 0x03 | DY | delta Y (int8), **remis à zéro à la lecture** |

Les deltas sont bornés à ±127 et le reliquat conservé (aucune perte si le mouvement
dépasse 127 entre deux lectures). Le mouvement exposé est **brut** (indépendant du
mode molette USB). Les callbacks target s'exécutent en contexte ISR ; l'accumulation
côté `input` est protégée par `irq_lock()`.

> ⚠️ Adresse et carte de registres = **contrat avec l'hôte** : à adapter si le
> contrôleur attend un protocole précis. Non encore testé sur cible.

## Arborescence

```
software/
├── CMakeLists.txt
├── prj.conf                     # Kconfig : SPI, INPUT_PMW3610, USB device_next, I2C target
├── west.yml                     # manifeste : Zephyr v4.4.0
├── boards/
│   └── rp2040_zero.overlay      # câblage capteur + HID + console CDC + I2C0
└── src/
    ├── main.c                   # input → rapports souris HID + feed I2C
    ├── usb.c                    # contexte USB device_next (HID + CDC-ACM)
    ├── usb.h
    ├── i2c_target.c             # sortie I2C target (carte de registres)
    └── i2c_target.h
```

## Compilation

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
Le build est **bloquant** : tout push qui casserait la compilation fera échouer la CI.

## Références

- [Driver PMW3610 — Zephyr `drivers/input`](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/drivers/input/input_pmw3610.c)
- [Carte RP2040-Zero — documentation Zephyr](https://docs.zephyrproject.org/latest/boards/waveshare/rp2040_zero/doc/index.html)
- [Sample HID souris (device_next) — Zephyr](https://github.com/zephyrproject-rtos/zephyr/tree/v4.4.0/samples/subsys/usb/hid-mouse)
- [Breakout PMW3610 — siderakb/pmw3610-pcb](https://github.com/siderakb/pmw3610-pcb)
