import adsk.core as C, adsk.fusion as F, os, json, traceback
BASE=os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
def run(context):
    app=C.Application.get()
    try:
        old=[doc for doc in app.documents if doc.name.startswith('Bike Stem Computer - V01 fit prototype')]
        doc=app.importManager.importToNewDocument(app.importManager.createFusionArchiveImportOptions(os.path.join(BASE,'Bike_Stem_Computer_V01.f3d')))
        doc.name='Bike Stem Computer V01 - local archive verified'
        d=F.Design.cast(app.activeProduct);root=d.rootComponent
        check={'reopened_local_archive':True,'parameters':d.userParameters.count,'timeline_items':d.timeline.count,'components':[o.component.name for o in root.occurrences],'issues':[]}
        for t in d.timeline:
            e=t.entity
            if hasattr(e,'healthState') and e.healthState!=F.FeatureHealthStates.HealthyFeatureHealthState:check['issues'].append(getattr(e,'errorOrWarningMessage',''))
        with open(os.path.join(BASE,'archive-verification.json'),'w') as f:json.dump(check,f,indent=2)
        # Close only the intermediate generated enclosure documents after archive verification.
        if check['parameters']==54 and not check['issues']:
            for x in old:x.close(False)
        doc.activate()
        vp=app.activeViewport;cam=vp.camera
        cam.eye=C.Point3D.create(8,-10,10);cam.target=C.Point3D.create(0,0,1);cam.upVector=C.Vector3D.create(0,0,1);cam.isFitView=True;vp.camera=cam;vp.fit()
        vp.saveAsImageFile(os.path.join(BASE,'assembled-preview.png'),1600,1200)
        # Internal inspection image with the upper components hidden.
        saved=[]
        for o in root.occurrences:
            saved.append((o,o.isLightBulbOn))
            o.isLightBulbOn=o.component.name[:2] in ['01','03','06']
        vp.fit();vp.saveAsImageFile(os.path.join(BASE,'internal-preview.png'),1600,1200)
        for o,v in saved:o.isLightBulbOn=v
        # Underside image shows replaceable attachment and three shell fasteners.
        cam=vp.camera;cam.eye=C.Point3D.create(7,-9,-8);cam.target=C.Point3D.create(0,0,0.7);cam.upVector=C.Vector3D.create(0,0,1);cam.isFitView=True;vp.camera=cam;vp.fit()
        vp.saveAsImageFile(os.path.join(BASE,'underside-preview.png'),1600,1200)
        cam=vp.camera;cam.eye=C.Point3D.create(8,-10,10);cam.target=C.Point3D.create(0,0,1);cam.upVector=C.Vector3D.create(0,0,1);cam.isFitView=True;vp.camera=cam;vp.fit()
    except:
        with open(os.path.join(BASE,'finalize-error.txt'),'w') as f:f.write(traceback.format_exc())
        app.userInterface.messageBox(traceback.format_exc())
