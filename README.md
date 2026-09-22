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

## Fichiers

| Fichier | Description |
| --- | --- |
| `trackball.scad` | Modèle paramétrique (OpenSCAD 2021.01). |
| `index.html` | Page de documentation (cotes, coupes, vues, reste à faire). |
| `*.png` | Rendus référencés par la page HTML. |

## Utilisation

Ouvrir `trackball.scad` dans [OpenSCAD](https://openscad.org/) (≥ 2021.01). La
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

Voir la section « Reste à faire avant impression définitive » de `index.html` :
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
