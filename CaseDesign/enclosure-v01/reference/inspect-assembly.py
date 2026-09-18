import adsk.core, adsk.fusion, os, json, traceback
BASE=os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
def run(context):
    app=adsk.core.Application.get()
    try:
        d=adsk.fusion.Design.cast(app.activeProduct)
        root=d.rootComponent
        rows=[]
        for o in root.allOccurrences:
            for b in o.component.bRepBodies:
                bb=b.createForAssemblyContext(o).boundingBox
                rows.append({'component':o.fullPathName,'body':b.name,'min':[round(v*10,4) for v in bb.minPoint.asArray()],'max':[round(v*10,4) for v in bb.maxPoint.asArray()]})
        with open(os.path.join(BASE,'hardware-bounds.json'),'w') as f: json.dump(rows,f,indent=2)
        app.activeViewport.fit()
    except:
        with open(os.path.join(BASE,'build-error.txt'),'w') as f: f.write(traceback.format_exc())
        app.userInterface.messageBox(traceback.format_exc())
