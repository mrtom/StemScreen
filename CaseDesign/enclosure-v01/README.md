# Bike Stem Computer — V01 printable fit prototype

The editable master is **Bike_Stem_Computer_V01.f3d**. It contains the Fusion timeline, 54 named user parameters, separate components, and Waveshare's official STEP reference. The STEP export is for interchange; the F3D retains the editable feature history. Dimensions are millimetres.

## Size and construction

- Round body: **54.5 mm diameter × 22.5 mm high**; **28.8 mm** including the replaceable mount. The temporary side cap increases overall width to approximately 55.7 mm.
- The 35 × 30 mm rectangular battery, its corner clearance, and the sealing land determine the diameter.
- Top bezel: 33.2 mm viewing aperture, underside seat for a **38 mm diameter × 1 mm polycarbonate lens**, 0.25 mm radial lens clearance and 0.5 mm clearance above the LCD.
- Lower shell: flat 4.2 mm floor, battery rails with 0.7 mm side clearance, 0.5 mm soft pad allowance and 0.9 mm nominal clearance above the battery.
- Removable 1 mm carrier separates the cell from the electronics. Three posts support the PCB underside; three peripheral locators limit sideways movement. Three soft rim pad references occupy the gap between the inactive LCD rim and the lens. Use compliant material and avoid loading the active display.
- Three underside M2 screws capture the carrier and engage heat-set inserts in the upper shell. The case opens without disturbing the replaceable underside mount.
- Shell seam: continuous groove for a **50 mm ID × 1.5 mm cross-section silicone O-ring**, 1.8 mm wide and 1.2 mm deep (nominal 20% squeeze). This is a prototype gland, not a tested ingress rating.
- USB-C opening: **13 × 6 mm** cable tunnel aligned to the official USB connector model. A **16.6 × 8 mm** outer recess reserves room for a future TPU plug flange. The plug itself is not included.
- Power control: generic **6 mm side opening**, plus a separate blanking cap. It is a mechanical provision, not an installed switch or power circuit. Select the switch before finalising its internal clearance and retention. BOOT and RESET remain accessible when the case is opened; neither is treated as a power switch.
- Mount: separate male quarter-turn fit-test cartridge, fastened with two M2 screws on a custom **24 mm pitch** into blind holes in the base. The flange/neck/ears are parameterised. Its ear dimensions are provisional and are not a published Garmin specification. Test insertion, turning, retention and clearance on the intended female mount; alter or replace only this component as needed.

## Printing

Use the five STL files in **print/**. They have been rotated onto their intended printing faces and placed at Z=0. The STL files in the main folder retain assembly coordinates.

| Part | Suggested first print | Orientation / support |
|---|---|---|
| 01 Bottom shell | PETG, 0.2 mm layers, 4 perimeters | Flat underside on bed; cavity up |
| 02 Top shell and bezel | PETG, 0.2 mm layers, 4 perimeters | Bezel face on bed; inspect USB bridge in slicer |
| 03 Removable electronics carrier | PETG, 0.2 mm layers | Flat separator face on bed; posts up |
| 04 Replaceable quarter turn cartridge | PETG fit coupon first, dense/solid | Flange on bed; local support under projecting ears if required |
| 07 Switch blanking cap | TPU fit trial | Flange on bed; stem up |

Use printer-specific compensation only after measuring a fit print. Keep supports off sealing faces, insert bores and the lens seat where possible. The short carrier locators are delicate; remove the carrier by its broad tabs. The polycarbonate lens and silicone O-ring are purchased/cut parts, not ordinary opaque prints.

## Assembly items and sequence

1. Dry-fit the empty shells, carrier, USB cable and mount cartridge. Confirm the actual battery pack including protection board, pouch seams, connector and cable fits without bending or compression. Pi Hut's published size is described as the cell size.
2. Fit three M2 heat-set inserts into the top shell and two into the base. **3.2 mm bore × 3.2 mm depth is a starting allowance**: match the actual insert supplier and test in spare material before installation.
3. Bond the clear lens into the underside bezel seat using a continuous compatible flexible seal. The model includes the cover but no adhesive solid. Protect the optical area from adhesive.
4. Use a thin soft pad to hold the battery on the floor. Route its cable around the cell and through the carrier's rear-left relief. Keep leads away from screw bosses and the O-ring.
5. Place the board on the carrier's three supports. Use small removable compliant pads to prevent rattling; confirm they bear only on clear PCB / inactive display rim regions. The reference geometry is not a substitute for inspecting the delivered board revision.
6. Install the O-ring, carrier and lid. Start with three **M2 × 12 mm** case screws and thin sealing washers under the heads; check actual engagement and shorten if necessary. Do not force the screws into a blind bore. Recesses are 4.2 mm diameter × 1.8 mm deep.
7. Fit the mount cartridge using two **M2 × 4 mm** screws. Check head fit and remaining insert depth. The 1.5 mm head recesses suit low-profile heads; change the recess parameter for the selected hardware.
8. Temporarily seal the blanking cap with removable compatible sealant. It is not a validated press-fit seal. Make the TPU USB plug and selected switch sealing arrangement before rain use. Seal the case-screw paths with the under-head washers/sealant as required.

First evaluate fit and sealing without electronics. This version has no claimed IP rating and is not yet validated for riding. The mount should pass physical retention testing before carrying the computer on a bike.

## Editing

Use Fusion's Change Parameters. The principal groups are `battery_*`, `pcb_*`, `case_*`, `lens_*`, `seal_*`, `usb_*`, `switch_*`, `insert_*` and `mount_*`. The enclosure geometry and Z stack use expressions. The official imported Waveshare assembly is a static reference placed at **screen_z = 20 mm**: move that reference to the new `screen_z` after changing the vertical stack. The nominal battery and lens reference bodies update parametrically.

Changing the battery footprint substantially also requires checking `cavity_diameter`, screw positions, battery rails and seal dimensions. Parameters support iterative changes, not arbitrary combinations. The seal centre is intentionally independently adjustable for a purchased O-ring.

## Verification performed

- Native Fusion features recompute without timeline errors or warnings.
- Every printable component contains one solid body.
- Pairwise solid-intersection checks found no overlaps between the printable parts, cover lens, nominal battery, or official assembled Waveshare geometry.
- `pcb_clearance` changed from 0.4 to 0.6 mm and restored: no timeline errors or warnings.
- All five exported STL meshes are closed two-manifold surfaces with one connected shell. Their millimetre dimensions and print heights were checked.
- These checks do not establish print tolerance, switch suitability, fastener strength, mount engagement, water resistance or battery-pack manufacturing tolerances.

## Dimension sources

- [Waveshare ESP32-S3-LCD-1.28 documentation](https://docs.waveshare.com/ESP32-S3-LCD-1.28): 32.4 mm active display and non-touch board identification.
- [Waveshare official mechanical package](https://files.waveshare.com/wiki/ESP32-S3-LCD-1.28/ESP32-S3-LCD-1.28.zip): 36.5 mm PCB width, 39.5 mm overall length; imported STEP gives 7.9 mm full rear depth and USB location. Local originals are under reference/.
- [Pi Hut 500 mAh PicoBlade battery, SKU 106602](https://thepihut.com/products/500mah-3-7v-lipo-battery-1-25mm-picoblade-connector): 35 × 30 × 5 mm nominal cell, 3.7 V, approximately 100 mm lead.

The enclosure and mount geometry are original prototype designs. The official Waveshare model is included as an attributed hardware reference.
