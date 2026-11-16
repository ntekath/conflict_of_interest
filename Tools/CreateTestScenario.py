from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
geo=json.loads((root/'GeoData/Generated/map.json').read_text(encoding='utf-8'))
amount=lambda good,kg:dict(GoodId=good,Kg=kg)
goods=[dict(Id='good.'+id,Name=name,Unit='kg',NeedCategory=need,StoragePointsPerKg=points,SpoilPartsPerMillionPerDay=spoil) for id,name,need,points,spoil in [('food','Nahrung','basic',1,10000),('wood','Holz','none',2,0),('ore','Erz','none',1,0),('tools','Werkzeuge','none',3,0),('luxury','Luxuswaren','luxury',4,1000)]]
companies=[dict(Id='company.'+id,Name=name,DecisionProfile=profile) for id,name,profile in [('player','Spielerhandel','player'),('food','Prairie Food (TEST)','producer'),('wood','Lake Timber (TEST)','producer'),('ore','Allegheny Ore (TEST)','producer'),('tools','Steel Tools (TEST)','producer'),('luxury','Detroit Crafts (TEST)','producer'),('merchant','Lakes Merchant (TEST)','trader'),('carrier','Great Lakes Freight (TEST)','carrier'),('services','Lager- und Betriebskostenkonto (TEST)','service'),('external','Offen bilanzierter externer Sektor (TEST)','service')]]
regions=[dict(Id=c['region_id'],CountryId='country.usa',MarketId='market.'+c['id'].split('.')[-1],FarmlandPermille=[800,300,250,100][i],ForestKg=[100000,800000,300000,150000][i],OreKg=[0,100000,0,900000][i]) for i,c in enumerate(geo['cities'])]
places=[dict(Id=c['id'],Name=c['name'],RegionId=c['region_id'],Latitude=c['latitude'],Longitude=c['longitude']) for c in geo['cities']]
needs=[dict(Id='need.'+band,BasicGoodId='good.food',LuxuryGoodId='good.luxury',BasicKgPerThousandPerDay=basic,LuxuryKgPerThousandPerDay=lux,BasicMaxPriceCents=12,LuxuryMaxPriceCents=120,LuxuryBudgetPermille=budget) for band,basic,lux,budget in [('low',100,1,50),('middle',120,10,150),('high',140,30,300)]]
groups=[];groupstates=[];accounts=[]
for c in geo['cities']:
 for band,pop,cash in [('low',6000,500000),('middle',3000,1000000),('high',1000,2000000)]:
  id='group.'+c['id'].split('.')[-1]+'.'+band; account='account.'+id
  groups.append(dict(Id=id,RegionId=c['region_id'],NeedProfileId='need.'+band,IncomeBand=band,ConsumptionCityId=c['id']))
  groupstates.append(dict(Id=id,AccountId=account,Population=pop,Workforce=pop//2,Employed=0,DailyIncomeCents=0,BudgetCents=cash//10,Consumed=[amount(g['Id'],0) for g in goods],UnmetNeed=[amount('good.food',0),amount('good.luxury',0)]))
  accounts.append(dict(Id=account,OwnerId=id,BalanceCents=cash,ReservedCents=0))
for c in companies:accounts.append(dict(Id='account.'+c['Id'],OwnerId=c['Id'],BalanceCents=2500000 if c['Id']=='company.player' else (0 if c['Id']=='company.external' else 5000000),ReservedCents=0))
facilitytypes=[];facilities=[];warehouses=[];stocks=[]
service='account.company.services'
def warehouse(id,owner,city,type='warehouse.standard'):
 warehouses.append(dict(Id=id,TypeId=type,OwnerId=owner,CityId='city.us.'+city,CostRecipientAccountId='account.company.external' if owner=='company.services' else service,ReservedIncomingPoints=0,ArrearsCents=0))
warehouse('storage.player','company.player','detroit','warehouse.player')
for id,city,resource,inputs,outputs in [('food','chicago','farmland',[],[amount('good.food',100)]),('wood','cleveland','forest',[],[amount('good.wood',100)]),('ore','pittsburgh','ore',[],[amount('good.ore',100)]),('tools','pittsburgh','none',[amount('good.ore',100),amount('good.wood',20)],[amount('good.tools',40)]),('luxury','detroit','none',[amount('good.tools',10),amount('good.wood',10)],[amount('good.luxury',10)])]:
 facilitytypes.append(dict(Id='facility_type.'+id,Name=id+' (TEST)',ResourceRequirement=resource,Inputs=inputs,Outputs=outputs,CapacityBatchesPerHour=1,Jobs=20,DailyWageCents=150,DailyOperatingCostCents=300,MarginPermille=200))
 warehouse('storage.'+id,'company.'+id,city)
 facilities.append(dict(Id='facility.'+id,TypeId='facility_type.'+id,OwnerId='company.'+id,CityId='city.us.'+city,StorageId='storage.'+id,CostRecipientAccountId=service,UtilizationPermille=0,PayrollArrearsCents=0,Employment=[]))
 for a in outputs+inputs:
  sid='stock.'+id+'.'+a['GoodId'].split('.')[-1]
  if not any(s['Id']==sid for s in stocks):stocks.append(dict(Id=sid,GoodId=a['GoodId'],OwnerId='company.'+id,StorageId='storage.'+id,QuantityKg=1000,ReservedKg=0))
warehouse('storage.merchant','company.merchant','cleveland')
for c in geo['cities']:warehouse('storage.service.'+c['id'].split('.')[-1],'company.services',c['id'].split('.')[-1])
stocks.extend([dict(Id='stock.player.food',GoodId='good.food',OwnerId='company.player',StorageId='storage.player',QuantityKg=100,ReservedKg=0),dict(Id='stock.merchant.wood',GoodId='good.wood',OwnerId='company.merchant',StorageId='storage.merchant',QuantityKg=2000,ReservedKg=0)])
vehicles=[dict(Id=id,TypeId='vehicle_type.truck',OwnerId=owner,CityId='city.us.'+city,TransportId='',CostRecipientAccountId=service,Bound=False) for id,owner,city in [('vehicle.player','company.player','detroit'),('vehicle.carrier.1','company.carrier','detroit'),('vehicle.carrier.2','company.carrier','cleveland'),('vehicle.merchant','company.merchant','cleveland')]]
def order(id,actor,side,seq):return dict(Id=id,ActorId=actor,MarketId='market.detroit',CityId='city.us.detroit',StorageId='storage.player' if actor=='company.player' else 'storage.service.detroit',GoodId='good.food',Side=side,Status='draft',OriginalKg=10,RemainingKg=10,LimitCentsPerKg=12,Sequence=seq,CreatedStep=0,ExpiresStep=24,ReservedCents=0,ReservedKg=0)
buy=order('example.buy','company.player','buy',1);sell=order('example.sell','company.merchant','sell',2)
trade=dict(Id='example.trade',BuyOrderId='example.buy',SellOrderId='example.sell',BuyerId='company.player',SellerId='company.merchant',MarketId='market.detroit',CityId='city.us.detroit',GoodId='good.food',Step=1,QuantityKg=10,PriceCentsPerKg=12,TotalCents=120)
transport=dict(Id='example.transport',OwnerId='company.player',ProviderId='company.carrier',VehicleId='vehicle.carrier.1',FromCityId='city.us.detroit',ToCityId='city.us.chicago',GoodId='good.food',CargoInventoryId='stock.player.food',DestinationStorageId='storage.service.chicago',Service='rented_truck',Status='draft',Kg=10,DepartureStep=1,ArrivalStep=25,CostCents=1500,ReservedIncomingPoints=0)
scenario=dict(Schema=2,Id=geo['scenario']['id'] if isinstance(geo['scenario'],dict) else geo['scenario'],StartDate='1913-01-01',PlayerCompanyId='company.player',BalancingNotice='All economics, resources, capacities, population, technology parameters and costs are fictional TEST balancing values, not historical claims.',Currency='USD',Settings=dict(Seed=1913,HoursPerStep=1,RealMicrosPerStep=1000000,MaxStepsPerFrame=4,MaxBacklogSteps=8,MaxPendingCommands=256,CommandHorizonSteps=720,StepBudgetMicros=2000),Definitions=dict(Goods=goods,Companies=companies,Needs=needs,Groups=groups,Regions=regions,Places=places,FacilityTypes=facilitytypes,WarehouseTypes=[dict(Id='warehouse.player',CapacityPoints=10000,DailyCostCents=100),dict(Id='warehouse.standard',CapacityPoints=50000,DailyCostCents=300)],VehicleTypes=[dict(Id='vehicle_type.truck',Mode='truck',CapacityKg=2000,SpeedKmPerHour=20,CostCentsPerKm=5,DailyCostCents=100)]),InitialState=dict(Step=0,Accounts=accounts,Companies=[dict(Id=c['Id'],AccountId='account.'+c['Id']) for c in companies],Groups=groupstates,Facilities=facilities,Warehouses=warehouses,Vehicles=vehicles,Inventories=stocks,Orders=[],Trades=[],Transports=[],Offers=[dict(Id='offer.detroit.chicago',ProviderId='company.carrier',VehicleId='vehicle.carrier.1',FromCityId='city.us.detroit',ToCityId='city.us.chicago',AvailableKg=2000,FixedCents=1000,CentsPerKg=5,ExpiresStep=24)],Reservations=[],Regions=[],Countries=[]),ContractExamples=dict(Orders=[buy,sell],Trades=[trade],Transports=[transport]))
scenario['Economy']=dict(Stage=3,PayrollHours=24,DecisionHours=6,DemandHours=24,CostsHours=24,OrderLifetimeHours=12,PickupGraceHours=24,RentalCentsPerPointDay=1,MaxOrderKg=100000,MaxPriceCents=1000000,MaxOpenOrders=512,HistoryLimit=256,DailyBudgetPermille=500,LocalHaulKm=1,RouteDetourPermille=1400,FreightMarkupPermille=200,BookingLeadHours=1,InputTargetHours=48,ProducerReserveKg=100,TraderMaxLoadKg=2000,TraderCashReserveCents=10000,TraderMinProfitCents=100,TraderMarkupPermille=200)
# A declared initial assignment demonstrates the employment relationship; no hiring/wages/production run in AP04.
next(f for f in facilities if f['Id']=='facility.food')['Employment']=[dict(GroupId='group.chicago.low',Workers=20)]
next(g for g in groupstates if g['Id']=='group.chicago.low')['Employed']=20
# Stable ID order, including all runtime rows. No random source and no timestamps.
for section in [scenario['Definitions'],scenario['InitialState'],scenario['ContractExamples']]:
 for rows in section.values():
  if isinstance(rows,list):rows.sort(key=lambda row:row.get('Id',''))
(root/'Content/Simulation/scenario.coisim').write_bytes((json.dumps(scenario,ensure_ascii=False,indent=2)+'\n').encode('utf-8'))
