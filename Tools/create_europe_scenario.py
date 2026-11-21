"""Deterministic offline MAP03/MAP04 configuration; no runtime grants."""
import copy
from build_transport_network import compile_network
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
def read(path):
    return json.loads((ROOT/path).read_text(encoding='utf-8-sig'))
def write(path, value):
    (ROOT/path).write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')

def main():
    config = read('GeoData/Scenarios/de_nl1913.json')
    geo = read('GeoData/Generated/de_nl_1913/map.json')
    # Retain the completed AP09.4 rules/types; never copy US actors or assets.
    base = read('Content/Simulation/scenario.coisim')
    d = {k: copy.deepcopy(base['Definitions'][k]) for k in
         ['Goods','Needs','FacilityTypes','WarehouseTypes','VehicleTypes','Construction']}
    d['Network'] = compile_network()
    for key in ['Companies','Groups','Regions','Places','ResourceSites','BuildPlots']:
        d[key] = []
    w = {k: [] for k in ['Accounts','Companies','Groups','Facilities','Warehouses','Vehicles',
         'Inventories','Orders','Trades','Transports','Offers','Reservations','Regions','Countries','AssetSales']}
    w['Step'] = 0
    cities = {c['id']:c for c in geo['cities']}
    def city(suffix):
        found = [c for c in cities if c.endswith('.'+suffix)]
        if len(found)!=1:
            raise ValueError('Ambiguous city suffix '+suffix)
        return found[0]
    def company(cid, name, profile, home='', playable=False):
        d['Companies'].append(dict(Id=cid,Name=name,DecisionProfile=profile,
            AccountingOnly=profile=='service',Playable=playable,StartCityId=home,
            HeadquartersCityId=home,Description=config['descriptions'].get(cid,'')))
        w['Companies'].append(dict(Id=cid,AccountId='account.'+cid))
        w['Accounts'].append(dict(Id='account.'+cid,OwnerId=cid,
            BalanceCents=config['capital_cents'].get(cid,config['local_capital_cents']),ReservedCents=0))
    def store(sid, owner, home, kind='warehouse.standard', name=''):
        w['Warehouses'].append(dict(Id=sid,TypeId=kind,OwnerId=owner,CityId=home,
            DisplayName=name,CostRecipientAccountId='account.company.external' if owner=='company.services'
            else 'account.company.services',ReservedIncomingPoints=0,ArrearsCents=0))
    def stock(owner, sid, good, kg):
        w['Inventories'].append(dict(Id='stock.'+sid+'.'+good,OwnerId=owner,StorageId=sid,
            GoodId='good.'+good,QuantityKg=kg,ReservedKg=0))
    def vehicle(vid, owner, home):
        w['Vehicles'].append(dict(Id=vid,OwnerId=owner,CityId=home,HomeCityId=home,
            TypeId='vehicle_type.truck',Bound=False,TransportId='',CostRecipientAccountId='account.company.services'))
    for item in config['companies']:
        company(item['id'],item['name'],item['profile'],city(item['home']) if item.get('home') else '',item.get('playable',False))
    resources = {city(k):v for k,v in config['resources'].items()}
    for c in sorted(cities.values(),key=lambda x:x['id']):
        cid, rid = c['id'], c['region_id']
        region = next(r for r in geo['regions'] if r['id']==rid)
        res = resources.get(cid,{})
        d['Places'].append(dict(Id=cid,Name=c['name'],RegionId=rid,Latitude=c['latitude'],Longitude=c['longitude']))
        d['Regions'].append(dict(Id=rid,CountryId=c['country_id'],MarketId='market.'+rid.removeprefix('region.'),
            Name=region['name'],CountryName=next(x['name'] for x in geo['countries'] if x['id']==c['country_id']),
            FarmlandPermille=res.get('farmland',0),ForestKg=res.get('forest',0),OreKg=res.get('ore',0)))
        for kind, value in res.items():
            d['ResourceSites'].append(dict(Id='site.'+cid+'.'+kind,CityId=cid,Kind=kind,
                InitialKg=0 if kind=='farmland' else value,SuitabilityPermille=value if kind=='farmland' else 1000,
                MaxBatchesPerHour=config['resource_batches_per_hour']))
        for band in config['households']:
            gid='group.'+cid+'.'+band['band']; account='account.'+gid
            d['Groups'].append(dict(Id=gid,RegionId=rid,NeedProfileId='need.'+band['band'],
                IncomeBand=band['band'],ConsumptionCityId=cid))
            w['Groups'].append(dict(Id=gid,AccountId=account,Population=band['population'],
                Workforce=band['population']//2,Employed=0,DailyIncomeCents=0,BudgetCents=band['cash_cents']//10,
                Consumed=[dict(GoodId=g['Id'],Kg=0) for g in d['Goods']],
                UnmetNeed=[dict(GoodId='good.'+g,Kg=0) for g in ['food','luxury']]))
            w['Accounts'].append(dict(Id=account,OwnerId=gid,BalanceCents=band['cash_cents'],ReservedCents=0))
        local='company.local.'+cid
        company(local,'Regionalhandel '+c['name']+' (TEST)','trader',cid)
        sid='retail.'+cid
        store(sid,local,cid,'retail.type.food','Lebensmittel · '+c['name'])
        stock(local,sid,'food',config['retail_opening_food_kg'])
        # Finite paid logistics even at hubs without extractive resources.
        vehicle('vehicle.local.'+cid,local,cid)
        store('storage.service.'+cid,'company.services',cid)
        for kind in ['warehouse','retail']:
            d['BuildPlots'].append(dict(Id='plot.'+cid+'.'+kind,CityId=cid,Kind='warehouse',ResourceSiteId=''))
        if 'farmland' in res:
            d['BuildPlots'].append(dict(Id='plot.'+cid+'.farm',CityId=cid,Kind='facility',ResourceSiteId='site.'+cid+'.farmland'))
    for plant in config['plants']:
        home=city(plant['home']); sid='storage.'+plant['id']; owner=plant['owner']; kind=plant['kind']
        store(sid,owner,home)
        resource={'food':'farmland','wood':'forest','ore':'ore'}.get(kind,'')
        w['Facilities'].append(dict(Id='facility.'+plant['id'],OwnerId=owner,TypeId='facility_type.'+kind,
            CityId=home,StorageId=sid,CostRecipientAccountId='account.company.services',UtilizationPermille=0,
            PayrollArrearsCents=0,Employment=[],ResourceSiteId='site.'+home+'.'+resource if resource else '',
            ResourceBatchesPerHour=1 if resource else 0))
        stock(owner,sid,kind,config['producer_opening_output_kg'])
        for good, kg in plant.get('inputs',{}).items():
            stock(owner,sid,good,kg)
    for item in config['trading_stores']:
        store(item['id'],item['owner'],city(item['home']),item.get('type','warehouse.player'),item.get('name',''))
        for good,kg in item.get('stock',{}).items():
            stock(item['owner'],item['id'],good,kg)
    for item in config['vehicles']:
        vehicle(item['id'],item['owner'],city(item['home']))
    for offer in config.get('asset_sales',[]):
        asset=next(x for x in w['Warehouses'] if x['Id']==offer['asset'])
        w['AssetSales'].append(dict(Id=offer['id'],SellerId=asset['OwnerId'],AssetId=asset['Id'],
            CityId=asset['CityId'],Kind='warehouse',PriceCents=offer['price_cents'],ExpiresHour=168,Active=True))
    economy=copy.deepcopy(base['Economy']); economy.update(config['economy'])
    economy['Bank']=copy.deepcopy(config['bank'])
    economy['Bank']['OpeningBackingCents']=sum(a['BalanceCents'] for a in w['Accounts'])+economy['Bank']['OpeningCapitalCents']
    economy['AccountingOpeningAssets']=[]
    for key,price in [('Warehouses',50000),('Facilities',300000),('Vehicles',200000)]:
        for asset in w[key]:
            economy['AccountingOpeningAssets'].append(dict(Id=asset['Id'],OwnerId=asset['OwnerId'],ValueCents=price,Known=True))
    result=dict(Schema=2,HubSchema=1,Id=geo['scenario']['id'],StartDate='1913-01-01',
        PlayerCompanyId=config['player_company_id'],Currency='USD',BalancingNotice=config['notice'],
        Settings=copy.deepcopy(base['Settings']),Definitions=d,InitialState=w,Economy=economy,
        ContractExamples=dict(Orders=[],Trades=[],Transports=[]))
    for section in [d,w,economy]:
        for value in section.values():
            if isinstance(value,list) and value and isinstance(value[0],dict) and 'Id' in value[0]:
                value.sort(key=lambda x:x['Id'])
    assert len(d['Regions'])==len(d['Places'])==50
    assert len({r['MarketId'] for r in d['Regions']})==50
    assert all('city.us.' not in str(x) and 'region.test.' not in str(x) for x in [d,w,economy])
    write('Content/Simulation/scenario.de_nl1913.coisim',result)
    print('MAP03/MAP04: 50 hubs, 50 markets, 150 household groups; explicit Rhine pilot network; all opening balances unchanged.')

if __name__=='__main__':
    main()
