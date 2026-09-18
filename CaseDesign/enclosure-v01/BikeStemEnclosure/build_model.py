"""Native Fusion feature model; named dimensions are millimetres, API lengths cm."""
import adsk.core as C, adsk.fusion as F, os, json, traceback
BASE=os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
P=C.Point3D.create
V=C.ValueInput.createByString
NEW=F.FeatureOperations.NewBodyFeatureOperation
JOIN=F.FeatureOperations.JoinFeatureOperation
CUT=F.FeatureOperations.CutFeatureOperation
D=None

def ev(s): return D.unitsManager.evaluateExpression(str(s),'mm')
def pt(x,y,z='0 mm'): return P(ev(x),ev(y),ev(z))
def comp(name):
    o=D.rootComponent.occurrences.addNewComponent(C.Matrix3D.create());o.component.name=name
    return o.component

def sketch(c,name,z='0 mm',axis='xy'):
    plane={'xy':c.xYConstructionPlane,'xz':c.xZConstructionPlane,'yz':c.yZConstructionPlane}[axis]
    if abs(ev(z))>1e-8:
        i=c.constructionPlanes.createInput();i.setByOffset(plane,V(z));plane=c.constructionPlanes.add(i);plane.name=name+' datum'
    s=c.sketches.add(plane);s.name=name;return s

def locate(s,p,x,y):
    g=s.geometricConstraints; dims=s.sketchDimensions;o=s.originPoint
    if abs(ev(x))<1e-8 and abs(ev(y))<1e-8:
        g.addCoincident(p,o);return
    if abs(ev(x))<1e-8:g.addVerticalPoints(o,p)
    else:dims.addDistanceDimension(o,p,F.DimensionOrientations.HorizontalDimensionOrientation,pt(x,y)).parameter.expression='abs('+x+')'
    if abs(ev(y))<1e-8:g.addHorizontalPoints(o,p)
    else:dims.addDistanceDimension(o,p,F.DimensionOrientations.VerticalDimensionOrientation,pt(x,y)).parameter.expression='abs('+y+')'

def circle(s,dia,x='0 mm',y='0 mm'):
    e=s.sketchCurves.sketchCircles.addByCenterRadius(pt(x,y),ev(dia)/2)
    locate(s,e.centerSketchPoint,x,y)
    s.sketchDimensions.addDiameterDimension(e,P(ev(x)+ev(dia)*0.65,ev(y)+0.3,0)).parameter.expression=dia

def rect(s,x,y,w,h):
    ls=s.sketchCurves.sketchLines.addTwoPointRectangle(pt(x,y),pt('('+x+')+('+w+')','('+y+')+('+h+')'))
    locate(s,ls.item(0).startSketchPoint,x,y)
    s.sketchDimensions.addDistanceDimension(ls.item(0).startSketchPoint,ls.item(0).endSketchPoint,F.DimensionOrientations.HorizontalDimensionOrientation,pt(x,y)).parameter.expression=w
    s.sketchDimensions.addDistanceDimension(ls.item(1).startSketchPoint,ls.item(1).endSketchPoint,F.DimensionOrientations.VerticalDimensionOrientation,pt(x,y)).parameter.expression=h

def extr(c,s,depth,op,name,annulus=False):
    if annulus: profile=next(p for p in s.profiles if p.profileLoops.count==2)
    else:
        profile=C.ObjectCollection.create()
        for p in s.profiles:profile.add(p)
    i=c.features.extrudeFeatures.createInput(profile,op);i.setDistanceExtent(False,V(depth))
    f=c.features.extrudeFeatures.add(i);f.name=name;s.isVisible=False
    return f

def disk(c,name,dia,z,depth,op=NEW,x='0 mm',y='0 mm'):
    s=sketch(c,name,z);circle(s,dia,x,y);return extr(c,s,depth,op,name)
def box(c,name,x,y,w,h,z,depth,op):
    s=sketch(c,name,z);rect(s,x,y,w,h);return extr(c,s,depth,op,name)
def ring(c,name,od,id,z,depth,op):
    s=sketch(c,name,z);circle(s,od);circle(s,id);return extr(c,s,depth,op,name,True)
def appearance(c,name,r,g,b):
    lib=C.Application.get().materialLibraries.itemByName('Fusion 360 Appearance Library')
    a=D.appearances.addByCopy(lib.appearances.itemByName('Plastic - Matte (White)'),name)
    p=a.appearanceProperties.itemById('opaque_albedo')
    if p:p.value=C.Color.create(r,g,b,255)
    for body in c.bRepBodies:body.appearance=a

