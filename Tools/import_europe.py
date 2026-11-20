"""MAP02 offline import. Pinned source bytes are checked before any output changes.

Sources: OHM historical relations, DANS NLGIS, GeoNames, Natural Earth and
NordNordWest's georeferenced historical SVG. Never infers navigability.
"""
import gzip
import hashlib
import json
import math
import re
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np
import shapefile
from pyproj import Geod, Proj, Transformer
from shapely import coverage_simplify, make_valid, set_precision
from shapely.geometry import (GeometryCollection, LineString, Point, Polygon,
                              MultiPolygon, box, shape)
from shapely.ops import polygonize, polygonize_full, transform, unary_union
from svgpathtools import parse_path

from import_geodata import triangulate, polygons

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / 'GeoData/Raw/MAP02'
OUT = ROOT / 'GeoData/Generated/de_nl_1913'
PLAN = ROOT / 'GeoData/Plans/de_nl_1913'


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write_json(path, data):
    path.write_text(json.dumps(data, ensure_ascii=False, separators=(',', ':')) + '\n',
                    encoding='utf-8', newline='\n')


def polygon_only(g):
    return unary_union(polygons(g))


def svg_parts(element, step=.7):
    """SVG cubic curves sampled in source pixels, not traced from screen colour."""
    tag = element.tag.split('}')[-1]
    if tag in ('polygon', 'polyline'):
        values = list(map(float, re.findall(r'-?\d*\.?\d+(?:[eE][-+]?\d+)?', element.get('points'))))
        points = list(zip(values[::2], values[1::2]))
        if tag == 'polygon':
            points.append(points[0])
        return [points]
    if tag == 'line':
        return [[(float(element.get('x1')), float(element.get('y1'))),
                 (float(element.get('x2')), float(element.get('y2')))]]
    result = []
    for sub in parse_path(element.get('d', '')).continuous_subpaths():
        points = []
        for segment in sub:
            count = max(1, math.ceil(abs(segment.end-segment.start)/step))
            for k in range(count):
                p = segment.point(k/count)
                points.append((p.real, p.imag))
        if sub:
            p = sub[-1].end
            points.append((p.real, p.imag))
        if len(points) > 1:
            result.append(points)
    return result


def svg_polygon(element):
    result = Polygon()
    for points in svg_parts(element):
        if len(points) > 3:
            result = result.symmetric_difference(polygon_only(make_valid(Polygon(points))))
    return result


