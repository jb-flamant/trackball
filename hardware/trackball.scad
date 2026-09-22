// =============================================================================
// Trackball DIY paramétrique — bille 34mm / PMW3610 / RP2040-Zero      (v7)
// =============================================================================
// Dérivé de "Trackball 1.5" (jfedor, Printables #131450), dessiné pour une boule de
// billard 38mm. Cotes lues sur les surfaces analytiques de Trackball15.step
// (cadquery/OCP), voir doc projet claude/mesures-step-trackball15.md.
//
// SUPPORT DE BILLE (v4, repris tel quel de l'original, transposé à 34mm) :
//   calotte R+0.625, ouverture Ø10, plaque 4mm, plan de joint 0.4mm sous le pôle sud,
//   3 billes de roulement Ø2.5 dans logements borgnes Ø2.6 à 60° du nadir, azimuts
//   30/150/270, murs-poteaux 10mm montant jusqu'à l'équateur.
// EMBASE CARRÉE (v5, reprise de l'original) :
//   corps rectangulaire largeur 52 (x ±26), angles R8, parois et fond 2mm, bord de
//   plaque vertical 1mm puis chanfrein 45° sur 3mm ; les poteaux dépassent le corps de
//   1mm (angles avant R9, nervure arrière) ; 4 vis M3 aux centres d'angle, montées par
//   dessous (fraisage dans le fond), taraudées dans la coque haute (avant trou pilote
//   Ø2.8 dans le poteau, arrière dans la plaque), comme sur l'original.
//   Profondeur : original y -36 → +18 autour de l'axe bille ; ici ajustée pour loger
//   le PMW3610 (en long, centré sous la bille) et le RP2040-Zero (en travers, derrière).
// RETENUE CLIPSABLE (v6, ajout, pas sur l'original) :
//   chaque poteau est prolongé au-dessus de l'équateur par une languette souple (6mm de
//   large, 1.6mm d'épaisseur) portant une lèvre arrondie tournée vers l'intérieur.
//   Ouverture entre les lèvres < Ø bille de 2×lip_interf : la bille se clipse en force
//   (rampe d'entrée à 45°) et se retire en tirant (face inférieure en pente), comme la
//   bague de la Kensington Orbit. Au repos, jeu lèvre/bille lip_clear ; la bille peut
//   se soulever d'environ 1mm avant de buter. Effort estimé ≈20N au total (PETG).
// PASSAGE DE CÂBLE (v7, repris de l'original) : encoche en U dans la paroi avant du bac
//   (côté bille), centrée x = -12, largeur 5, fond R2.5 à 5.5mm sous le plan de joint ;
//   la coque haute referme l'encoche.
// Provenance de chaque cote : STEP (mesuré sur l'original), DATASHEET, CHOIX.
// =============================================================================

/* [Bille] */
ball_d          = 34;      // spec projet (38 à l'origine)
ball_r          = ball_d/2;

/* [Support de bille] — STEP Trackball 1.5, valeurs indépendantes du diamètre */
cup_clearance   = 0.625;   // STEP : jeu radial calotte/poteaux ↔ bille
brg_ball_d      = 2.5;     // STEP : billes de roulement Ø2.5 (céramique/acier)
brg_hole_d      = 2.6;     // STEP : logement Ø2.6
brg_polar       = 60;      // STEP : angle depuis le nadir (60.0° exact)
brg_az          = [30, 150, 270]; // STEP : 30/150 vers les angles avant, 270 vers l'arrière
post_w          = 10;      // STEP : largeur des murs-poteaux
post_top_flat   = 2;       // STEP : épaisseur radiale du sommet plat
post_overhang   = 1;       // STEP : dépassement des poteaux hors du corps (R9 vs R8)
plate_t         = 4;       // STEP : épaisseur plaque haute (Z 7→11)
plate_edge_v    = 1;       // STEP : bord vertical de la plaque (Z 7→8)
plate_chamfer   = 3;       // STEP : chanfrein 45° (Z 8→11)
split_below_pole= 0.4;     // STEP : plan de joint sous le pôle sud de la bille
aperture_d      = 10;      // STEP : ouverture optique au fond de la calotte

