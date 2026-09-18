import adsk.core, adsk.fusion, os, json, traceback
BASE=os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
def run(context):
    app=adsk.core.Application.get()
    try:
        doc=app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
        d=adsk.fusion.Design.cast(app.activeProduct)
        root=d.rootComponent
        opt=app.importManager.createSTEPImportOptions(os.path.join(BASE,'reference/ESP32-S3-LCD-1.28/ESP32-S3-LCD-1_28.stp'))
        app.importManager.importToTarget(opt,root)
        rows=[]
        def walk(c):
            for b in c.bRepBodies:
                bb=b.boundingBox
                rows.append({'component':c.name,'body':b.name,'min':[v*10 for v in bb.minPoint.asArray()],'max':[v*10 for v in bb.maxPoint.asArray()]})
            for o in c.occurrences: walk(o.component)
        walk(root)
        with open(os.path.join(BASE,'hardware-bounds.json'),'w') as f: json.dump(rows,f,indent=2)
        app.activeViewport.fit()
    except:
        with open(os.path.join(BASE,'build-error.txt'),'w') as f: f.write(traceback.format_exc())
        app.userInterface.messageBox(traceback.format_exc())
