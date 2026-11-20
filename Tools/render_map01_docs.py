"""Render review tables from MAP01 data, or --check that they are current."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'GeoData/Plans/de_nl_1913'


def cell(value):
    return str(value).replace('|', '\\|').replace('\n', ' ')


def tables():
    plan = json.loads((PLAN / 'manifest.json').read_text(encoding='utf-8'))
    sources = json.loads((PLAN / 'sources.json').read_text(encoding='utf-8'))
    waters = json.loads((PLAN / 'waters.json').read_text(encoding='utf-8'))
    hubs = {h['id']: h for h in plan['hubs']}
    lines = ['# MAP01 – Vollständige Regions- und Hubliste', '',
             'Generiert aus `GeoData/Plans/de_nl_1913/manifest.json`. Stichtag aller Zeilen: **01.01.1913**.', '',
             '39 deutsche und 11 niederländische Regionen. Jeder Hub ist ein wirtschaftlich begründeter Vorschlag,',
             'kein behaupteter eindeutiger statistischer Rang. Quellenbelege: `hub_evidence.json`.',
             'RegionId und HubId sind getrennt; vollständige IDs und historische Eltern stehen im Manifest.',
             'Alle Exklaven bleiben beim jeweiligen Gebiet. [Methodik und Grenzen](MAP01.md).', '',
             '| Land / historischer Elternstaat | Region / historischer Typ | Hub | Wirtschaftlicher Grund | Alternativen / Unsicherheit | Quelle |',
             '|---|---|---|---|---|---|']
    for r in plan['regions']:
        h = hubs[r['hub_id']]
        cols = [r['historical_parent'], r['name'] + ' / ' + r['historical_type'], h['name'],
                h['economic_reason'], h['alternatives_and_uncertainty'],
                f"[Beleg]({h['source_url']})" + ('; ergänzende Quellen im Manifest' if h.get('additional_sources') else '')]
        lines.append('| ' + ' | '.join(map(cell, cols)) + ' |')
    region_text = '\n'.join(lines) + '\n'
    lines = ['# MAP01 – Quellen- und Lizenzregister', '',
             'Geprüft am 20.11.2026. Frei zugänglich bedeutet nicht automatisch frei importierbar.',
             '`approved_digitisation`: historische Originalkarte für eigene Digitalisierung.',
             '`historical_reference` und `facts_only_no_asset_copy`: Tatsachenbeleg, keine automatische Assetfreigabe.',
             '`approved_context_only`: moderne Hilfsdaten mit historischem Abgleich.',
             '`rejected_import`: nicht als Import-/Abzeichnungsvorlage verwenden.', '',
             'Originalraster und neue Koordinatendumps sind noch nicht gepinnt; keine erfundenen Datei-Hashes.',
             'Siehe [Arbeitsgates und Reproduktion](MAP01.md). Per-Hub-URLs, Auszüge und Seitendigests separat',
             'in `hub_evidence.json`; 49 historische Textbelege und eine eigene kommunale Faktenparaphrase.', '',
             '| Quellen-ID / Link | Datenstand | Rechte / Attribution | Verwendung | Genauigkeit / Grenze |',
             '|---|---|---|---|---|']
    for s in sources:
        cols = [f"`{s['id']}` – [{s['title']}]({s['url']})", s['data_date'],
                s['license'] + '. ' + s['attribution'], s['use'], s['accuracy']]
        lines.append('| ' + ' | '.join(map(cell, cols)) + ' |')
    source_text = '\n'.join(lines) + '\n'
    lines = ['# MAP01 – Gewässer und Schiffbarkeitsbelege', '',
             'Planungsumfang: **37 Einträge**, keine aktivierte Transportkante.',
             'Sichtbarkeit ist keine Schiffbarkeit. `supported_corridor` belegt den historischen Verkehrskorridor,',
             'noch kein fertiges kantenweises Netz. `existence_supported` belegt das historische Vorhandensein,',
             'keine unbegrenzte Schiffsklasse oder Kapazität. `not_cleared` bleibt für Fracht gesperrt.',
             'Die sechs optionalen Kanäle werden ohne weiteren Nachweis auch nicht sichtbar importiert.', '',
             waters['scope_note'], '',
             '| ID / Gewässer | Typ | Sichtbarer MAP02-Umfang | Schiffbarkeitsstatus / Abschnitt | Quellen | Grenze / nächste Prüfung |',
             '|---|---|---|---|---|---|']
    for w in waters['waterbodies']:
        cols = [f"`{w['id']}` / {w['name']}", w['kind'],
                'vorgesehen' if w['visible_in_MAP02'] else 'optional, noch ausgeschlossen',
                w['navigation_evidence'] + ': ' + w['documented_extent'],
                ', '.join(w['evidence_sources']), w['note']]
        lines.append('| ' + ' | '.join(map(cell, cols)) + ' |')
    lines += ['', '## Ausdrücklich ausgeschlossen', '']
    for x in waters['excluded']:
        lines.append(f"- **{x['name']}**: {x['reason']}")
    route = waters['first_route']
    lines += ['', '## Erster geplanter Wasserweg', '',
              f"`{route['id']}`: `{route['from_hub']}` → `{route['to_hub']}`.", '',
              'Verlauf: ' + ' → '.join(route['via']) + '.', '', route['access_policy'], '',
              route['parameters'], '',
              'Hafenknoten und Flusskreuzungen sind keine zusätzlichen Wirtschaftshubs oder Märkte.']
    return {'MAP01_Regions.md': region_text, 'MAP01_Sources.md': source_text,
            'MAP01_Waters.md': '\n'.join(lines) + '\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for name, expected in tables().items():
        path = ROOT / 'Docs' / name
        if args.check:
            if not path.exists() or path.read_text(encoding='utf-8') != expected:
                raise SystemExit(f'Stale/missing table: {path}')
        else:
            path.write_text(expected, encoding='utf-8', newline='\n')
    print('PASS: 3 MAP01 review tables match data.' if args.check else 'Rendered 3 MAP01 review tables.')
