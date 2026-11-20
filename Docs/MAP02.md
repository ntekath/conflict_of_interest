# MAP02 – historischer Geografieimport

**Aktueller Folgestand:** [MAP03](MAP03.md) aktiviert die Wirtschaft auf unveränderter Geometrie. Die nachfolgende Beschreibung dokumentiert die damalige MAP02-Geografievorschau; aktuelle Start-/Szenario-/Save-Vorgaben stehen in MAP03.

Stand: 20.11.2026. Deutschland/Niederlande, 01.01.1913. Die MAP01-IDs bleiben erhalten: zwei Länder, 39 deutsche und elf niederländische Verwaltungsregionen, genau 50 Hubs. Implementiert ist die geografische Vorschau. Das frische Wirtschaftsszenario folgt ausdrücklich mit MAP03; Schiffbarkeit und Transportnetz folgen mit MAP04/05.

## Start und Datenfluss

`ConflictOfInterest.uproject` öffnen, Standardlevel `/Game/Maps/DE_NL1913`, Play. Regionen/Länder umschalten, Hub oder Fläche auswählen; Land/Wasser und politische Zugehörigkeit werden getrennt angezeigt. Auswahl bleibt beim Layerwechsel bestehen. Mausrad/WASD bedienen die vorhandene Kamera; das Panel scrollt unabhängig von ihr. Namen werden abhängig von Zoom, Platz und Panelüberdeckung angezeigt.

Die bestehende Wirtschaft ist weiterhin durch Öffnen von `/Game/Maps/GreatLakes1913` erreichbar. Europa startet keine US-Firmen oder US-Konten. Keine neuen Wirtschaftsbestände, Debugzugaben, Save-Versionen oder Migrationen in MAP02. Die früheren US-Kataloge dienen weiterhin den Regressionstests.

`GeoData/Plans/de_nl_1913/manifest.json` definiert Hierarchie, IDs und begründete Hubwahl. Der neue Importer liest ausschließlich eingecheckte Quelldateien unter `GeoData/Raw/MAP02`, prüft zuerst deren SHA256 gegen `sources.lock.json` und schreibt `GeoData/Generated/de_nl_1913/{map,validation}.json`. Das Editor-Commandlet erstellt daraus den Katalog, statische Meshes und das Level. Der Katalog enthält Definition und Provenienz; sämtliche Laufzeitreferenzen werden gekocht. Im Spiel sind weder Python, Netzwerk noch lose Roharchive erforderlich.

## Kostenlose Quellen und Lizenzen

Die neuen, konkret geprüften MAP02-Quellen ergänzen den MAP01-Rechercheplan. Dessen offene Lizenz-/Geometrielücken sind nicht stillschweigend als gelöst angenommen worden.

