# Trackball 34 mm — PMW3610 / RP2040-Zero

Coque de trackball DIY paramétrique en OpenSCAD, dessinée pour une bille de
Ø34 mm, un capteur optique **PMW3610** et un microcontrôleur **RP2040-Zero**.

Le modèle est dérivé de [**Trackball 1.5** de jfedor (Printables #131450)](https://www.printables.com/model/131450-trackball-15),
conçu à l'origine pour une boule de billard Ø38 mm. Les cotes du support de
bille, de l'embase et du passage de câble ont été relevées sur les surfaces
analytiques du fichier source `Trackball15.step` (lecture cadquery/OCP, valeurs
exactes), puis transposées à la bille Ø34.

## Caractéristiques

- **Bille Ø34 mm** portée par 3 billes de roulement Ø2,5 (tripode à 60° du
  nadir, azimuts 30/150/270°), reprises telles quelles de l'original.
- **Retenue clipsable** (ajout par rapport à l'original) : trois languettes
  souples terminées par une lèvre arrondie empêchent la bille de sortir par le
  haut. Ouverture Ø33,46, clipsage/déclipsage par interférence, à la manière de
  la bague d'une Kensington Orbit.
- **Embase carrée** 52 × 55,75 mm, angles R8, montage par 4 vis M3 fraisées par
  dessous, taraudées dans la coque haute.
- **Passage de câble** en U dans la paroi avant, refermé par la coque haute
  (repris de l'original).
- Logements dédiés pour le breakout **PMW3610** (en long, optique centrée sous
  la bille) et le **RP2040-Zero** (en travers, derrière).

## Organisation du dépôt

| Dossier | Contenu |
| --- | --- |
| `hardware/` | Modèle 3D et documentation associée. |
| `software/` | Firmware Zephyr : souris HID USB (capteur PMW3610 + boutons). Voir [`software/README.md`](software/README.md). |

### `hardware/`

| Fichier | Description |
| --- | --- |
| `trackball.scad` | Modèle paramétrique (OpenSCAD 2021.01). |
| `index.html` | Page de documentation (cotes, coupes, vues, reste à faire). |
| `*.png` | Rendus référencés par la page HTML. |

## Câblage électronique

Capteur **PMW3610** sur le SPI0 du RP2040-Zero, plus quatre boutons. Le PMW3610
étant un SPI **3 fils half-duplex** (une seule broche de données `SDIO`) et le SPI
du RP2040 étant matériel (MOSI toujours piloté), il faut **GP4 (MISO) en prise
directe sur `SDIO` et GP3 (MOSI) relié à `SDIO` via une résistance série ~1 kΩ**.
Un pont direct GP3–GP4 provoque une contention de bus en lecture (self-test KO).

```
        RP2040-Zero                        PMW3610 (breakout)
      +-------------+                     +------------------+
      |        GP6  |------- SCLK --------| SCLK             |
      |        GP4  |----------+--------- | SDIO  (MISO      |
      |        GP3  |--[~1k]---+          |        direct,   |
      |             |                     |        MOSI 1k)  |
      |        GP5  |------- NCS  --------| NCS              |
      |        GP7  |------- MOTION ------| MOTION           |
      |        GP2  |------- NRESET ------| NRESET           |
      |        3V3  |------- VDD  --------| VDD              |
      |        GND  |------- GND  --------| GND              |
      +-------------+                     +------------------+

  Boutons — actif bas, pull-up interne (non cables = relaches, aucun clic parasite) :

      GP8  --[ /]-- GND     Gauche
      GP9  --[ /]-- GND     Droit
      GP10 --[ /]-- GND     Milieu
      GP11 --[ /]-- GND     Scroll  (bascule le mode molette)

  Sortie I2C target (optionnelle) — pull-ups ~4.7k vers 3V3, en general cote hote :

      GP28 ---- SDA ----\
                         }--- controleur hote I2C
      GP29 ---- SCL ----/
```

| Signal | Broche RP2040-Zero | Fonction |
| --- | --- | --- |
| SCLK | GP6 | Horloge SPI0 |
| SDIO | GP4 direct **+** GP3 via ~1kΩ | Données SPI0 : MISO direct, MOSI en série (half-duplex) |
| NCS | GP5 | Chip select (actif bas) |
| MOTION | GP7 | Interruption capteur (actif bas, pull-up) |
| NRESET | GP2 | Reset capteur (actif bas, piloté par le driver) |
| Bouton gauche | GP8 | Clic gauche |
| Bouton droit | GP9 | Clic droit |
| Bouton milieu | GP10 | Clic milieu |
| Bouton scroll | GP11 | Bascule mode molette (non transmis à l'hôte) |
| I2C SDA | GP28 | Sortie I2C target (vers contrôleur hôte) |
| I2C SCL | GP29 | Sortie I2C target (vers contrôleur hôte) |
| VDD / GND | 3V3 / GND | Alimentation |

Le trackball fonctionne **sans aucun bouton câblé** (souris qui se déplace, sans
clic) : les entrées ont un pull-up interne, une broche non connectée est donc lue
au niveau « relâché ». L'alimentation vient du connecteur **USB-C** du RP2040-Zero.

### Sortie I2C target (optionnelle)

En plus de la souris USB, le trackball peut être lu par un **contrôleur hôte** via
I2C (mode target, adresse **0x54** par défaut). Carte de registres (auto-incrément) :

| Reg | Nom | Contenu |
| --- | --- | --- |
| 0x00 | WHO_AM_I | `0x3C` (identification) |
| 0x01 | BUTTONS | bit0 gauche, bit1 droit, bit2 milieu |
| 0x02 | DX | delta X (int8, remis à zéro à la lecture) |
| 0x03 | DY | delta Y (int8, remis à zéro à la lecture) |

L'hôte écrit l'index de registre puis lit N octets. Détails : [`software/README.md`](software/README.md).

Côté bus : seulement **SDA (GP28), SCL (GP29) et GND** entre le trackball et l'hôte,
avec une **masse commune obligatoire**. Les **pull-ups (~2,2–4,7 kΩ) sont fournis par
l'hôte** — rien à ajouter côté trackball.

### Alimentation : USB ou hôte I2C

Le trackball peut être alimenté de deux façons, au choix :

- **par son USB-C** (il reste aussi une souris USB) ;
- **par le 3,3 V de l'hôte** via le connecteur I2C (module pur, sans USB).

Le RP2040-Zero n'a pas de broche `3V3_EN` : on ne peut pas couper son régulateur
interne. Relier directement le 3,3 V de l'hôte au pad `3V3` **pendant que l'USB est
branché** ferait se battre deux sources. Pour permettre **l'un ou l'autre sans
risque**, intercaler une **diode Schottky** (OR-ing) sur la ligne 3,3 V :

```
  Hôte +3V3 ──►|── pad 3V3 (RP2040-Zero)
             Schottky            (anode = hôte, cathode = carte)

  SDA(GP28), SCL(GP29), GND : câblés directement
```

- **USB branché** → le régulateur interne alimente la carte, la diode est bloquée
  (pas de retour vers l'hôte).
- **USB absent** → l'hôte alimente la carte à travers la diode (~3,0 V après la
  chute Schottky ; OK pour le RP2040 et le PMW3610).

Diodes adaptées (I_F ≥ 0,5 A, V_f faible, V_r 20–40 V) :

| Type | Références |
| --- | --- |
| CMS | **PMEG2010ER**, MBR0520L, SS14, B5819W |
| Traversant | **1N5817**, 1N5819 |

Pour le **3,3 V plein** (chute quasi nulle), remplacer la diode par un *ideal-diode*
(**LM66100**, TPS22860). Dans tous les cas : **ne pas relier** les rails 3V3 des deux
cartes autrement que par cette diode, et **ne pas** brancher l'USB en même temps que
l'alimentation par l'hôte si tu utilises un simple cavalier au lieu de la diode.

## Utilisation

Ouvrir `hardware/trackball.scad` dans [OpenSCAD](https://openscad.org/) (≥ 2021.01). La
variable `RENDER` sélectionne la pièce ou la vue à générer :

| Valeur | Rendu |
| --- | --- |
| `assembly` | Assemblage complet (défaut). |
| `top` | Coque haute (support de bille + retenue). |
| `bottom` | Bac électronique. |
| `section_h` | Coupe horizontale au plan des billes de roulement. |
| `section_v` | Coupe verticale (X = 0). |

Les principaux paramètres (diamètre de bille, jeux, interférence des lèvres,
dimensions des cartes, vis) sont regroupés en tête de fichier et documentés
inline. Exporter chaque pièce en STL depuis OpenSCAD pour l'impression.

Matière conseillée : **PETG** (le PLA est à la limite pour la flexion des
languettes de retenue).

## Reste à faire

Voir la section « Reste à faire avant impression définitive » de `hardware/index.html` :
support de lentille LM18-LSI/PMW3610 non modélisé, hypothèses sur la hauteur du
RP2040-Zero et le passage USB-C, coupon d'essai plaque + 3 poteaux recommandé
avant l'impression complète.

## Références

- [Trackball 1.5 — jfedor (Printables #131450)](https://www.printables.com/model/131450-trackball-15) — modèle de base.
- [siderakb/pmw3610-pcb](https://github.com/siderakb/pmw3610-pcb) — breakout PMW3610.
- [OpenSCAD](https://openscad.org/) — logiciel de CAO paramétrique.

## Crédits & licence

Travail dérivé de Trackball 1.5 (jfedor). Vérifier les conditions de licence du
modèle d'origine sur sa page Printables avant toute redistribution.
