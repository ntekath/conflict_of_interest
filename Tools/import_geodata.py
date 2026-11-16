"""Deterministic offline AP02 import. Only --download accesses the network."""
import argparse,hashlib,io,json,math,re,urllib.request,zipfile,html
from pathlib import Path
import numpy as np
import shapefile,mapbox_earcut
from shapely.geometry import shape,box,Polygon,MultiPolygon,GeometryCollection,Point
from shapely.ops import transform,unary_union
from pyproj import Proj,Geod
ROOT=Path(__file__).resolve().parents[1];RAW=ROOT/'GeoData/Raw';OUT=ROOT/'GeoData/Generated'
HEADER=(ROOT/'Source/ConflictOfInterest/GeoTransform.h').read_text(encoding='utf-8-sig')
def constant(name):return float(re.search(r'\b'+name+r'\s*=\s*(-?[0-9.]+)',HEADER).group(1))
R=constant('RadiusKm');SCALE=constant('UnitsPerKm')
PROJ=Proj(proj='lcc',lat_0=constant('OriginLatitude'),lon_0=constant('OriginLongitude'),lat_1=constant('Parallel1'),lat_2=constant('Parallel2'),R=R*1000,units='m')
def project(lon,lat,z=None):
 east,north=PROJ(lon,lat);return np.asarray(north)*SCALE/1000,np.asarray(east)*SCALE/1000
def distance(a,b):
 la,lo=map(math.radians,a);lb,lob=map(math.radians,b)
 h=math.sin((lb-la)/2)**2+math.cos(la)*math.cos(lb)*math.sin((lob-lo)/2)**2
 return 2*R*math.atan2(math.sqrt(max(0,h)),math.sqrt(max(0,1-h)))
def dataset(name,kind,download):
 p=RAW/(name+'.zip');url=f'https://naciscdn.org/naturalearth/10m/{kind}/{name}.zip'
 if download:
  data=urllib.request.urlopen(url,timeout=120).read();archive=zipfile.ZipFile(io.BytesIO(data))
  assert archive.testzip() is None
  if p.exists() and p.read_bytes()!=data:raise ValueError('Upstream changed. Preserve cache and explicitly review a new source version.')
  p.write_bytes(data)
 if not p.exists():raise FileNotFoundError(str(p)+'; run with --download once')
 z=zipfile.ZipFile(p);assert z.testzip() is None
 def stream(ext):return io.BytesIO(z.read(name+ext))
 versions=[n for n in z.namelist() if n.endswith('VERSION.txt')]
 meta={'file':p.name,'url':url,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'version':z.read(versions[0]).decode().strip(),'retrieved':'2026-11-16'}
 return shapefile.Reader(shp=stream('.shp'),shx=stream('.shx'),dbf=stream('.dbf'),encoding='utf-8'),meta
def polygons(g):
 if g.is_empty:return []
 if isinstance(g,Polygon):return [g]
 if isinstance(g,(MultiPolygon,GeometryCollection)):return [p for part in g.geoms for p in polygons(part)]
 return []
def triangulate(g):
 vertices=[];triangles=[];rings=[];discarded=0
 for p in polygons(g):
  coords=[];ends=[]
  for ring in [p.exterior,*p.interiors]:
   coords.extend(list(ring.coords)[:-1]);ends.append(len(coords));rings.append(list(map(list,ring.coords)))
  xy=np.asarray(coords,dtype=np.float64);indices=mapbox_earcut.triangulate_float64(xy,np.array(ends,dtype=np.uint32));area=0
  for k in range(0,len(indices),3):
   a,b,c=map(int,indices[k:k+3]);u=xy[b]-xy[a];v=xy[c]-xy[a];cross=u[0]*v[1]-u[1]*v[0]
   if cross<0:b,c=c,b
   t=Polygon([xy[a],xy[b],xy[c]])
   if t.area<1e-8:discarded+=1;continue
   assert t.area>0 and t.difference(p).area<max(1e-6,t.area*1e-9),'Triangle crosses an island/hole boundary'
   area+=t.area;triangles.extend([a+len(vertices),b+len(vertices),c+len(vertices)])
  assert abs(area-p.area)<max(1e-5,p.area*1e-8),'Triangulation area mismatch'
  vertices.extend(xy.tolist())
 return {'vertices':vertices,'triangles':triangles,'rings':rings,'discarded_roundoff_triangles':discarded,'parts':len(polygons(g)),'holes':sum(len(p.interiors) for p in polygons(g))}