def historical_regions(config, report):
    raw = json.loads(gzip.decompress((RAW/'germany1913.ohm.json.gz').read_bytes()))
    if raw.get('remark'):
        raise ValueError('Incomplete Overpass snapshot: '+raw['remark'])
    nodes = {e['id']: (e['lon'], e['lat']) for e in raw['elements'] if e['type'] == 'node'}
    ways = {e['id']: e for e in raw['elements'] if e['type'] == 'way'}
    relations = {e['id']: e for e in raw['elements'] if e['type'] == 'relation'}
    output = {}
    for slug, relation_id in read_json(RAW/'region_sources.json').items():
        relation = relations[relation_id]
        tags = relation['tags']
        if not (tags['start_date'] <= '1913-01-01' < tags.get('end_date', '9999')):
            raise ValueError(f'Wrong historical interval: {slug}')
        rings = {'outer': [], 'inner': []}
        used = []
        for member in relation['members']:
            if member['type'] != 'way' or member.get('role') not in rings:
                continue  # labels/admin centres/subareas are not boundary geometry
            way = ways[member['ref']]
            used.append(way)
            rings[member['role']].append(LineString([nodes[n] for n in way['nodes']]))
        for entity in [relation, *used]:
            license = entity.get('tags', {}).get('license', entity.get('tags', {}).get('licence', 'CC0'))
            if license not in ('CC0', 'CC0-1.0', 'Public domain', 'copyright expired', 'CC-PDM-1.0'):
                raise ValueError(f'Unreviewed per-element licence on {entity["id"]}: {license}')
        outer = unary_union(list(polygonize(rings['outer'])))
        inner = unary_union(list(polygonize(rings['inner'])))
        g = outer.difference(inner)
        if g.is_empty or not g.is_valid:
            raise ValueError(f'Invalid historical relation: {slug}')
        # All boundary members must belong to a closed ring. No arbitrary closure.
        # Test individual rings before union: touching source outer parts can
        # legitimately have an interior shared edge after their union.
        for members in rings.values():
            _, cuts, dangles, invalid = polygonize_full(members)
            if cuts.length+dangles.length+invalid.length > 1e-6:
                raise ValueError(f'Unclosed/unaccounted historical boundary: {slug}')
        rid = 'region.de.1913.'+slug
        output[rid] = g
        report['region_provenance'].append(dict(
            id=rid, relation=relation_id, version=relation['version'],
            start=tags['start_date'], end=tags.get('end_date'),
            uncertainty={k: v for k, v in tags.items() if k.startswith(('fixme', 'note'))},
            source=tags.get('source', 'OHM contributor reconstruction; source details vary by member'),
            member_source_counts=dict(__import__('collections').Counter(w.get('tags', {}).get('source', 'not tagged') for w in used))))
    # Historical province snapshots bracket the scenario. Source coordinates are
    # Dutch RD in decametres (not EPSG:4326 degrees). Keep original SHP/DBF/SHX.
    rd = Transformer.from_crs('EPSG:28992', 'EPSG:4326', always_xy=True)
    def rd_to_wgs(x, y, z=None):
        return rd.transform(np.asarray(x)*10, np.asarray(y)*10)
    names = {1:'groningen', 2:'friesland', 3:'drenthe', 4:'overijssel', 5:'gelderland',
             6:'utrecht', 7:'noord_holland', 8:'zuid_holland', 9:'zeeland',
             10:'noord_brabant', 11:'limburg'}
    later = {int(r.record['ID_']): transform(rd_to_wgs, shape(r.shape.__geo_interface__))
             for r in shapefile.Reader(str(RAW/'prov1920'), encoding='latin1').iterShapeRecords()}
    for row in shapefile.Reader(str(RAW/'prov1890'), encoding='latin1').iterShapeRecords():
        code = int(row.record['ID_'])
        g = transform(rd_to_wgs, shape(row.shape.__geo_interface__))
        repaired = not g.is_valid
        g = polygon_only(make_valid(g))
        rid = 'region.nl.1913.'+names[code]
        output[rid] = g
        report['region_provenance'].append(dict(
            id=rid, source='DANS NLGIS PROV1890', comparison='PROV1920',
            make_valid=repaired, validity='Historical 1890/1920 province reconstruction used as 1913 cartographic proxy; not survey-exact 1913.',
            bracketing_difference_degrees2=g.symmetric_difference(polygon_only(make_valid(later[code]))).area))
    return output


