"""MAP04 offline graph compiler. Standard library only; no network, assets or grants."""
import hashlib
import json
import math
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def read(p): return json.loads((ROOT/p).read_text(encoding='utf-8-sig'))
def point(p): return dict(Latitude=p[0],Longitude=p[1])
def meters(a,b):
    x,y=map(math.radians,[b['Latitude']-a['Latitude'],b['Longitude']-a['Longitude']])
    h=math.sin(x/2)**2+math.cos(math.radians(a['Latitude']))*math.cos(math.radians(b['Latitude']))*math.sin(y/2)**2
    return 12742017.6*math.atan2(math.sqrt(min(1,h)),math.sqrt(max(0,1-h)))
def compile_network():
    s=read('GeoData/Networks/de_nl_1913/network.source.json');g=read('GeoData/Generated/de_nl_1913/map.json');rules=s['Rules']
    n=dict(Id=s['Id'],Date=s['Date'],Nodes=[],Edges=[],Ports=[],Access=[],Evidence=s['Evidence'])
    countries=['country.de.1913','country.nl.1913']
    n['Access']=[dict(Id='access.rhine.de_nl.1913',EvidenceId='ccnr.history',Enabled=True,FirstYear=1869,LastYear=1913,Countries=countries,Carriers=countries)]
    def node(id,name,kind,country,p,hub=''):
        n['Nodes'].append(dict(Id=id,Name=name,Kind=kind,CountryId=country,HubId=hub,Position=point(p)))
    cities={c['id']:c for c in g['cities']}
    for c in g['cities']:node('hub.'+c['id'],c['name'],'hub',c['country_id'],[c['latitude'],c['longitude']],c['id'])
    for w in s['WaterNodes']:node('water.'+w['Id'],w['Id'],'river','country.'+w['Country']+'.1913',[w['Latitude'],w['Longitude']])
    def edge(id,a,b,mode,evidence,bends=(),reverse=False,enabled=True):
        nodes={x['Id']:x for x in n['Nodes']};geo=[nodes[a]['Position']]+[point(p) for p in bends]+[nodes[b]['Position']]
        transfer=mode=='transfer';road=mode=='road'
        row=dict(Id=id,FromId=a,ToId=b,Mode=mode,AccessId=n['Access'][0]['Id'],EvidenceId=evidence,Enabled=enabled,
          FirstYear=rules['FirstYear'],LastYear=rules['LastYear'],LengthMeters=0 if transfer else math.ceil(sum(meters(x,y) for x,y in zip(geo,geo[1:]))),
          CapacityKg=rules['TerminalCapacityKg'] if transfer or road else rules['WaterCapacityKg'],
          SpeedMetersHour=0 if transfer else rules['RoadMetersHour'] if road else rules['UpstreamMetersHour'] if reverse else rules['DownstreamMetersHour'],
          FixedMinutes=rules['HandlingMinutes'] if transfer else 0,FixedCents=rules['HandlingFixedCents'] if transfer else 0,
          CentsPerKm=0 if transfer else rules['RoadCentsKm'] if road else rules['WaterCentsKm'],CentsPerKg=rules['HandlingCentsKg'] if transfer else 0,Geometry=geo)
        n['Edges'].append(row)
    water=s['WaterNodes']
    for i,(a,b) in enumerate(zip(water,water[1:])):
        evidence='eb1911.rhine' if i<6 else 'eb1911.holland';prefix='reach.'+a['Id']+'.'+b['Id']
        edge(prefix+'.down','water.'+a['Id'],'water.'+b['Id'],'river',evidence,s['Bends'][i])
        edge(prefix+'.up','water.'+b['Id'],'water.'+a['Id'],'river',evidence,list(reversed(s['Bends'][i])),True)
    for p in s['Ports']:
        hub=cities[p['Hub']];land='terminal.'+p['Id'];w='water.'+p['Water'];hn='hub.'+p['Hub'];base='port.'+p['Id']
        node(land,p['Name'],'terminal',hub['country_id'],p['Land'])
        edge(base+'.feeder.out',hn,land,'road',p['Evidence'],p['RoadBends']);edge(base+'.feeder.back',land,hn,'road',p['Evidence'],list(reversed(p['RoadBends'])))
        edge(base+'.load',land,w,'transfer',p['Evidence']);edge(base+'.unload',w,land,'transfer',p['Evidence'])
        n['Ports'].append(dict(Id=base,Name=p['Name'],HubId=p['Hub'],LandNodeId=land,WaterNodeId=w,FeederOutId=base+'.feeder.out',FeederBackId=base+'.feeder.back',TransferOutId=base+'.load',TransferBackId=base+'.unload',Enabled=True))
    for c in s['Candidates']:
        node(c['To'],c['Id'],c['Mode'],'country.nl.1913',[c['Latitude'],c['Longitude']]);edge('candidate.'+c['Id'],'water.'+c['From'],c['To'],c['Mode'],c['Evidence'],enabled=False)
    for key in ['Nodes','Edges','Ports','Access','Evidence']:n[key].sort(key=lambda x:x['Id'])
    assert len({x['Id'] for x in n['Nodes']})==len(n['Nodes'])
    return n
if __name__=='__main__':
    n=compile_network();output=json.dumps(n,ensure_ascii=False,indent=2)+'\n';folder=ROOT/'GeoData/Generated/de_nl_1913';(folder/'transport_network.json').write_text(output,encoding='utf-8',newline='\n')
    report=dict(network_id=n['Id'],sha256=hashlib.sha256(output.encode()).hexdigest(),nodes=len(n['Nodes']),edges=len(n['Edges']),ports=len(n['Ports']),enabled_edges=sum(e['Enabled'] for e in n['Edges']),disabled_candidates=[e['Id'] for e in n['Edges'] if not e['Enabled']],water_route_meters=sum(e['LengthMeters'] for e in n['Edges'] if e['Mode']=='river' and e['Id'].endswith('.down')),money_or_goods_created=0)
    (folder/'transport_validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report))
