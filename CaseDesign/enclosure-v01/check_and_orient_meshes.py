import struct, pathlib, collections, json
base=pathlib.Path(__file__).resolve().parent
out=base/'print';out.mkdir(exist_ok=True)
report=[]
for p in sorted(base.glob('*.stl')):
 data=p.read_bytes();count=struct.unpack_from('<I',data,80)[0]
 assert len(data)==84+50*count, p
 facets=[struct.unpack_from('<12fH',data,84+50*i) for i in range(count)]
 vertices=[tuple(f[j:j+3]) for f in facets for j in (3,6,9)]
 bounds=[[min(v[k] for v in vertices),max(v[k] for v in vertices)] for k in range(3)]
 def rotate(v):
  if p.name.startswith(('02','04')):return (v[0],-v[1],-v[2])
  if p.name.startswith('07'):return (v[2],v[1],-v[0])
  return tuple(v)
 rotated=[rotate(v) for v in vertices];minz=min(v[2] for v in rotated)
 edges=collections.Counter(); graph=collections.defaultdict(set)
 for f in facets:
  vs=[tuple(round(x,5) for x in f[j:j+3]) for j in (3,6,9)]
  for a,b in zip(vs,vs[1:]+vs[:1]):
   edges[tuple(sorted((a,b)))]+=1;graph[a].add(b);graph[b].add(a)
 todo=set(graph);components=0
 while todo:
  components+=1;stack=[todo.pop()]
  while stack:
   for v in graph[stack.pop()]:
    if v in todo:todo.remove(v);stack.append(v)
 bad=sum(n!=2 for n in edges.values())
 assert bad==0 and components==1,(p,bad,components)
 payload=bytearray(b'Bike Stem Computer V01 - mm - oriented for printing'.ljust(80,b' '))+struct.pack('<I',count)
 for f in facets:
  vals=list(rotate(f[:3]))
  for j in (3,6,9):
   v=rotate(f[j:j+3]);vals.extend((v[0],v[1],v[2]-minz))
  payload.extend(struct.pack('<12fH',*vals,0))
 (out/p.name).write_bytes(payload)
 report.append({'file':p.name,'triangles':count,'closed_manifold':bad==0,'connected_shells':components,'original_bounds_mm':bounds,'print_height_mm':max(v[2] for v in rotated)-minz})
(base/'mesh-validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