def import_map():
    config = read_json(RAW/'import_config.json')
    locks = read_json(RAW/'sources.lock.json')
    for item in locks['files']:
        file = ROOT/item['path']
        if hashlib.sha256(file.read_bytes()).hexdigest() != item['sha256']:
            raise ValueError('Source changed: '+item['path'])
    plan = read_json(PLAN/'manifest.json')
    report = dict(package='MAP02', sources=locks, region_provenance=[],
                  seam_resolutions=[], tests=[], limitations=config['limitations'])
    projection = Proj(proj='lcc', lat_0=52, lon_0=13, lat_1=48, lat_2=55,
                      R=6371008.8, units='m')
    def project(lon, lat, z=None):
        east, north = projection(lon, lat)
        return np.asarray(north)*.007, np.asarray(east)*.007
    raw_regions = historical_regions(config, report)
    expected = {r['id'] for r in plan['regions']}
    if set(raw_regions) != expected:
        raise ValueError('Region manifest mismatch')
    # Source uncertainty at a small number of historical river/territorial seams
    # is resolved by a reviewed priority list, never by random array order.
    projected = {k: set_precision(transform(project, g), .00007) for k,g in raw_regions.items()}
    for fix in config['seam_priority']:
        winner, loser = fix['winner'], fix['loser']
        overlap = projected[winner].intersection(projected[loser])
        km2 = overlap.area/49
        if km2 > fix['max_overlap_km2']:
            raise ValueError('Historical seam exceeds reviewed budget: '+loser)
        projected[loser] = projected[loser].difference(projected[winner])
        report['seam_resolutions'].append(dict(**fix, overlap_km2=km2))
    ids = sorted(projected)
    for i, key in enumerate(ids):
        for other in ids[i+1:]:
            area = projected[key].intersection(projected[other]).area
            if area > .00001:
                raise ValueError(f'Unreviewed regional overlap {key}/{other}: {area/49} km2')
    # Coverage-aware simplification keeps shared arcs identical and retains all
    # source islands/enclaves, including ones smaller than the base tolerance.
    simplified = coverage_simplify([projected[k] for k in ids], 1.4)
    projected = dict(zip(ids, simplified))
    # Independently reconstructed source edges can have narrow gaps rather than
    # identical shared arcs. Simplification may bridge those gaps. Apply only
    # the same reviewed priorities/budgets again; never accept new seam pairs.
    for fix, audit in zip(config['seam_priority'], report['seam_resolutions']):
        winner, loser = fix['winner'], fix['loser']
        overlap_km2 = projected[winner].intersection(projected[loser]).area/49
        if overlap_km2 > fix['max_overlap_km2']:
            raise ValueError('Simplified seam exceeds reviewed budget: '+loser)
        audit['simplification_overlap_km2'] = overlap_km2
        projected[loser] = projected[loser].difference(projected[winner])
    for k,g in projected.items():
        if g.is_empty or not g.is_valid:
            raise ValueError('Invalid simplified region: '+k)
    for i, key in enumerate(ids):
        for other in ids[i+1:]:
            if projected[key].intersection(projected[other]).area > .00001:
                raise ValueError(f'Simplification introduced regional overlap: {key}/{other}')

    # NordNordWest supplies a stated equirectangular source extent; converting
    # this SVG coordinate system is exact, rather than fitting an unknown CRS.
    svg = ET.parse(RAW/'germany1914.svg').getroot()
    def svg_wgs(x,y,z=None):
        return 5.5+np.asarray(x)/1424.931*17.7, 56-np.asarray(y)/1133.484*8.8
    water_svg = unary_union([svg_polygon(e) for e in svg if e.get('fill') == '#C7EDFF'])
    water_wgs = transform(svg_wgs, water_svg)
    water = polygon_only(transform(project, water_wgs))
    # Modern national borders are dissolved here: this is physical background
    # context only. The NL modern coast/polders are replaced by historical land.
    import zipfile, io
    ne = ROOT/'GeoData/Raw/ne_10m_admin_0_countries.zip'
    archive = zipfile.ZipFile(ne)
    reader = shapefile.Reader(shp=io.BytesIO(archive.read('ne_10m_admin_0_countries.shp')),
                             shx=io.BytesIO(archive.read('ne_10m_admin_0_countries.shx')),
                             dbf=io.BytesIO(archive.read('ne_10m_admin_0_countries.dbf')), encoding='utf-8')
    clip = box(*plan['projection']['bbox_wgs84'])
    context = []
    for row in reader.iterShapeRecords():
        g = shape(row.shape.__geo_interface__)
        if not g.intersects(clip):
            continue
        if row.record['ADM0_A3'] != 'NLD':
            context.append(g.intersection(clip))
    nl = unary_union([g for k,g in raw_regions.items() if '.nl.' in k])
    context.append(nl)
    # Replace the German core with historical coast and lakes; political
    # membership is deliberately retained separately in projected regions.
    de = unary_union([g for k,g in projected.items() if '.de.' in k])
    land = transform(project, unary_union(context)).union(de).difference(water)
    # Historical NL shoreline wins over any neighbouring modern country polygon
    # overlap. DANS' bracketing years and corrections are recorded, not hidden.
    countries = []
    regions = []
    for country in plan['countries']:
        members = [projected[r['id']] for r in plan['regions'] if r['country_id']==country['id']]
        countries.append(dict(id=country['id'],name=country['name'], **triangulate(unary_union(members))))
    for region in plan['regions']:
        g = projected[region['id']]
        regions.append(dict(id=region['id'], name=region['name'], country_id=region['country_id'],
                            hub_id=region['hub_id'], historical_type=region['historical_type'],
                            is_test=False, **triangulate(g)))

    hubs = read_json(RAW/'hubs_wgs84.json')
    import zipfile
    geonames = {}
    with zipfile.ZipFile(RAW/'cities500.zip') as z:
        for row in z.read('cities500.txt').decode('utf-8').splitlines():
            fields=row.split('\t')
            geonames[fields[0]]=fields
    for hub in hubs:
        source=geonames[hub['geonameid']]
        if (float(source[4]),float(source[5])) != (hub['latitude'],hub['longitude']):
            raise ValueError('Hub differs from pinned GeoNames record: '+hub['id'])
    shore_errors=[]
    for hub in hubs:
        point = Point(*map(float,project(hub['longitude'],hub['latitude'])))
        owners = [k for k,g in projected.items() if g.covers(point)]
        if owners != [hub['region_id']]:
            raise ValueError(f'Hub outside or multiply assigned: {hub["name"]}: {owners}')
        if not land.covers(point):
            km=land.distance(point)/7
            print(f'SHORE_REVIEW {hub["id"]} distance_km={km}',flush=True)
            if km>config.get('hub_shore_tolerance_km',{}).get(hub['id'],0):
                shore_errors.append((hub['name'],km))
            hub['shore_tolerance_units']=config.get('hub_shore_tolerance_km',{}).get(hub['id'],0)*7
        hub['position'] = list(point.coords[0])
    if shore_errors:
        raise ValueError('Hub/shore generalisation review required: '+str(shore_errors))

    rivers = []
    for index,element in enumerate(svg):
        if element.get('stroke') != '#84BCD5' or element.get('fill') != 'none':
            continue
        for part,points in enumerate(svg_parts(element)):
            if len(points)<2:
                continue
            line = transform(project,transform(svg_wgs,LineString(points))).simplify(.35)
            rivers.append(dict(id=f'river.svg.{index:03}.{part:03}', name='Historischer GewÃ¤sserverlauf',
                               points=list(line.coords), half_width=.35, navigable=False))
    # Historical main delta branches are corroborated in the pinned EB1911
    # Holland plate. Natural Earth's generalised modern linework is a declared
    # geometric proxy only, not a historical channel survey or navigation graph.
    archive=zipfile.ZipFile(RAW/'rivers.zip')
    name='ne_10m_rivers_lake_centerlines'
    reader=shapefile.Reader(shp=io.BytesIO(archive.read(name+'.shp')),
                           shx=io.BytesIO(archive.read(name+'.shx')),
                           dbf=io.BytesIO(archive.read(name+'.dbf')),encoding='utf-8')
    allowed={1159129229,1159128949,1159112369,1159112387,1159112519,1159112531}
    for row in reader.iterShapeRecords():
        source_id=int(row.record['ne_id'])
        if source_id not in allowed:continue
        g=shape(row.shape.__geo_interface__).intersection(box(2.5,47,5.5,56.5))
        parts=list(g.geoms) if hasattr(g,'geoms') else [g]
        for k,part in enumerate(parts):
            if part.is_empty or part.geom_type!='LineString':continue
            line=transform(project,part).simplify(.35)
            rivers.append(dict(id=f'river.ne.{source_id}.{k}',name='Rheindelta / Maas Â· vereinfachter Verlauf',
                               points=list(line.coords),half_width=.35,navigable=False))
    # The sea outside the historical Dutch land is supplied independently of
    # the old SVG's western clipping edge. Never fill the Zuiderzee with polders.
    bounds = transform(project,clip).envelope
    background_water = bounds.difference(land)
    water_visible = polygon_only(water.intersection(bounds).union(background_water))
    water_surfaces = [dict(id='water.physical.1913',name='Meer und BinnengewÃ¤sser',**triangulate(water_visible))]
    land_surfaces = [dict(id='land.context.1913',name='Historische Landmaske mit modernem Nachbarkontext',**triangulate(polygon_only(land.intersection(bounds))))]
    # Region rendering uses land-clipped surfaces, but picking uses the original
    # political polygons above. No conflation of jurisdiction and wet pixels.
    region_land = []
    for region in regions:
        g = projected[region['id']].intersection(land)
        if g.is_empty:
            raise ValueError('Region without any visible land: '+region['id'])
        region_land.append(dict(id=region['id'],name=region['name'],**triangulate(polygon_only(g))))
    minimum = list(bounds.bounds[:2]); maximum = list(bounds.bounds[2:])
    result = dict(schema=1, scenario=dict(id='scenario.de_nl.geography.1913', start_date='1913-01-01',
                  geography_only=True, bbox_wgs84=plan['projection']['bbox_wgs84']),
                  projection=dict(method='spherical LCC 2SP',parameters=dict(RadiusKm=6371.0088,
                    OriginLatitude=52,OriginLongitude=13,Parallel1=48,Parallel2=55,UnitsPerKm=7)),
                  countries=countries,regions=regions,cities=hubs,lakes=water_surfaces,
                  land=land_surfaces,region_land=region_land,rivers=rivers,
                  camera=dict(minimum=minimum,maximum=maximum,center=[(minimum[0]+maximum[0])/2,(minimum[1]+maximum[1])/2-1800],height=16000),
                  projection_references=[dict(latitude=h['latitude'],longitude=h['longitude'],north=h['position'][0],east=h['position'][1]) for h in hubs])
    geod = Geod(ellps='WGS84')
    report['distances'] = []
    for a,b in [('city.nl.1913.zuid_holland','city.de.1913.rheinprovinz'),('city.de.1913.berlin','city.de.1913.ostpreussen')]:
        h,q = next(x for x in hubs if x['id']==a),next(x for x in hubs if x['id']==b)
        _,_,m = geod.inv(h['longitude'],h['latitude'],q['longitude'],q['latitude'])
        report['distances'].append(dict(**{'from':a,'to':b},independent_wgs84_km=m/1000))
    # Explicit, configured acceptance probes, including foreign land, water,
    # Alt-Berlin, offshore islands and disconnected historic territories.
    checks = []
    for probe in config['probes']:
        point = Point(*map(float,project(probe['longitude'],probe['latitude'])))
        owners = [key for key,g in projected.items() if g.covers(point)]
        actual_land = land.covers(point)
        if probe.get('region') is not None and owners != ([probe['region']] if probe['region'] else []):
            raise ValueError(f'Political probe {probe["name"]}: {owners}')
        if 'land' in probe and actual_land != probe['land']:
            raise ValueError(f'Physical probe {probe["name"]}: {actual_land}')
        checks.append(dict(**probe,actual_regions=owners,actual_land=actual_land,position=list(point.coords[0])))
    report.update(counts=dict(countries=2,regions=50,cities=50,river_lines=len(rivers)),probes=checks,
                  tests=['source byte locks','historical relation intervals','closed relation rings','per-element licence tags',
                         'reviewed seam budgets','valid political/physical polygons','50 distinct hub containments',
                         'island/hole preserving per-triangle coverage','configured physical/political probes'],
                  attribution=locks['attribution'])
    OUT.mkdir(parents=True,exist_ok=True)
    # Output is committed only after all validations pass.
    write_json(OUT/'map.json',result)
    report['map_sha256'] = hashlib.sha256((OUT/'map.json').read_bytes()).hexdigest()
    write_json(OUT/'validation.json',report)
    print(json.dumps({k:report[k] for k in ['counts','map_sha256','seam_resolutions']},ensure_ascii=False,indent=2))


if __name__ == '__main__':
    import_map()