/* [Retenue clipsable] — CHOIX (v6) */
lip_interf      = 0.3;     // interférence radiale par lèvre (ouverture = Ø bille − 2×lip_interf)
lip_clear       = 0.2;     // jeu lèvre ↔ bille au repos
lip_finger_t    = 1.6;     // épaisseur radiale de la languette (flexion)
lip_finger_w    = 6;       // largeur de la languette (centrée sur le poteau)
lip_lead        = 1.2;     // hauteur de la rampe d'entrée à 45° au-dessus de la lèvre
lip_round       = 0.3;     // arrondi du nez de lèvre

/* [Embase carrée] */
base_w          = 52;      // STEP : x ±26
front_min       = 18;      // STEP : bord avant à 18mm de l'axe bille (bille en porte-à-faux)
corner_r        = 8;       // STEP : rayon des angles
wall            = 2.0;     // STEP : parois du bac
floor_t         = 2.0;     // STEP : fond du bac
pillar_r        = 4;       // CHOIX : piliers de vis dans les angles du bac
elec_gap        = 0.75;    // CHOIX : jeu autour des cartes

/* [Passage de câble] — STEP : encoche en U dans la paroi avant du bac */
cable_x         = -12;     // STEP : centre de l'encoche
cable_w         = 5;       // STEP : largeur (fond R2.5)
cable_depth     = 5.5;     // STEP : profondeur sous le plan de joint

/* [Vis] — M3 fraisée par dessous, taraudée dans la coque haute (comme l'original) */
screw_clear_d   = 3.4;
screw_head_d    = 6.5;     // fraisage 90°
screw_pilot_d   = 2.8;     // STEP : trou pilote Ø2.8
pilot_depth_front = 8;     // CHOIX : dans le poteau avant
pilot_depth_back  = 3.5;   // STEP : 3mm dans la plaque d'origine

/* [PMW3610 breakout] — plan coté fabricant */
pmw_hole_dx     = 17.5;
pmw_hole_dy     = 25.5;
pmw_hole_d      = 3.2;
pmw_thickness   = 1.6;

/* [Empilement optique PMW3610] — DATASHEET PMW3610DM-SUDU */
lens_gap        = 2.4;     // Table 2 : lentille -> surface, 2.2/2.4/2.6
pcb_top_to_ball = 7.40;    // Fig.4 : "Top of PCB to Surface" (montage de référence)

/* [RP2040-Zero] — fiche vendeur */
rp_w            = 18.0;
rp_h            = 24.0;
rp_thickness    = 1.6;     // CHOIX
rp_h_components = 3.5;     // CHOIX (hypothèse) : composants + USB-C

/* [Supports cartes] */
standoff_h      = 2.5;     // CHOIX

/* [Rendu] */
RENDER    = "assembly";    // assembly | top | bottom | section_h | section_v
SHOW_BALL = true;          // bille + billes de roulement de référence (hors STL)
$fn = 72;

// =============================================================================
// Repères verticaux (Z=0 = dessous du bac)
// =============================================================================
pcb_top_z     = floor_t + standoff_h + pmw_thickness;
ball_south_z  = pcb_top_z + pcb_top_to_ball;       // DATASHEET
ball_center_z = ball_south_z + ball_r;
split_z       = ball_south_z - split_below_pole;   // STEP (principe)
plate_top_z   = split_z + plate_t;
R_cup         = ball_r + cup_clearance;
brg_r         = ball_r + brg_ball_d/2;             // centre bille de roulement
brg_bottom_r  = ball_r + brg_ball_d;               // fond du logement
rp_top_z      = floor_t + rp_thickness + rp_h_components;
lip_r_tip     = ball_r - lip_interf;                              // rayon horizontal du nez
lip_h         = sqrt(ball_r*ball_r - pow(lip_r_tip - lip_clear, 2)); // hauteur du nez / équateur