| Bestandteil | Quelle und Datenstand | Lizenz / Verwendung |
| --- | --- | --- |
| Deutsche Regionen | OpenHistoricalMap, 39 datierte Relationen aus `region_sources.json`, Snapshot 20.11.2026 inklusive rekursiver Elemente, Versionen und Tags | [OHM-Copyright](https://wiki.openstreetmap.org/wiki/OpenHistoricalMap/Copyright): CC0 als Projektstandard; verwendete explizite Elementlizenzen werden geprüft. Historische Rekonstruktion mit erhaltenen `fixme`, `note` und Quellenhinweisen. |
| Niederländische Provinzen | O.W.A. Boonstra (2007), [NLGIS, DANS V2.2](https://doi.org/10.17026/DANS-XB9-T677), PROV1890 mit Vergleich PROV1920 | CC0 laut archivierter DANS-Metadaten. 1890 ist eine ausdrücklich deklarierte kartografische Näherung für 1913. |
| Historische Wassergeometrie | NordNordWest, [German Empire 1914 adm location map.svg](https://commons.wikimedia.org/wiki/File:German_Empire_1914_adm_location_map.svg) | [CC BY-SA 3.0 Deutschland](https://creativecommons.org/licenses/by-sa/3.0/de/). Wassergeometrie extrahiert, projiziert, vereinfacht und trianguliert; diese Bearbeitungen unter derselben Lizenz weitergeben. Attribution im Lock, Bericht und gekochten Katalog. |
| Hubkoordinaten | [GeoNames cities500](https://download.geonames.org/export/dump/), gepinnter Snapshot 20.11.2026 | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Originale Punktkoordinaten/IDs; moderne Bevölkerungszahlen werden nicht übernommen. Historische Hubwahl/Namen kommen aus MAP01. |
| Nachbarland / niederländische Flusslinien | [Natural Earth](https://www.naturalearthdata.com/about/terms-of-use/), 1:10m; Rivers 5.0.0 | Public Domain. Moderne Landesgrenzen werden aufgelöst, ausschließlich physischer Nachbarkontext. Die heutige niederländische Landfläche wird durch historische Provinzen ersetzt. Sechs explizit freigegebene Rhein-/Delta-/Maas-Quellobjekte ergänzen die Linien westlich 5,5°E als generalisierte Näherung. |
| Historischer Vergleich NL-Gewässer | Encyclopaedia Britannica 1911, Bd.13, Holland-Tafel nach S.588, [Originaltafel](https://commons.wikimedia.org/wiki/File:EB1911_-_Holland.jpg) | Public Domain; visuell geprüfter Referenzbeleg, keine vermessungsgleiche Linienquelle. |

Alle Originaldateien, DANS-Metadaten, URLs und Hashes sind versioniert. `.gitattributes` bewahrt die Rohbytes. Die Wasserquellenlizenz bleibt an den abgeleiteten Wasserflächen/-linien und entsprechenden Meshes erhalten; die Projektcode-Lizenz wird dadurch nicht geändert. Bei Weitergabe die Attribution aus `sources.lock.json` und die dort verlinkten Lizenztexte mitliefern. Das gemischte Datenpaket ist ausdrücklich nicht pauschal Public Domain.

Die Importwerkzeuge verwenden kostenlose Bibliotheken aus `Tools/requirements-map02.txt`: unter anderem Shapely (BSD), pyproj/PROJ und svgpathtools/svgwrite (MIT), NumPy/SciPy (BSD), pyshp (MIT), Pillow (HPND) und certifi (MPL). Bestehende UE-/C++-Toolchain wie bisher.

## Berechnungsregeln und Unsicherheiten

- Quelle WGS84; niederländische Quelldaten sind RD in **Dekametern**, daher vor EPSG:28992→4326 mit zehn multiplizieren. Original-SHP/SHX/DBF bleiben unverändert.
- Sphärische Lambert-Conformal-Conic-Projektion: Ursprung 52°N/13°E, Standardparallelen 48°/55°, Radius 6371,0088 km. **7 UE-Einheiten/km**, X Norden, Y Osten. C++ und unabhängige PROJ-Referenzvektoren werden verglichen. Startkamera 16000, seitlicher Fokusversatz für das Panel; kein Globus/Streaming erforderlich.
- OHM-Ringe werden über originale Knotenreferenzen zusammengesetzt. Nicht geschlossene Ringe führen zum Abbruch. Zwei ungültige niederländische Quellpolygone werden ausdrücklich mit `make_valid` repariert und im Bericht markiert. PROV1890/1920-Abweichungen bleiben pro Region protokolliert.
- Quellpräzision 1 cm; topologiebezogene Vereinfachung mit 200 m Toleranz, Inseln/Exklaven und Löcher bleiben erhalten. Jeder triangulierte Teil wird auf Flächenübereinstimmung und Dreiecke innerhalb des Polygons geprüft.
- Unabhängig generalisierte Quellränder überschneiden sich lokal. `import_config.json` enthält ausschließlich explizite Prioritätspaare mit km²-Höchstwerten; Überschreitungen oder neue Paare brechen ab. Dieselben Regeln gelten nach Vereinfachung, weil schmale Quelllücken sonst überbrückt werden können. Beide tatsächlichen Überlappungsflächen werden in `validation.json` protokolliert. Anschließend dürfen keine flächigen Regionsüberschneidungen verbleiben. Dies ist eine kartografische Auflösung, keine historische Rechtsentscheidung.
- Größter ursprünglicher Binnenkonflikt: Schleswig-Holstein/Hannover ca.11,95 km² am Elberaum. Kleine thüringische Konflikte und deutsche/niederländische Quellnähte sind ebenfalls explizit aufgeführt. Die Grenzlage ist dort keine vermessungsgleiche 1913-Aussage.
- Historische SVG-Georeferenz: 5,5–23,2°E, 47,2–56°N, Viewbox 1424,931×1133,484. Die Landmaske verwendet historische Küsten/Wasser; insbesondere keine modernen Flevoland-Polder. Nachbargebiete sind vereinfachter moderner physischer Kontext, keine simulierten politischen Einheiten.
- Kiel: originaler GeoNames-Punkt liegt **79,088 m** auf der Wasserseite der generalisierten Küste. Einzige konfigurierte Ufertoleranz **150 m**. Der Punkt wird nicht verschoben und kein künstliches Land hinzugefügt. Laufzeitprüfung misst den Abstand erneut; die physische Abfrage darf dort Wasser melden.
- 81 Flusslinienabschnitte, schematische Halbbreite 50 m; keine Aussage über reale Flussbreite. Alle haben `navigable=false`. Masken und Linien besitzen bewusst unterschiedliche Auflösung. Die niederländischen Delta-Proxys und der 1914-Wasserstand sind bekannte zeitliche/geometrische Näherungen. Kein vollständiges historisches Gewässerkataster.

`FCoIGeoMapData::Query` liefert unabhängig `CountryId`, `RegionId`, `WaterId`, `bLand`. Politische Regionen behalten ihre Binnengewässer; exakte gemeinsame Randpunkte werden stabil nach ID aufgelöst. Wasserflächen sind physische IDs, keine zusätzlichen Wirtschaftsregionen. `region_land` dient nur der Darstellung. Die vorhandene Wirtschaftsengine erhält keine Projektions-/Meshabhängigkeit.

## Reproduktion

Ab Repositorywurzel in PowerShell; die eingecheckten Rohquellen benötigen keinen Download:

```powershell
python -m venv Saved/MAP02Python
& Saved/MAP02Python/Scripts/python.exe -m pip install -r Tools/requirements-map02.txt
& Saved/MAP02Python/Scripts/python.exe Tools/import_europe.py
./Tools/Build.ps1 -EngineRoot E:\UE_5.8 -Target Editor
& E:/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe "$PWD/ConflictOfInterest.uproject" -run=CoIBuildMap -Europe -unattended -nop4 -nullrhi
./Tools/Test.ps1 -EngineRoot E:\UE_5.8
& E:/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe "$PWD/ConflictOfInterest.uproject" -run=Cook -TargetPlatform=Windows -Map=/Game/Maps/DE_NL1913 -unattended -nop4 -nullrhi
```

`CoI.RebuildMap` im Editor baut nun Europa. Das Commandlet ohne `-Europe` sowie `Tools/import_geodata.py` behalten den ursprünglichen US-Reproduktionsweg. Keine manuelle Neuerstellung einzelner Regionen erforderlich.

Quellenaktualisierung ist eine bewusste Datenänderung: neue Dateien erst prüfen und den Lock aktualisieren. Ein erneuter Liveabruf ist wegen veränderlicher Providerdaten **kein** Ersatz für den archivierten Snapshot. OHM-Abfrage: `[out:json][timeout:180];relation(id:<IDs aus region_sources.json>);(._;>>;);out meta;`. DANS-Download-API `/api/access/datafile/<id>`: PROV1890 DBF274620/SHP274590/SHX274624, PROV1920 DBF274705/SHP274728/SHX274725. TLS-Zertifikatsprüfung bleibt aktiv.

## Abnahme und Grenzen

Automatische Prüfungen: Quellhashes/Lizenzen/Gültigkeitsintervalle, vollständiges Manifest, gültige Ringe/Polygone/Dreiecke, eindeutige IDs/Referenzen, genau ein korrekt platzierter Hub je Region, getrennte physische/politische Proben. Repräsentative Punkte: Alt-Berlin/Charlottenburg, Zuiderzee, Helgoland, Büsingen, Birkenfeld/Eutin, Cuxhaven/Bremerhaven, Nordsee, belgisches Hintergrundland, Schweriner See und linkes Rheinufer bei Köln. Distanzreferenzen WGS84: Rotterdam–Köln 204,048 km, Berlin–Königsberg 528,609 km; Bildschirmabstände sind keine Transportentfernungen.

Neue Unreal-Tests prüfen die gekochte Katalogstruktur, Projektion, alle Proben, politische Auswahl auf Wasser und atomare Ablehnung beschädigter IDs, Hubzuordnung, Projektion, Koordinaten und unzulässiger Navigationsfreigaben. Bestehende Wirtschaft-/Save-/Kostenrechnungstests laufen mit ihren US-Fixtures weiter. Build-, Test-, Cook- und UI-Nachweise einschließlich der finalen Dateihashes stehen unter `Docs/Verification/MAP02`.

Prüfergebnis: 73/73 Unreal-Tests bestanden; nach der letzten reinen Beschriftungskorrektur nochmals 3/3 Geografietests. Editor, Game Development und Shipping erfolgreich. Windows-Cook: 666 Pakete gekocht, sieben plattformbedingt übersprungen, null Fehler; einzige Warnung ist das fehlende NTFS-Journal des Engine-Laufwerks. Der finale Import wurde zweimal byteidentisch ausgeführt, Hashes siehe `Reproducibility.json`. Sichtprüfung in beiden Auflösungen und Zoomstufen dokumentiert; die bisherige geschätzte Labelbreite bleibt bei dichter Beschriftung eine MAP06-Verfeinerung.

Die vorhandenen 1913-Quellen sind historische Rekonstruktionen mit dokumentierten Näherungen. Eine präzise Küsten-/Grenzvermessung oder belegte Schiffbarkeit wird nicht behauptet. Ein vollständiger gestagter Installer wurde in MAP02 nicht getestet; geprüft werden native Builds, Editor-Spielbetrieb und Windows-Cook der referenzierten Offlineassets. Nächstes Paket: **MAP03**, frisches Wirtschaftsszenario auf diesen IDs.