def halfplane(poly,p,q):
 # Keep the side nearer to p than q; deterministic clipping of the initial rectangle.
 n=np.array(q)-np.array(p);limit=(np.dot(q,q)-np.dot(p,p))/2.;points=list(poly.exterior.coords)[:-1];out=[]
 for a,b in zip(points,points[1:]+points[:1]):
  da=float(np.dot(n,a)-limit);db=float(np.dot(n,b)-limit)
  if da<=1e-8:out.append(a)
  if (da<0<db) or (db<0<da):out.append(tuple(np.array(a)+(np.array(b)-np.array(a))*da/(da-db)))
 return Polygon(out)
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--download',action='store_true');args=parser.parse_args()
 RAW.mkdir(parents=True,exist_ok=True);OUT.mkdir(parents=True,exist_ok=True)
 scenario=json.loads((ROOT/'GeoData/scenario.json').read_text(encoding='utf-8-sig'));clip=box(*scenario['bbox_wgs84'])
 countries,mc=dataset('ne_10m_admin_0_countries','cultural',args.download)
 lakes,ml=dataset('ne_10m_lakes','physical',args.download)
 cities,mp=dataset('ne_10m_populated_places','cultural',args.download)
 sources=[mc,ml,mp];lake_shapes={};lake_rows=[];great={'Lake Superior','Lake Michigan','Lake Huron','Lake Erie','Lake Ontario'}
 # Only natural lakes, never modern reservoirs masquerading as 1913 geography.
 for rec in lakes.iterShapeRecords():
  d=rec.record.as_dict();name=d.get('name');g=shape(rec.shape.__geo_interface__)
  if name not in great:continue
  assert g.is_valid;g=g.intersection(clip);assert not g.is_empty and g.is_valid
  lake_shapes[name]=g
 assert set(lake_shapes)==great
 water=unary_union(list(lake_shapes.values()))
 # Simplify water once, then cut that exact geometry out of both land and game regions.
 projected_water=transform(project,water).simplify(1.4,preserve_topology=True)
 projected_lakes=[]
 for name,g in sorted(lake_shapes.items()):
  pg=transform(project,g).simplify(1.4,preserve_topology=True);projected_lakes.append(pg)
  lake_rows.append({'id':'lake.'+name.replace('Lake ','').lower(),'name':name,'wgs84':g.__geo_interface__,**triangulate(pg)})
 projected_water=unary_union(projected_lakes)
 land={};country_rows=[]
 for rec in countries.iterShapeRecords():
  d=rec.record.as_dict();iso=d['ADM0_A3']
  if iso not in ['USA','CAN']:continue
  g=shape(rec.shape.__geo_interface__);assert g.is_valid;g=g.intersection(clip)
  pg=transform(project,g).simplify(1.4,preserve_topology=True).difference(projected_water)
  assert pg.is_valid and not pg.is_empty and pg.intersection(projected_water).area<1e-6
  land[iso]=pg
  country_rows.append({'id':'country.'+iso.lower(),'name':'United States' if iso=='USA' else 'Canada (context only)','wgs84':g.__geo_interface__,**triangulate(pg)})
 assert set(land)=={'USA','CAN'}
 wanted={x['source_name']:x for x in scenario['cities']};places=[]
 for rec in cities.iterShapeRecords():
  d=rec.record.as_dict();name=d['NAME']
  if name not in wanted or d['ADM0_A3']!='USA':continue
  lon,lat=rec.shape.points[0];pos=list(map(float,project(lon,lat)));assert land['USA'].buffer(20).covers(Point(*pos)),name+' outside US land'
  places.append({**wanted[name],'name':name,'latitude':lat,'longitude':lon,'position':pos,'source_id':str(d['NE_ID'])})
 places.sort(key=lambda x:x['id']);assert len(places)==4 and len({p['id'] for p in places})==4
 assert {p['name'] for p in places}==set(wanted)
 regions=[];region_shapes=[]
 for city in places:
  cell=box(*land['USA'].bounds).buffer(1).envelope
  for other in places:
   if other['id']!=city['id']:cell=halfplane(cell,city['position'],other['position'])
  cell=cell.intersection(land['USA']);assert cell.is_valid and cell.buffer(20).covers(Point(*city['position']))
  regions.append({'id':city['region_id'],'country_id':'country.usa','name':city['name']+' – Spielregion','is_test':True,**triangulate(cell)})
  region_shapes.append(cell)
 assert len({r['id'] for r in regions})==4
 for i,a in enumerate(region_shapes):
  for b in region_shapes[i+1:]:assert a.intersection(b).area<1e-5,'Overlapping game regions'
 assert unary_union(region_shapes).symmetric_difference(land['USA']).area<.01,'Regions do not cover US land'
 for city in places:
  owning=[r['id'] for r,g in zip(regions,region_shapes) if g.buffer(20).covers(Point(*city['position']))]
  assert owning==[city['region_id']]
 checks=[];geod=Geod(ellps='WGS84');by_name={p['name']:p for p in places}
 for a,b in [('Chicago','Detroit'),('Cleveland','Pittsburgh')]:
  p,q=by_name[a],by_name[b];s=distance((p['latitude'],p['longitude']),(q['latitude'],q['longitude']))
  _,_,m=geod.inv(p['longitude'],p['latitude'],q['longitude'],q['latitude']);relative=abs(s-m/1000)/(m/1000);assert relative<.005
  checks.append({'from':p['id'],'to':q['id'],'sphere_km':s,'independent_wgs84_km':m/1000,'relative_error':relative})
 triangulate(MultiPolygon([Polygon([(0,0),(10,0),(10,10),(0,10)],holes=[[(2,2),(2,4),(4,4),(4,2)]]),Polygon([(20,0),(22,0),(22,2),(20,2)])]))
 # Cross-language projection reference vectors are computed by PROJ, not by the C++ formula.
 references=[{'latitude':p['latitude'],'longitude':p['longitude'],'north':p['position'][0],'east':p['position'][1]} for p in places]
 all_land=unary_union(list(land.values()));minx,miny,maxx,maxy=all_land.bounds
 result={'schema':1,'scenario':scenario,'projection':{'method':'spherical LCC 2SP','parameters':{n:constant(n) for n in ['RadiusKm','OriginLatitude','OriginLongitude','Parallel1','Parallel2','UnitsPerKm']}},
 'countries':sorted(country_rows,key=lambda x:x['id']),'lakes':lake_rows,'regions':regions,'cities':places,'projection_references':references,
 'camera':{'minimum':[minx-500,miny-500],'maximum':[maxx+500,maxy+500],'center':[(minx+maxx)/2,(miny+maxy)/2],'height':21000}}
 data=(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n').encode();(OUT/'map.json').write_bytes(data)
 report={'sources':sources,'license':'Public domain','attribution':'Made with Natural Earth.','license_url':'https://www.naturalearthdata.com/about/terms-of-use/','scenario_sha256':hashlib.sha256((ROOT/'GeoData/scenario.json').read_bytes()).hexdigest(),
 'map_sha256':hashlib.sha256(data).hexdigest(),'distances':checks,'counts':{'countries':len(country_rows),'lakes':len(lake_rows),'regions':len(regions),'cities':len(places)},
 'city_positions':places,'geometry':[{'id':r['id'],'parts':r['parts'],'holes':r['holes'],'triangles':len(r['triangles'])//3,'discarded_roundoff_triangles':r['discarded_roundoff_triangles']} for r in country_rows+lake_rows+regions],
 'tests':['valid WGS84 sources','lake subtraction from land','per-triangle hole/island coverage','area conservation','non-overlapping region partition','stable IDs and four city-region assignments','independent WGS84 distances <0.5%','hole+island regression fixture'],
 'limitations':['Modern generalized geography is not a verified 1913 reconstruction.','Only Great Lakes are subtracted, not every small lake or river.','No historical traffic, population or production values asserted.']}
 (OUT/'validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
 # Lightweight review artifact generated from the same vertices as Unreal, not a hand-drawn map.
 scale=1000/(maxy-miny);height=(maxx-minx)*scale
 def xy(p):return ((p[1]-miny)*scale,(maxx-p[0])*scale)
 svg=[f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 {height}"><rect width="1000" height="{height}" fill="#17364f"/>']
 for row,color in [(c,'#758779' if c['id']=='country.usa' else '#b4b8ac') for c in country_rows]+[(l,'#3a789b') for l in lake_rows]:
  path=' '.join('M'+' L'.join(f'{xy(p)[0]:.3f},{xy(p)[1]:.3f}' for p in ring)+' Z' for ring in row['rings'])
  svg.append(f'<path d="{path}" fill="{color}" fill-rule="evenodd" stroke="#253c40" stroke-width=".4"/>')
 for city in places:
  x,y=xy(city['position']);svg.append(f'<circle cx="{x}" cy="{y}" r="4" fill="#ffd16c"/><text x="{x+7}" y="{y-5}" font-family="sans-serif" font-size="16" fill="#fff">{html.escape(city["name"])}</text>')
 svg.append('</svg>');(ROOT/'Docs/MapPreview.svg').write_text(''.join(svg),encoding='utf-8')
 print(json.dumps(report,ensure_ascii=False,indent=2))
if __name__=='__main__':main()