// =============================================================================
// Embase : rectangle à angles R8 ; profondeur calculée pour les deux cartes
// =============================================================================
pmw_len  = 31.5;  pmw_wid = 23.5;                  // plan coté (en long selon Y)
front_y  = max(front_min, pmw_len/2 + elec_gap + wall);
rp_cy    = -(pmw_len/2 + elec_gap + rp_w/2);       // RP2040 en travers, derrière le PMW3610
back_y   = -(pmw_len/2 + elec_gap + rp_w + elec_gap + wall);
corner_c = [[ base_w/2-corner_r, front_y-corner_r], [-(base_w/2-corner_r), front_y-corner_r],
            [ base_w/2-corner_r, back_y+corner_r],  [-(base_w/2-corner_r), back_y+corner_r]];
screw_pos = corner_c;

echo(str("embase ", base_w, " x ", front_y-back_y, "  (y ", back_y, " -> ", front_y,
         ")  split_z=", split_z, " plate_top_z=", plate_top_z, " ball_center_z=", ball_center_z));
assert(abs(cable_x) + cable_w/2 <= base_w/2 - corner_r, "Encoche de câble dans l'arrondi d'angle");
assert(split_z >= rp_top_z + 0.5, "Plan de joint trop bas pour le RP2040-Zero");
assert(base_w/2 - corner_r - pillar_r >= pmw_wid/2 + 0.5, "Piliers avant en collision avec le PMW3610");
assert(base_w/2 - corner_r - pillar_r >= rp_h/2 + 1, "Piliers arrière en collision avec le RP2040");

module footprint_2d() {
    hull() for (c = corner_c) translate(c) circle(r=corner_r);
}

// direction radiale (unitaire) du palier i, depuis le centre bille
function brg_dir(i) = [sin(brg_polar)*cos(brg_az[i]), sin(brg_polar)*sin(brg_az[i]), -cos(brg_polar)];
function brg_center(i) = [0,0,ball_center_z] + brg_r*brg_dir(i);

// =============================================================================
// BAC ÉLECTRONIQUE (bas)
// =============================================================================
// encoche en U traversant la paroi avant, ouverte vers le haut (fermée par la coque haute)
module cable_notch() {
    zc_cable = split_z - cable_depth + cable_w/2;
    translate([cable_x, front_y - wall - 1, 0]) {
        translate([0,0,zc_cable]) rotate([-90,0,0]) cylinder(d=cable_w, h=wall+2, $fn=48);
        translate([-cable_w/2, 0, zc_cable]) cube([cable_w, wall+2, cable_depth]);
    }
}

module bottom_tray() {
    difference() {
        union() {
            difference() {
                linear_extrude(height=split_z) footprint_2d();
                translate([0,0,floor_t]) linear_extrude(height=split_z) offset(delta=-wall) footprint_2d();
            }
            for (p = screw_pos) translate([p[0], p[1], 0]) cylinder(r=pillar_r, h=split_z);
        }
        cable_notch();
        for (p = screw_pos) translate([p[0], p[1], 0]) {
            translate([0,0,-0.1]) cylinder(d=screw_clear_d, h=split_z+0.2);
            translate([0,0,-0.01]) cylinder(d1=screw_head_d, d2=screw_clear_d, h=(screw_head_d-screw_clear_d)/2);
        }
    }
    // colonnettes PMW3610 (carte en long selon Y, optique centrée sous la bille)
    for (dx=[-1,1]) for (dy=[-1,1])
        translate([dx*pmw_hole_dx/2, dy*pmw_hole_dy/2, floor_t])
            difference() { cylinder(d=6, h=standoff_h); translate([0,0,-0.1]) cylinder(d=pmw_hole_d, h=standoff_h+0.2); }
    // cadre RP2040-Zero (en travers, derrière le PMW3610 ; maintien friction + colle)
    translate([0, rp_cy, floor_t])
        difference() {
            translate([-(rp_h+2)/2, -(rp_w+2)/2, 0]) cube([rp_h+2, rp_w+2, 1.5]);
            translate([-rp_h/2, -rp_w/2, -0.1]) cube([rp_h, rp_w, 2]);
        }
}