def log(t):
    with open(os.path.join(BASE,'build-progress.txt'),'a') as f:f.write(t+'\n')

def run(context):
    global D
    app=C.Application.get()
    try:
        with open(os.path.join(BASE,'build-progress.txt'),'w') as f:f.write('Starting native Fusion build\n')
        doc=app.documents.add(C.DocumentTypes.FusionDesignDocumentType)
        D=F.Design.cast(app.activeProduct);D.designType=F.DesignTypes.ParametricDesignType
        D.designIntent=F.DesignIntentTypes.HybridDesignIntentType
        doc.name='Bike Stem Computer - V01 fit prototype'
        pars=[
        ('pcb_diameter','36.5 mm','Waveshare official drawing: round PCB area'),
        ('pcb_length','39.5 mm','Official drawing overall length including USB tongue'),
        ('pcb_clearance','0.4 mm','Radial clearance; verify printed fit'),
        ('board_depth','7.9 mm','Official STEP: display front to lowest header'),
        ('pcb_back_offset','3.4 mm','Official STEP: screen face to PCB underside'),
        ('battery_width','30 mm','Pi Hut PKCell SKU106602'),
        ('battery_length','35 mm','Pi Hut PKCell SKU106602'),
        ('battery_thickness','5 mm','Nominal cell thickness; check actual pouch'),
        ('battery_xy_clearance','0.7 mm','Per side; keep cell uncompressed'),
        ('battery_pad','0.5 mm','Soft adhesive pad under cell'),
        ('battery_headroom','0.9 mm','Free space above cell below carrier'),
        ('cavity_diameter','48.5 mm','Fits battery diagonal, leads and service bosses'),
        ('wall_thickness','3 mm','Wide enough for seal gland'),
        ('case_diameter','cavity_diameter+2*wall_thickness','Derived outside diameter'),
        ('floor_thickness','4.2 mm','Flat printable floor with blind mount insert depth'),
        ('seam_z','floor_thickness+battery_pad+battery_thickness+battery_headroom','Case split and carrier seat'),
        ('carrier_thickness','1 mm','Removable insulating separator'),
        ('header_air_gap','0.5 mm','Separator to lowest header'),
        ('screen_z','seam_z+carrier_thickness+header_air_gap+board_depth','Display front datum'),
        ('pcb_back_z','screen_z-pcb_back_offset','PCB support datum'),
        ('lens_air_gap','0.5 mm','Clear cover separation from LCD'),
        ('lens_thickness','1 mm','Polycarbonate sheet'),
        ('lens_diameter','38 mm','Cut lens outside diameter'),
        ('lens_fit','0.25 mm','Radial installation allowance'),
        ('bezel_thickness','1 mm','Lip over cover lens'),
        ('window_diameter','33.2 mm','Clears 32.4 mm display active area'),
        ('case_height','screen_z+lens_air_gap+lens_thickness+bezel_thickness','Excludes mount'),
        ('seal_id','50 mm','Silicone O ring nominal ID'),
        ('seal_cs','1.5 mm','Silicone O ring cross section'),
        ('seal_groove_width','1.8 mm','Approx 82 percent gland fill'),
        ('seal_groove_depth','1.2 mm','20 percent nominal squeeze'),
        ('screw_radius','22 mm','Three screw axes: E W N'),
        ('boss_diameter','5.6 mm','Service screw boss OD'),
        ('m2_clearance','2.3 mm','M2 screw clearance'),
        ('insert_bore','3.2 mm','Provisional M2 heat set pilot; match insert supplier'),
        ('insert_depth','3.2 mm','Provisional M2 insert length'),
        ('screw_head_diameter','4.2 mm','M2 socket screw counterbore'),
        ('screw_head_depth','1.8 mm','Underside case screw recess'),
        ('usb_width','13 mm','Cable overmould tunnel width; fit-check chosen cable'),
        ('usb_height','6 mm','Cable tunnel height'),
        ('usb_center_z','screen_z-4.52 mm','From official STEP USB shell centre'),
        ('usb_plug_width','16.6 mm','Future TPU plug flange pocket'),
        ('usb_plug_height','8 mm','Future TPU plug flange pocket'),
        ('switch_bore','6 mm','Panel switch provision; capped until selected'),
        ('switch_z','6 mm','Side control centre height'),
        ('mount_plate_diameter','32 mm','Replaceable cartridge flange'),
        ('mount_pad_height','0 mm','Mount datum; zero keeps lower shell flat for printing'),
        ('mount_plate_thickness','2.5 mm','Replaceable flange thickness'),
        ('mount_screw_pitch','24 mm','Custom cartridge fasteners, not a Garmin standard'),
        ('mount_neck_diameter','18.5 mm','Provisional male quarter-turn fit'),
        ('mount_neck_height','2 mm','Gap above ears'),
        ('mount_tab_span','24.4 mm','Provisional ear tip diameter; physical fit test required'),
        ('mount_tab_width','9 mm','Provisional opposing ear width'),
        ('mount_tab_thickness','1.8 mm','Provisional engagement ear thickness')]
        for a,b,c in pars:D.userParameters.add(a,V(b),'mm',c)
        axes=[('screw_radius','0 mm'),('-screw_radius','0 mm'),('0 mm','screw_radius')]
        bottom=comp('01 Bottom shell - PETG')
        disk(bottom,'Outer lower shell','case_diameter','0 mm','seam_z')
        disk(bottom,'Battery and wiring cavity','cavity_diameter','floor_thickness','seam_z-floor_thickness',CUT)
        for n,(x,y) in enumerate(axes):
            disk(bottom,'Case boss '+str(n+1),'boss_diameter','floor_thickness','seam_z-floor_thickness',JOIN,x,y)
            disk(bottom,'Case screw '+str(n+1),'m2_clearance','0 mm','seam_z',CUT,x,y)
            disk(bottom,'Recessed screw head '+str(n+1),'screw_head_diameter','0 mm','screw_head_depth',CUT,x,y)
        ring(bottom,'Silicone O ring gland','seal_id+seal_cs+seal_groove_width','seal_id+seal_cs-seal_groove_width','seam_z-seal_groove_depth','seal_groove_depth',CUT)
        for sign in ['','-']:
            disk(bottom,'Blind mount insert '+sign,'insert_bore','-mount_pad_height','insert_depth',CUT,'0 mm',sign+'mount_screw_pitch/2')
        for sign in [1,-1]:
            x='battery_width/2+battery_xy_clearance' if sign==1 else '-battery_width/2-battery_xy_clearance-1 mm'
            box(bottom,'Battery side rail '+str(sign),x,'-battery_length/2','1 mm','battery_length','floor_thickness','1.3 mm',JOIN)
        s=sketch(bottom,'Side power provision','cavity_diameter/2-0.1 mm','yz');circle(s,'switch_bore','-switch_z','0 mm');extr(bottom,s,'wall_thickness+1 mm',CUT,'Panel switch bore')
        log('Bottom completed')
        top=comp('02 Top shell and bezel - PETG')
        disk(top,'Outer upper shell','case_diameter','seam_z','case_height-seam_z')
        disk(top,'Display and board chamber','cavity_diameter','seam_z','screen_z+lens_air_gap-seam_z',CUT)
        disk(top,'Display aperture','window_diameter','screen_z+lens_air_gap','lens_thickness+bezel_thickness',CUT)
        disk(top,'Underside bonded lens seat','lens_diameter+2*lens_fit','screen_z+lens_air_gap','lens_thickness',CUT)
        for n,(x,y) in enumerate(axes):
            disk(top,'Upper insert boss '+str(n+1),'boss_diameter','seam_z+carrier_thickness','screen_z+lens_air_gap-seam_z-carrier_thickness',JOIN,x,y)
            disk(top,'M2 heat set bore '+str(n+1),'insert_bore','seam_z+carrier_thickness','insert_depth',CUT,x,y)
        box(top,'USB cable tunnel','-usb_width/2','-case_diameter/2-1 mm','usb_width','10 mm','usb_center_z-usb_height/2','usb_height',CUT)
        box(top,'Future TPU plug flange pocket','-usb_plug_width/2','-case_diameter/2-1 mm','usb_plug_width','2.5 mm','usb_center_z-usb_plug_height/2','usb_plug_height',CUT)
        log('Top completed')
        carrier=comp('03 Removable electronics carrier - PETG')
        disk(carrier,'Battery separator deck','pcb_diameter+2*pcb_clearance+2.2 mm','seam_z','carrier_thickness')
        for n,(x,y) in enumerate(axes):
            if n<2:
                xx='18 mm' if n==0 else '-screw_radius'
                box(carrier,'Carrier ear '+str(n),xx,'-boss_diameter/2','screw_radius-18 mm','boss_diameter','seam_z','carrier_thickness',JOIN)
            else:box(carrier,'Carrier north ear','-boss_diameter/2','18 mm','boss_diameter','screw_radius-18 mm','seam_z','carrier_thickness',JOIN)
            disk(carrier,'Carrier seat '+str(n),'4.4 mm','seam_z','carrier_thickness',JOIN,x,y)
            disk(carrier,'Carrier screw passage '+str(n),'m2_clearance','seam_z','carrier_thickness',CUT,x,y)
        supports=[('pcb_diameter/2-0.75 mm','0 mm'),('-pcb_diameter/2+0.75 mm','0 mm'),('0 mm','pcb_diameter/2-0.25 mm')]
        for n,(x,y) in enumerate(supports):
            disk(carrier,'PCB edge support '+str(n),'2 mm','seam_z+carrier_thickness','pcb_back_z-seam_z-carrier_thickness',JOIN,x,y)
        for n,(x,y) in enumerate([('(pcb_diameter/2+pcb_clearance+0.7 mm)/sqrt(2)','(pcb_diameter/2+pcb_clearance+0.7 mm)/sqrt(2)'),('-(pcb_diameter/2+pcb_clearance+0.7 mm)/sqrt(2)','(pcb_diameter/2+pcb_clearance+0.7 mm)/sqrt(2)'),('-(pcb_diameter/2+pcb_clearance+0.7 mm)/sqrt(2)','-(pcb_diameter/2+pcb_clearance+0.7 mm)/sqrt(2)')]):
            disk(carrier,'PCB radial locator '+str(n),'1.4 mm','seam_z+carrier_thickness','pcb_back_z+1.6 mm-seam_z-carrier_thickness',JOIN,x,y)
        box(carrier,'Battery lead route','-11 mm','-21 mm','7 mm','7 mm','seam_z','carrier_thickness',CUT)
        log('Carrier completed')
        mount=comp('04 Replaceable quarter turn cartridge - FIT TEST')
        disk(mount,'Cartridge flange','mount_plate_diameter','-mount_pad_height-mount_plate_thickness','mount_plate_thickness')
        disk(mount,'Quarter turn neck','mount_neck_diameter','-mount_pad_height-mount_plate_thickness-mount_neck_height','mount_neck_height',JOIN)
        tab_z='-mount_pad_height-mount_plate_thickness-mount_neck_height-mount_tab_thickness'
        disk(mount,'Quarter turn engagement ears','mount_tab_span',tab_z,'mount_tab_thickness',JOIN)
        box(mount,'Ear north trim','-mount_tab_span/2-1 mm','mount_tab_width/2','mount_tab_span+2 mm','mount_tab_span',tab_z,'mount_tab_thickness',CUT)
        box(mount,'Ear south trim','-mount_tab_span/2-1 mm','-mount_tab_span-mount_tab_width/2','mount_tab_span+2 mm','mount_tab_span',tab_z,'mount_tab_thickness',CUT)
        for sign in ['','-']:
            disk(mount,'Cartridge fixing '+sign,'m2_clearance','-mount_pad_height-mount_plate_thickness','mount_plate_thickness',CUT,'0 mm',sign+'mount_screw_pitch/2')
            disk(mount,'Cartridge head recess '+sign,'screw_head_diameter','-mount_pad_height-mount_plate_thickness','1.5 mm',CUT,'0 mm',sign+'mount_screw_pitch/2')
        log('Mount completed')
        lens=comp('05 Cover lens - polycarbonate REFERENCE')
        disk(lens,'Polycarbonate optical cover','lens_diameter','screen_z+lens_air_gap','lens_thickness')
        battery=comp('06 Battery - 500 mAh REFERENCE')
        box(battery,'Nominal cell envelope','-battery_width/2','-battery_length/2','battery_width','battery_length','floor_thickness+battery_pad','battery_thickness',NEW)
        cap=comp('07 Switch blanking cap - TPU fit prototype')
        s=sketch(cap,'Cap stem','cavity_diameter/2','yz');circle(s,'switch_bore-0.2 mm','-switch_z','0 mm');extr(cap,s,'wall_thickness',NEW,'Cap stem')
        s=sketch(cap,'Cap flange','case_diameter/2','yz');circle(s,'switch_bore+3 mm','-switch_z','0 mm');extr(cap,s,'1.2 mm',JOIN,'Cap flange')
        foam=comp('09 LCD rim pads - soft foam REFERENCE')
        for n,(x,y) in enumerate([('17.1 mm','0 mm'),('-17.1 mm','0 mm'),('0 mm','17.1 mm')]):
            disk(foam,'Soft rim pad '+str(n),'0.8 mm','screen_z','lens_air_gap',NEW,x,y)
        hardware=comp('08 Waveshare official STEP - REFERENCE')
        opt=app.importManager.createSTEPImportOptions(os.path.join(BASE,'reference/ESP32-S3-LCD-1.28/ESP32-S3-LCD-1_28.stp'))
        app.importManager.importToTarget(opt,hardware)
        hw_occ=next(o for o in D.rootComponent.occurrences if o.component==hardware)
        mat=C.Matrix3D.create();mat.translation=C.Vector3D.create(0,0,ev('screen_z'));hw_occ.transform2=mat
        if D.snapshots.hasPendingSnapshot:D.snapshots.add()
        for c,n,r,g,b in [(bottom,'Graphite lower',48,55,63),(top,'Graphite bezel',65,73,81),(carrier,'Carrier orange',225,125,44),(mount,'Mount charcoal',35,39,44),(battery,'Battery silver',170,182,196),(cap,'TPU black',26,29,33)]:
            try:appearance(c,n,r,g,b)
            except:pass
        for c in [bottom,top,carrier,mount,lens,battery,cap]:
            for i,b in enumerate(c.bRepBodies):b.name=c.name.split(' - ')[0]+' solid '+str(i+1)
            c.isConstructionFolderLightBulbOn=False;c.isSketchFolderLightBulbOn=False
        for o in D.rootComponent.occurrences:
            if o.component==lens:o.isLightBulbOn=False
        D.rootComponent.isOriginFolderLightBulbOn=False
        for c,z,r in [(bottom,'0 mm','0.8 mm'),(top,'case_height','1.5 mm')]:
            edges=C.ObjectCollection.create()
            for e in c.bRepBodies.item(0).edges:
                g=C.Circle3D.cast(e.geometry)
                if g and abs(g.radius-ev('case_diameter')/2)<1e-5 and abs(g.center.z-ev(z))<1e-5:edges.add(e)
            if edges.count:
                fi=c.features.filletFeatures.createInput();fi.edgeSetInputs.addConstantRadiusEdgeSet(edges,V(r),False)
                c.features.filletFeatures.add(fi).name='Soft outside edge'
        D.computeAll();log('Validating and exporting')
        report={'parameters':[{'name':p.name,'expression':p.expression,'mm':round(p.value*10,4),'comment':p.comment} for p in D.userParameters], 'parts':[], 'timeline_issues':[]}
        for c in [bottom,top,carrier,mount,lens,battery,cap]:
            report['parts'].append({'name':c.name,'body_count':c.bRepBodies.count,'bodies':[{'name':b.name,'solid':b.isSolid,'volume_cm3':b.volume} for b in c.bRepBodies]})
        for t in D.timeline:
            e=t.entity
            if hasattr(e,'healthState') and e.healthState!=F.FeatureHealthStates.HealthyFeatureHealthState:report['timeline_issues'].append({'name':getattr(e,'name',''),'message':getattr(e,'errorOrWarningMessage','')})
        with open(os.path.join(BASE,'validation.json'),'w') as f:json.dump(report,f,indent=2)
        em=D.exportManager
        for c in [bottom,top,carrier,mount,cap]:
            path=os.path.join(BASE,c.name.split(' - ')[0].replace(' ','_')+'.stl')
            o=em.createSTLExportOptions(c,path);o.meshRefinement=F.MeshRefinementSettings.MeshRefinementHigh;em.execute(o)
        cam=app.activeViewport.camera;cam.eye=P(8,-10,10);cam.target=P(0,0,1);cam.upVector=C.Vector3D.create(0,0,1);cam.isFitView=True;app.activeViewport.camera=cam;app.activeViewport.fit()
        em.execute(em.createFusionArchiveExportOptions(os.path.join(BASE,'Bike_Stem_Computer_V01.f3d')))
        em.execute(em.createSTEPExportOptions(os.path.join(BASE,'Bike_Stem_Computer_V01.step')))
        log('SUCCESS - local F3D, STEP and print meshes exported')
    except:
        err=traceback.format_exc();log(err)
        with open(os.path.join(BASE,'build-error.txt'),'w') as f:f.write(err)
        app.userInterface.messageBox(err)
