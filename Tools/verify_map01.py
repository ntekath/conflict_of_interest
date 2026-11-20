"""Offline MAP01 data-contract checks; no UE, pip or network required.

Does not claim polygon coverage, historical coordinate containment or a runnable
water network: these are explicitly MAP02/MAP04 gates.
"""
import argparse
import copy
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'GeoData/Plans/de_nl_1913'
FILES = ('manifest.json', 'sources.json', 'hub_evidence.json', 'waters.json')


def validate(plan, sources, evidence, waters):
    def require(condition, message):
        if not condition:
            raise ValueError(message)

    require(plan['schema'] == 1 and plan['as_of'] == '1913-01-01', 'Plan version/date')
    require(plan['activation'] == 'planning_only_MAP02_MAP03_required', 'Unexpected runtime activation')
    countries, regions, hubs = plan['countries'], plan['regions'], plan['hubs']
    require(len(countries) == 2 and len(regions) == len(hubs) == 50, 'Scope counts')
    all_ids = [r['id'] for r in countries + regions + hubs + sources + evidence]
    require(len(all_ids) == len(set(all_ids)), 'Duplicate ID')
    require(all(re.fullmatch(r'[a-z0-9_.]+', k) for k in all_ids), 'Non-canonical ID')
    country_ids = {c['id'] for c in countries}
    require(country_ids == {'country.de.1913', 'country.nl.1913'}, 'Country hierarchy')
    require(Counter(r['country_id'] for r in regions) == {
        'country.de.1913': 39, 'country.nl.1913': 11}, 'Country region coverage')
    require(sum(r['historical_parent'] == 'Preußen' for r in regions) == 14, 'Prussian partition')
    require(sum(r['historical_type'] == 'Reichsland' for r in regions) == 1, 'Reichsland missing')
    require(sum(r['historical_parent'] == 'Deutsches Reich' and r['historical_type'] != 'Reichsland'
                for r in regions) == 24, 'Other federal states missing')
    by_hub = {h['id']: h for h in hubs}
    by_region = {r['id']: r for r in regions}
    require(len({r['hub_id'] for r in regions}) == 50, 'Hub shared between regions')
    require(len({h['region_id'] for h in hubs}) == 50, 'Multiple hubs in region')
    require(len({h['name'] for h in hubs}) == 50, 'Duplicate physical hub name')
    source_by_id = {s['id']: s for s in sources}
    evidence_by_id = {e['id']: e for e in evidence}
    require(len(evidence) == 50, 'Missing per-hub evidence')
    source_ids = set(source_by_id) | set(evidence_by_id)
    for row in sources:
        require(all(row.get(k) for k in ('url', 'data_date', 'checked', 'license', 'use',
                                        'accuracy', 'attribution')), 'Incomplete source record')
        require(row['url'].startswith('https://'), 'Source URL')
    def check_refs(refs):
        require(bool(refs), 'Missing source references')
        for ref in refs:
            require(ref in source_ids, f'Unknown source {ref}')
            require(source_by_id.get(ref, {}).get('use') != 'rejected_import',
                    f'Restricted/unresolved source used: {ref}')
    for region in regions:
        require(region['country_id'] in country_ids and region['valid_at'] == plan['as_of'], 'Region country/date')
        require(region['hub_id'] in by_hub, 'Unknown hub')
        require(by_hub[region['hub_id']]['region_id'] == region['id'], 'Region/hub mapping')
        require(region['boundary_status'] == 'pending_digitisation_MAP02', 'Unverified polygon claim')
        require(bool(region['territory_rule']) and bool(region['historical_type']), 'Missing territory/type')
        check_refs(region['sources'])
    for hub in hubs:
        require(hub['region_id'] in by_region and by_region[hub['region_id']]['hub_id'] == hub['id'], 'Hub/region mapping')
        require(hub['id'] != hub['region_id'], 'Hub and region IDs collapsed')
        require(bool(hub['economic_reason']) and bool(hub['alternatives_and_uncertainty']), 'Unreasoned hub')
        require(hub['selection_status'] == 'curated_proposal', 'Unjustified unique historical rank')
        check_refs([hub['source_id'], hub['coordinate_source']] + hub.get('additional_sources', []))
        require(hub['source_id'] in evidence_by_id, 'Missing hub source evidence')
        ev = evidence_by_id[hub['source_id']]
        require(ev['url'] == hub['source_url'] and ev['status'] == 'retrieved', 'Unretrieved/mismatched hub source')
        require(bool(re.fullmatch('[a-f0-9]{64}', ev['page_sha256'])), 'Source digest')
        require(0 < len(ev['evidence_excerpt'].split()) <= 150, 'Missing/overlong excerpt')
        require('.mw-parser' not in ev['evidence_excerpt'], 'Website CSS in evidence')
    # These historically distinct units must never become a parent plus its own children.
    require('region.de.1913.preussen' not in by_region, 'Prussia double-counted')
    require(by_region['region.de.1913.berlin']['hub_id'] == 'city.de.1913.berlin', 'Berlin collapsed into Brandenburg')
    require(by_hub['city.de.1913.brandenburg']['name'] == 'Charlottenburg', 'Brandenburg hub inside excluded Berlin')
    require(by_hub['city.de.1913.schleswig_holstein']['name'] == 'Kiel', 'Unreviewed Schleswig-Holstein hub')
    require(not any('flevoland' in r['id'] for r in regions), 'Anachronistic province')
    projection = plan['projection']
    require(projection['source_crs'] == 'EPSG:4326' and projection['source_axis_order'] == 'longitude,latitude', 'WGS84 contract')
    require(projection['bbox_wgs84'] == [2.5, 47, 23.5, 56.5], 'Scope extent')
    require(projection['units_per_km'] == 7 and projection['parallel_1'] < projection['parallel_2'], 'Projection scale/parallels')
    water_rows = waters['waterbodies']
    require(len(water_rows) == 37 and len({w['id'] for w in water_rows}) == 37, 'Water scope/IDs')
    require(len(waters['excluded']) == 4, 'Missing exclusion list')
    for water in water_rows:
        require(water['network_enabled'] is False, 'MAP01 must not activate transport')
        require(water['navigation_evidence'] in {'supported_corridor', 'candidate_requires_segment_review',
                'existence_supported', 'not_cleared'}, 'Unclassified navigability')
        require(water['kind'] != 'canal' or water['navigation_evidence'] != 'not_cleared'
                or water['visible_in_MAP02'] is False, 'Unverified canal made visible')
        require('population' not in water and 'market_id' not in water, 'Water/terminal duplicates economic hub')
        check_refs(water['evidence_sources'])
        check_refs(water['geometry_sources'])
    route = waters['first_route']
    require(route['from_hub'] in by_hub and route['to_hub'] in by_hub, 'Unknown water route endpoint')
    require(by_region[by_hub[route['from_hub']]['region_id']]['country_id'] !=
            by_region[by_hub[route['to_hub']]['region_id']]['country_id'], 'Missing international test route')
    require(route['state'] == 'historically_supported_not_yet_graph', 'Unbuilt network claim')
    check_refs(route['evidence_sources'])


