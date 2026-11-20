"""Optional free online research. Writes review drafts in Saved, never replaces the plan.

python Tools/collect_map01_evidence.py
Review the resulting excerpts before copying them to hub_evidence.json.
Only historical public-domain text paragraphs are excerpted, not site chrome.
"""
import concurrent.futures
import hashlib
import json
import re
import urllib.request
from html.parser import HTMLParser
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'GeoData/Plans/de_nl_1913'


class Paragraphs(HTMLParser):
    def __init__(self):
        super().__init__()
        self.depth = self.skip = 0
        self.out = []

    def handle_starttag(self, tag, attrs):
        if tag in ('script', 'style'):
            self.skip += 1
        if tag == 'p':
            self.depth += 1

    def handle_endtag(self, tag):
        if tag in ('script', 'style'):
            self.skip = max(0, self.skip - 1)
        if tag == 'p':
            self.depth = max(0, self.depth - 1)
            self.out.append('\n')

    def handle_data(self, data):
        if self.depth and not self.skip:
            self.out.append(data)


def fetch(hub):
    if not hub['source_url'].startswith('https://en.wikisource.org/wiki/1911_'):
        return dict(id=hub['source_id'], url=hub['source_url'],
                    status='manual_review_required',
                    note='Only facts may be paraphrased; no automatic extraction of copyrighted text.')
    request = urllib.request.Request(hub['source_url'], headers={
        'User-Agent': 'ConflictOfInterest-MAP01-research/1.0'})
    with urllib.request.urlopen(request, timeout=30) as response:
        raw = response.read()
    parser = Paragraphs()
    parser.feed(raw.decode('utf-8'))
    text = ''.join(parser.out)
    sentences = re.split(r'(?<=[.!?])\s+', text)
    economic = [s for s in sentences if re.search(
        r'\b(?:industr\w*|manufact\w*|trade|commerce|commercial|shipbuilding|'
        r'optical|porcelain|toys?|linen|wool\w*|spa|baths?)\b', s, re.I)]
    excerpt = ' '.join(' '.join(economic or [text]).split()[:150])
    cache = ROOT / 'Saved/MAP01Research'
    cache.mkdir(parents=True, exist_ok=True)
    (cache / (hub['source_id'] + '.html')).write_bytes(raw)
    (cache / (hub['source_id'] + '.txt')).write_text(text, encoding='utf-8', newline='\n')
    return dict(id=hub['source_id'], url=hub['source_url'],
                work_date='1911; Einzeldaten oft älter',
                page_sha256=hashlib.sha256(raw).hexdigest(),
                evidence_excerpt=excerpt, status='retrieved',
                license='Historischer Text Public Domain; keine Website-Gestaltung übernommen',
                interpretation='Belegt Funktionen, nicht mathematisch eindeutigen Rang. Hubwahl ist kuratierter Vorschlag.')


if __name__ == '__main__':
    from datetime import date
    hubs = json.loads((PLAN / 'manifest.json').read_text(encoding='utf-8'))['hubs']
    # Two concurrent requests, no automated retries on provider errors/rate limits.
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        rows = list(pool.map(fetch, hubs))
    for row in rows:
        row['retrieved'] = date.today().isoformat()
    output = ROOT / 'Saved/MAP01Research/hub_evidence_draft.json'
    output.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'{len(rows)} source drafts: {output}')
