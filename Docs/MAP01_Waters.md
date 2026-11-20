# MAP01 – Gewässer und Schiffbarkeitsbelege

Planungsumfang: **37 Einträge**, keine aktivierte Transportkante.
Sichtbarkeit ist keine Schiffbarkeit. `supported_corridor` belegt den historischen Verkehrskorridor,
noch kein fertiges kantenweises Netz. `existence_supported` belegt das historische Vorhandensein,
keine unbegrenzte Schiffsklasse oder Kapazität. `not_cleared` bleibt für Fracht gesperrt.
Die sechs optionalen Kanäle werden ohne weiteren Nachweis auch nicht sichtbar importiert.

37 Planungsdatensätze, darunter 5 einzeln benannte Seen; weitere kleine Gewässer nicht pauschal zugesagt. Mehrarmige Flussdatensätze sind explizite Gruppen, die beim Import getrennte Feature-/Abschnitts-IDs benötigen; keine einzige universelle Navigationskante.

| ID / Gewässer | Typ | Sichtbarer MAP02-Umfang | Schiffbarkeitsstatus / Abschnitt | Quellen | Grenze / nächste Prüfung |
|---|---|---|---|---|---|
| `water.1913.rhine` / Rhein / Rijn | river | vorgesehen | supported_corridor: Köln–Emmerich–Lobith; Waal–Merwede–Rotterdamer Hafengebiet separat routen. | eb1911.rhine, eb1911.zuid_holland | Erster MAP04-Korridor. Deltaarme, Fließrichtung und Hafenanschlüsse topologisch festlegen; kein gerader Seeweg über Land. |
| `water.1913.waal_merwede` / Waal / Merwede / Nieuwe Maas | river | vorgesehen | supported_corridor: Niederländischer Delta-Abschnitt des Rotterdam–Köln-Korridors. | eb1911.rhine, eb1911.holland, eb1911.zuid_holland | Nicht mit Nederrijn/Lek zu einer unspezifischen Linie verschmelzen. Fahrwasser im Hafengebiet historisch prüfen. |
| `water.1913.elbe` / Elbe | river | vorgesehen | candidate_requires_segment_review: Magdeburg–Hamburg/Altona als späterer Kandidat. | eb1911.elbe | Dresden ist kein zusätzlicher sächsischer Hub; Leipzig braucht kostenpflichtigen Landzubringer. |
| `water.1913.oder` / Oder | river | vorgesehen | candidate_requires_segment_review: Breslau–Stettin als späterer Kandidat. | eb1911.oder | Nicht alle Nebenarme freigeben; historische Ausbauten und Wasserstände offen. |
| `water.1913.weser` / Weser | river | vorgesehen | candidate_requires_segment_review: Bremen–Wesermündung als späterer Kandidat. | eb1911.weser | Bremerhaven nur Hafen-/Terminalknoten, keine zweite Wirtschaftsregion. |
| `water.1913.merwede_canal` / Merwedekanaal (Verlauf 1913) | canal | vorgesehen | existence_supported: Amsterdam–Utrecht–Vreeswijk–Gorinchem; 1892-Verbindung. | rws.merwede, eb1911.holland | Heutigen Verlauf/Querschnitt nicht kopieren; kein moderner Amsterdam-Rijnkanaal. |
| `water.1913.kiel_canal` / Kaiser-Wilhelm-Kanal | canal | vorgesehen | existence_supported: Brunsbüttel–Kiel-Holtenau; Eröffnung 1895. | dhm.kiel | 1913 noch nicht Maße der 1914 beendeten Erweiterung voraussetzen; Kiel-Holtenau als Terminal des Kieler Hubs, Zugang und Zubringer extra prüfen. |
| `water.1913.dortmund_ems` / Dortmund-Ems-Kanal | canal | vorgesehen | existence_supported: Dortmund–Ems-System; historische Anbindung belegt. | eb1911.westfalen, eb1911.canal | Schleusen, Streckenführung und Fahrzeuggrenzen vor Netznutzung prüfen. |
| `water.1913.north_sea` / Nordsee | sea | vorgesehen | not_cleared: Sichtbarer Küstenkontext. | map.de.1911, map.nl.1911 | Keine pauschale Küstentransportkante oder freie Abkürzung. |
| `water.1913.baltic` / Ostsee | sea | vorgesehen | not_cleared: Schleswig bis Ostpreußen; historische deutsche Küste. | map.de.1911 | Kein Landweg durch Dänemark; keine kostenlose Außenroute. |
| `water.1913.wadden` / Wattenmeer | sea | vorgesehen | not_cleared: Niederländische und deutsche Insel-/Wattküste. | map.de.1911, map.nl.1911 | Wattflächen sind keine schiffbaren Flächen; Fahrrinnen separat belegen. |
| `water.1913.zuiderzee` / Zuiderzee | sea | vorgesehen | not_cleared: Historische offene Meeresbucht. | map.nl.1911, rws.zuiderzee | Kein IJsselmeer, kein Afsluitdijk, kein Flevoland oder Noordoostpolder 1913. |
| `water.1913.bodensee` / Bodensee | lake | vorgesehen | not_cleared: Sichtbare Wasserfläche, keine wirtschaftliche Zusatzregion. | map.de.1911 | Keine Schiffbarkeit aus Sichtbarkeit ableiten; grenzüberschreitende Wasserzugehörigkeit separat modellieren. |
| `water.1913.mueritz` / Müritz | lake | vorgesehen | not_cleared: Sichtbare Wasserfläche, keine wirtschaftliche Zusatzregion. | map.de.1911 | Keine Schiffbarkeit aus Sichtbarkeit ableiten; grenzüberschreitende Wasserzugehörigkeit separat modellieren. |
| `water.1913.schweriner_see` / Schweriner See | lake | vorgesehen | not_cleared: Sichtbare Wasserfläche, keine wirtschaftliche Zusatzregion. | map.de.1911 | Keine Schiffbarkeit aus Sichtbarkeit ableiten; grenzüberschreitende Wasserzugehörigkeit separat modellieren. |
| `water.1913.spirdingsee` / Spirdingsee (Śniardwy) | lake | vorgesehen | not_cleared: Sichtbare Wasserfläche, keine wirtschaftliche Zusatzregion. | map.de.1911 | Keine Schiffbarkeit aus Sichtbarkeit ableiten; grenzüberschreitende Wasserzugehörigkeit separat modellieren. |
| `water.1913.steinhuder_meer` / Steinhuder Meer | lake | vorgesehen | not_cleared: Sichtbare Wasserfläche, keine wirtschaftliche Zusatzregion. | map.de.1911 | Keine Schiffbarkeit aus Sichtbarkeit ableiten; grenzüberschreitende Wasserzugehörigkeit separat modellieren. |
| `water.1913.maas` / Maas | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.ijssel` / IJssel | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.nederrijn_lek` / Nederrijn / Lek | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.ems` / Ems | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.danube` / Donau | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.main` / Main | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.mosel` / Mosel | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.saale` / Saale | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.spree_havel` / Spree / Havel | river | vorgesehen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.neckar` / Neckar | river | optional, noch ausgeschlossen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.vistula` / Weichsel | river | optional, noch ausgeschlossen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.pregel` / Pregel | river | optional, noch ausgeschlossen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.memel` / Memel | river | optional, noch ausgeschlossen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.scheldt` / Schelde / Zeeland-Arme | river | optional, noch ausgeschlossen | not_cleared: Nur sichtbarer Hauptverlauf im Ausschnitt. | map.de.1911, map.nl.1911 | Historische Frachtabschnitte nicht pauschal recherchiert/freigegeben. Keine moderne Kanalisierung voraussetzen. |
| `water.1913.north_sea_canal` / Noordzeekanaal | canal | optional, noch ausgeschlossen | not_cleared: Detailgeometrie/1913-Betriebsnachweis vor optionaler Aufnahme prüfen. | eb1911.canal | In MAP02 nur nach explizitem historisch-geometrischem Nachweis sichtbar aufnehmen; derzeit auf der Prüfliste. |
| `water.1913.elbe_luebeck` / Elbe-Lübeck-Kanal | canal | optional, noch ausgeschlossen | not_cleared: Detailgeometrie/1913-Betriebsnachweis vor optionaler Aufnahme prüfen. | eb1911.canal | In MAP02 nur nach explizitem historisch-geometrischem Nachweis sichtbar aufnehmen; derzeit auf der Prüfliste. |
| `water.1913.finow` / Finowkanal | canal | optional, noch ausgeschlossen | not_cleared: Detailgeometrie/1913-Betriebsnachweis vor optionaler Aufnahme prüfen. | eb1911.canal | In MAP02 nur nach explizitem historisch-geometrischem Nachweis sichtbar aufnehmen; derzeit auf der Prüfliste. |
| `water.1913.oder_spree` / Oder-Spree-Kanal | canal | optional, noch ausgeschlossen | not_cleared: Detailgeometrie/1913-Betriebsnachweis vor optionaler Aufnahme prüfen. | eb1911.canal | In MAP02 nur nach explizitem historisch-geometrischem Nachweis sichtbar aufnehmen; derzeit auf der Prüfliste. |
| `water.1913.ems_jade` / Ems-Jade-Kanal | canal | optional, noch ausgeschlossen | not_cleared: Detailgeometrie/1913-Betriebsnachweis vor optionaler Aufnahme prüfen. | eb1911.canal | In MAP02 nur nach explizitem historisch-geometrischem Nachweis sichtbar aufnehmen; derzeit auf der Prüfliste. |
| `water.1913.zuid_willemsvaart` / Zuid-Willemsvaart | canal | optional, noch ausgeschlossen | not_cleared: Detailgeometrie/1913-Betriebsnachweis vor optionaler Aufnahme prüfen. | eb1911.canal | In MAP02 nur nach explizitem historisch-geometrischem Nachweis sichtbar aufnehmen; derzeit auf der Prüfliste. |

## Ausdrücklich ausgeschlossen

- **Durchgehender Mittellandkanal**: 1913 keine durchgehende fertiggestellte West-Ost-Verbindung; keine spätere Geometrie aktivieren.
- **Amsterdam-Rijnkanaal (moderner Ausbau)**: Erst 1952; stattdessen historischen Merwedekanaal prüfen.
- **Moderner Rhein-Main-Donau-Kanal**: Keine 1913-Verbindung nach modernem Verlauf.
- **Twentekanaal**: Kein 1913-Netzbestand; nicht aus modernen Geodaten übernehmen.

## Erster geplanter Wasserweg

`routeplan.1913.rotterdam_koeln`: `city.nl.1913.zuid_holland` → `city.de.1913.rheinprovinz`.

Verlauf: Rotterdamer Binnen-/Seehafenanschluss → Nieuwe Maas / Merwede / Waal → Lobith / Emmerich → Köln Rheinhafen.

Explizite DE/NL-Zugangsfreigabe als Spielregel in MAP04; kein freier Transit in Hintergrundländern.

Kapazität, Schiffstyp, Tiefgang, Geschwindigkeit, Kosten, Umschlag und Leerfahrt erst in MAP04/05 konfigurieren; keine historischen Zahlen erfinden.

Hafenknoten und Flusskreuzungen sind keine zusätzlichen Wirtschaftshubs oder Märkte.
