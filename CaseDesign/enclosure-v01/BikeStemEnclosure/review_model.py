import adsk.core as C, adsk.fusion as F, os, json, traceback
BASE=os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
def run(context):
    app=C.Application.get();d=F.Design.cast(app.activeProduct);root=d.rootComponent
    try:
        occs=list(root.occurrences)
        parts=[o for o in occs if o.component.name[:2] in ['01','02','03','04','05','06','07']]
        hw=next(o for o in occs if o.component.name.startswith('08'))
        # Pairwise interference using temporary BRep copies: no design edits.
        bodies=[]
        for o in parts:
            for b in o.component.bRepBodies:bodies.append((o.component.name,b.createForAssemblyContext(o)))
        hw_bodies=[]
        for o in root.allOccurrences:
            if o.fullPathName.startswith(hw.fullPathName):
                for b in o.component.bRepBodies:hw_bodies.append((o.fullPathName,b.createForAssemblyContext(o)))
        mgr=F.TemporaryBRepManager.get();issues=[];pairs=0
        candidates=[]
        for i,(n,a) in enumerate(bodies):
            for m,b in bodies[i+1:]:candidates.append((n,a,m,b))
            if n[:2] in ['01','02','03','04','05','07']:
                for m,b in hw_bodies:candidates.append((n,a,m,b))
        for n,a,m,b in candidates:
            aa=a.boundingBox;bb=b.boundingBox
            if any(min(aa.maxPoint.asArray()[k],bb.maxPoint.asArray()[k])-max(aa.minPoint.asArray()[k],bb.minPoint.asArray()[k])<0.00001 for k in range(3)):continue
            pairs+=1;t=mgr.copy(a)
            if mgr.booleanOperation(t,b,F.BooleanTypes.IntersectionBooleanType) and t.volume>0.00000001:
                issues.append({'part1':n,'part2':m,'overlap_mm3':t.volume*1000})
        p=d.userParameters.itemByName('pcb_clearance');old=p.expression;p.expression='0.6 mm';d.computeAll()
        param_issues=[]
        for t in d.timeline:
            e=t.entity
            if hasattr(e,'healthState') and e.healthState!=F.FeatureHealthStates.HealthyFeatureHealthState:param_issues.append({'name':getattr(e,'name',''),'message':getattr(e,'errorOrWarningMessage','')})
        p.expression=old;d.computeAll()
        report={'interference_candidates':len(candidates),'boolean_tests':pairs,'overlaps':issues,'parameter_test':{'parameter':'pcb_clearance','from':old,'to':'0.6 mm','restored':True,'issues':param_issues},'bodies':[{'name':n,'min_mm':[v*10 for v in b.boundingBox.minPoint.asArray()],'max_mm':[v*10 for v in b.boundingBox.maxPoint.asArray()]} for n,b in bodies]}
        with open(os.path.join(BASE,'clearance-check.json'),'w') as f:json.dump(report,f,indent=2)
        # Hardware reference position and editable notes saved inside the design.
        root.attributes.add('BikeStemV01','ReferenceNotes','Waveshare official STEP is placed at screen_z=20 mm. Reposition this reference after changing the vertical stack. Printed enclosure features are expression-driven. Garmin ears are provisional fit-test geometry; no sealing or road validation claimed.')
        em=d.exportManager;em.execute(em.createFusionArchiveExportOptions(os.path.join(BASE,'Bike_Stem_Computer_V01.f3d')))
    except:
        with open(os.path.join(BASE,'review-error.txt'),'w') as f:f.write(traceback.format_exc())
        app.userInterface.messageBox(traceback.format_exc())