// =============================================================================
// COQUE HAUTE : plaque chanfreinée + calotte + 3 murs-poteaux
// =============================================================================
module post_raw(az) {
    r_top  = R_cup + post_top_flat;
    r_foot = r_top + (ball_center_z - split_z);   // pente 45° prolongée jusqu'au plan de joint,
    rotate([0,0,az]) rotate([90,0,0])             // puis rognée par l'empreinte (+post_overhang)
        linear_extrude(height=post_w, center=true)
            polygon([[0,split_z],[r_foot,split_z],[r_top,ball_center_z],[0,ball_center_z]]);
}
module post_inner_cut(az) {
    translate([0,0,ball_center_z]) rotate([0,0,az]) rotate([90,0,0])
        cylinder(r=R_cup, h=post_w+2, center=true);
}
module brg_hole(i) {
    d = brg_dir(i);
    translate([0,0,ball_center_z])
        rotate([0, 0, atan2(d[1],d[0])]) rotate([0, 180-brg_polar, 0])
            cylinder(d=brg_hole_d, h=brg_bottom_r, $fn=32);
}
// languette + lèvre, dans le plan radial du poteau (x = distance à l'axe, y = z − z centre)
module lip(az) {
    r_out = R_cup + lip_finger_t;
    z_top = lip_h + lip_lead;
    translate([0,0,ball_center_z]) rotate([0,0,az]) rotate([90,0,0])
        linear_extrude(height=lip_finger_w, center=true)
            offset(r=lip_round) offset(delta=-lip_round)
                polygon([[R_cup-0.01,-1], [r_out,-1], [r_out,z_top],
                         [lip_r_tip+lip_lead, z_top], [lip_r_tip, lip_h], [R_cup-0.01, 0]]);
}

module plate() {
    translate([0,0,split_z]) linear_extrude(height=plate_edge_v) footprint_2d();
    hull() {
        translate([0,0,split_z+plate_edge_v]) linear_extrude(height=0.01) footprint_2d();
        translate([0,0,plate_top_z-0.01]) linear_extrude(height=0.01) offset(delta=-plate_chamfer) footprint_2d();
    }
}

module top_body() {
    difference() {
        union() {
            plate();
            intersection() {
                union() for (i=[0:2]) difference() { post_raw(brg_az[i]); post_inner_cut(brg_az[i]); }
                translate([0,0,split_z]) linear_extrude(height=100) offset(r=post_overhang) footprint_2d();
            }
        }
        translate([0,0,ball_center_z]) sphere(r=R_cup);                 // calotte
        translate([0,0,split_z-1]) cylinder(d=aperture_d, h=plate_t+2); // ouverture optique
        for (i=[0:2]) brg_hole(i);                                      // logements Ø2.6
        for (k=[0:3]) translate([screw_pos[k][0], screw_pos[k][1], split_z-0.1])
            cylinder(d=screw_pilot_d, h=(k<2 ? pilot_depth_front : pilot_depth_back)+0.1, $fn=24);
    }
    for (i=[0:2]) lip(brg_az[i]);   // ajoutées après la calotte (le nez entre dans R_cup)
}

// =============================================================================
// Références (visualisation uniquement)
// =============================================================================
module ball_ref() {
    color([0.2,0.6,0.9,0.35]) translate([0,0,ball_center_z]) sphere(d=ball_d);
    color([0.25,0.25,0.28]) for (i=[0:2]) translate(brg_center(i)) sphere(d=brg_ball_d, $fn=24);
}

module assembly() {
    color([0.75,0.77,0.8]) bottom_tray();
    color([0.9,0.85,0.6])  top_body();
    if (SHOW_BALL) ball_ref();
}

module section_cut_h(z) { difference() { assembly(); translate([-200,-200,z]) cube([400,400,200]); } }
module section_cut_v(x) { difference() { assembly(); translate([x,-200,-50]) cube([200,400,200]); } }

SECTION_Z = brg_center(0)[2];   // plan des billes de roulement
SECTION_X = 0;                  // plan vertical du poteau arrière (270°)

if      (RENDER == "top")       top_body();
else if (RENDER == "bottom")    bottom_tray();
else if (RENDER == "section_h") section_cut_h(SECTION_Z);
else if (RENDER == "section_v") section_cut_v(SECTION_X);
else                            assembly();
