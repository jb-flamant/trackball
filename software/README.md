# Firmware — Trackball RP2040-Zero + PMW3610 (Zephyr)

Firmware Zephyr reliant le capteur optique **PMW3610** au **RP2040-Zero**. Cette
première étape valide la liaison MCU ↔ capteur : le déplacement de la bille est lu
via le sous-système `input` de Zephyr et journalisé sur la console USB. L'étape HID
souris viendra ensuite.

## Faisabilité de l'architecture

Vérifiée avant d'écrire le code :

| Élément | État | Détail |
| --- | --- | --- |
| RP2040-Zero sous Zephyr | ✅ supporté en amont | Carte `rp2040_zero` (Waveshare) intégrée à Zephyr. |
| Driver PMW3610 | ⚠️ hors-arbre | Pas de driver PMW3610 en amont ; on utilise le module communautaire ZMK ([`badjeff/zmk-pmw3610-driver`](https://github.com/badjeff/zmk-pmw3610-driver), Zephyr ≥ 3.5), qui alimente le sous-système `input`. |
| Liaison électrique | ⚠️ point d'attention | Le PMW3610 est un **SPI 3 fils half-duplex** (une seule broche de données SDIO) ; le RP2040 est full-duplex → MOSI et MISO à ponter sur SDIO (voir overlay). |

### Points à valider sur matériel

1. **Couplage ZMK du driver.** Le module tire des symboles Kconfig ZMK (ex.
   `CONFIG_ZMK_POINTING`). Pour cette application Zephyr *pure*, il faut soit fournir
   un shim de ces symboles, soit utiliser un fork « dé-ZMKifié ». C'est le principal
   risque d'intégration ; à traiter au premier `west build`.
2. **SPI half-duplex.** Le pont MOSI↔MISO sur la broche SDIO doit être fait côté
   carte. Alternative plus propre sur RP2040 : implémenter le SPI en **PIO**.
3. **Console USB.** Le label `zephyr_udc0` et le nœud CDC-ACM sont à confirmer selon
   la version de Zephyr figée.

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
├── prj.conf                     # Kconfig : SPI, INPUT, USB CDC, CONFIG_PMW3610_ALT
├── west.yml                     # manifeste : Zephyr + module PMW3610
├── boards/
│   └── rp2040_zero.overlay      # câblage capteur + console USB
└── src/
    └── main.c                   # lit les évènements input, journalise dx/dy
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

## Références

- [Driver PMW3610 — badjeff/zmk-pmw3610-driver](https://github.com/badjeff/zmk-pmw3610-driver)
- [Carte RP2040-Zero — documentation Zephyr](https://docs.zephyrproject.org/latest/boards/waveshare/rp2040_zero/doc/index.html)
- [Breakout PMW3610 — siderakb/pmw3610-pcb](https://github.com/siderakb/pmw3610-pcb)