def self_test(data):
    mutations = [
        ('shared hub', lambda p, s, e, w: p['regions'][1].update(hub_id=p['regions'][0]['hub_id'])),
        ('orphan region', lambda p, s, e, w: p['regions'][0].update(country_id='country.unknown')),
        ('missing region', lambda p, s, e, w: p['regions'].pop()),
        ('restricted source', lambda p, s, e, w: p['regions'][0]['sources'].append('rejected.mpidr')),
        ('missing evidence', lambda p, s, e, w: e.pop()),
        ('early network', lambda p, s, e, w: w['waterbodies'][0].update(network_enabled=True)),
        ('unverified canal', lambda p, s, e, w: w['waterbodies'][-1].update(visible_in_MAP02=True)),
        ('second population', lambda p, s, e, w: w['waterbodies'][0].update(population=100)),
    ]
    for name, mutate in mutations:
        case = copy.deepcopy(data)
        mutate(*case)
        try:
            validate(*case)
        except ValueError:
            continue
        raise ValueError(f'Negative check failed: {name}')
    return [name for name, _ in mutations]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--write-report', action='store_true')
    args = parser.parse_args()
    data = [json.loads((PLAN / name).read_text(encoding='utf-8')) for name in FILES]
    validate(*data)
    negative = self_test(data) if args.self_test else []
    report = dict(package='MAP01', result='PASS', counts=data[0]['expected_counts'],
                  waters=37, excluded_waterways=4, per_hub_sources=50,
                  negative_checks_passed=negative,
                  inputs_sha256={name: hashlib.sha256((PLAN / name).read_bytes()).hexdigest() for name in FILES},
                  limitations=['Logical hierarchy only; polygons not yet digitised or validated.',
                               'Hub coordinates and historic containment remain MAP02.',
                               'Water geometry is not a navigable transport graph.',
                               'Runtime, economics and save format unchanged.'])
    output = json.dumps(report, ensure_ascii=False, indent=2) + '\n'
    if args.write_report:
        (ROOT / 'Docs/Verification/MAP01.json').write_text(output, encoding='utf-8', newline='\n')
    print(output)


if __name__ == '__main__':
    main()
