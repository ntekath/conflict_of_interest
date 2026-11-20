# MAP01 – Historischer Datenplan Deutschland/Niederlande 1913

Stand: 20.11.2026. Grundlage ist das vollständig gelesene Arbeitsdokument
`Handelsspiel_UE5_Prompt_Arbeitspakete(8).md`, unverändert als
`Docs_DevelopmentBrief.md` übernommen. Beauftragt und umgesetzt ist MAP01.
Dies ist der recherchierte Daten-/Integrationsplan, noch kein Kartenimport.

## Festgelegter Umfang

Stichtag **01.01.1913**, europäische Heimatgebiete des Deutschen Reichs und der
Niederlande, einschließlich Inseln und Exklaven. Keine Kolonien oder anderen
Überseegebiete. Zwei simulierte Länder, **50 Regionen und genau 50 Hubs**:

- Deutschland: zwölf preußische Provinzen, Berlin als eigene Vergleichseinheit
  und die Hohenzollernschen Lande als Sonderbezirk: zusammen 14.
- Weitere 24 deutsche Gliedstaaten, jeweils als ganze Spieleinheit.
- Elsass-Lothringen als Reichsland: insgesamt 39 deutsche Regionen.
- Niederlande: die elf damaligen Provinzen; kein Flevoland.

Die preußischen Einheiten ersetzen für die wirtschaftliche Einteilung den
Gesamtstaat Preußen. Dieser wird **nicht zusätzlich als Region** angelegt.
Preußen bleibt historische Elterninformation unter dem Land Deutsches Reich,
kein weiterer nationaler Wirtschaftskreislauf. Keine Mischung mit überlappenden
Regierungsbezirken. Hohenzollern ist ausdrücklich kein gewöhnlicher Provinztyp.
Das Prinzip folgt der zeitgenössischen [Preußenbeschreibung](https://en.wikisource.org/wiki/1911_Encyclop%C3%A6dia_Britannica/Prussia).

**Vollständige Einteilung, Typen, Hubs, Gründe, Alternativen und Quellen:**
[Regionsliste](MAP01_Regions.md). Maschinenlesbarer Vertrag:
`GeoData/Plans/de_nl_1913/manifest.json`. Alle Einheiten sind am Stichtag gültig;
das ist keine Behauptung, dass sie erst 1913 gegründet wurden oder jede Grenze
über das gesamte Jahr unverändert blieb. Historische Geometrie wird im nächsten
Paket gegen den Stichtag geprüft.

Besonders zu beachten:

- Berlin vor Groß-Berlin 1920 separat; Charlottenburg ist Brandenburgs Hub,
  keine zweite Berliner Wirtschaftsregion.
- Altona und Harburg gehören 1913 nicht zu Hamburg. Schleswig-Holsteins Hub
  ist Kiel wegen Werften, Industrie und Handel; Altona bleibt Alternative.
- Hamburg einschließlich seiner damaligen Außengebiete/Cuxhaven und Bremen
  einschließlich Bremerhaven erhalten jeweils nur einen Wirtschaftshub.
- Oldenburg einschließlich Birkenfeld und Fürstentum Lübeck. Letzteres gehört
  nicht zur Freien und Hansestadt Lübeck.
- Bayerische Pfalz, Hohenzollern, thüringische Ober-/Unterherrschaften und
  Exklaven bleiben beim jeweiligen historischen Gebiet. Sachsen-Coburg und
  Gotha wird bewusst als eine gemeinsame wirtschaftliche Spieleinheit geführt.
- Waldeck-Pyrmont wird trotz preußischer Verwaltungsverbindung nicht nochmals
  als preußische Provinz gezählt. Büsingen, Helgoland und kleine Inseln müssen
  beim späteren Polygonabgleich erhalten bleiben.
- Elsass-Lothringen, Posen, West-/Ostpreußen, Nordschleswig und andere damalige
  Reichsgebiete werden nicht anhand heutiger Staaten ausgeschnitten.
- Nachbarstaaten sind höchstens Hintergrund. Es gibt keinen automatischen
  Transit durch Belgien, Luxemburg, Frankreich, Dänemark, Russland,
  Österreich-Ungarn oder die Schweiz.

## Hubauswahl und Belegstärke

Die Auswahl gewichtet Industrie/Handel, urbanen Markt und Logistik. Ein
Regierungssitz gewinnt nicht automatisch: etwa Mannheim statt Karlsruhe,
Frankfurt statt Kassel, Rostock statt Schwerin und Hechingen statt Sigmaringen.
Die Gründe beruhen überwiegend auf zeitgenössischen Ortsbeschreibungen;
`hub_evidence.json` enthält 49 kurze historische Textauszüge und eine eigene
Paraphrase kommunaler Quellen, jeweils mit URL, Abrufdatum und Seitendigest.
Keine übernommenen modernen Fotos, Karten oder langen Website-Texte.

Es gibt keine vollständige, einheitlich abgegrenzte Beschäftigten-/Umsatzrangliste
aller Städte für genau 1913. Die Hubs sind deshalb **begründete Vorschläge**, kein
statistisch bewiesenes Maximum. Bei München/Nürnberg, Leipzig/Dresden/Chemnitz,
Köln/Essen/Duisburg, Jena/Eisenach, Oldenburg/Delmenhorst, Neubrandenburg/Neustrelitz,
Wildungen und Stadthagen sind Gewichtung bzw. Datenlücken ausdrücklich sichtbar.
Stadthagens Quelle beschreibt vor allem die Landeswirtschaft, keinen gesicherten
Vorsprung vor Bückeburg. Jenas EB-Artikel bewertet das Gewerbe niedrig;
die [Stadtchronik](https://geschichte.jena.de/de/kurzchronik) belegt Zeiss/Schott
und den Bahnanschluss. Diese Abweichung wird nicht versteckt.
Historische Bevölkerungszahlen aus den Beschreibungen werden nicht als neue
Simulationsbevölkerung importiert.

## Quellen und Importfreigabe

[Quellenregister](MAP01_Sources.md) und `sources.json` unterscheiden:
freigegebene historische Kartengrundlagen, moderne Koordinaten-/Kontextdaten,
reine Tatsachenreferenzen und ausgeschlossene Quellen. Alle Informationen sind
frei zugänglich; kein Kauf, Abonnement, API-Schlüssel oder kostenpflichtiger Dienst.

Nationale und regionale Grenzen werden auf Grundlage der historischen
Britannica-Karten von 1911 rekonstruiert, nicht aus heutigen Bundesländern.
Die Karten sind gemeinfreie historische Werke; Attribution wird freiwillig
mitgeführt. Für Deutschland ist Emery Walker (1851–1933) angegeben; die
Holland-Karte ist anonym veröffentlicht. Die Commons-Dateiseiten kennzeichnen
den US-Gemeinfreiheitsstatus; der Ablauf der Schutzfrist des historischen Werks
ist von der Lizenz des Commons-Webseitentexts zu unterscheiden. Nur die alten
Karten/selbst erstellte Digitalisierung verwenden, nicht den Webseitentext.

Die verfügbaren Übersichtsscans reichen **nicht** für ungeprüft genaue kleine
Enklaven oder Alt-Berlin. Sie sind eine erlaubte Ausgangsquelle, kein fertiges
präzises GIS. Vor der Abnahme von MAP02 müssen Detaillücken mit weiteren
geeigneten historischen Originalkarten geschlossen bzw. nachvollziehbare
Vereinfachungen je Feature ausgewiesen werden.

MPIDR/Mosaic ist auf nichtkommerzielle wissenschaftliche Nutzung begrenzt.
CShapes-Daten sind CC BY-NC-SA, unabhängig von der Lizenz des R-Paketcodes.
Beide werden nicht importiert oder als Abzeichnungsvorlage verwendet.
NLGIS ist ohne hinreichend bestätigte Vertriebsrechte ebenfalls nicht freigegeben.
Die Dokumentation dieser Konflikte ist Teil der MAP01-Abnahme.

## Vorhandener Code und kleinste Änderung ab MAP02

| Bestand | Tatsächliche Bindung | Gezielte folgende Anpassung |
|---|---|---|
| `Tools/import_geodata.py` | USA/Kanada, fünf Große Seen, vier Städte, Voronoi-Spielregionen | Szenarioparameter, historische Regionflächen und getrennte politische/physische Geometrie einlesen; vorhandene Vereinfachung/Triangulierung weiterverwenden |
| `GeoData/scenario.json`, `Generated/map.json` | Aktiver Great-Lakes-Katalog | Separater DE/NL-Import; US-Daten als Regression erhalten; keine parallele wirtschaftliche Initialisierung |
| `GeoTransform.h` | Gemeinsame Python/C++-Konstanten, Ursprung USA | Einen konfigurierten Projektionsvertrag an Import und Runtime geben; keine separate Transformation im Widget |
| `GeoMapData.cpp` | `country.usa/can`, `region.test.*`, vier `city.us.*`, US-Koordinatenfenster | Validierung aus dem Szenariokatalog statt dieser US-Konstanten; aktuelle strenge Referenz-/Geometrieprüfungen erhalten |
| `CoIBuildMapCommandlet.cpp` | `BuildUSMap`, benannte Seenmaterialien und `/Game/Maps/GreatLakes1913` | Benannter DE/NL-Katalog/Level, vorhandene StaticMeshes/Geokatalog-Cooking nutzen; keine laufzeitliche Grenztriangulierung |
| `GeoCatalog.h` | JSON und Provenienz als gekochtes DataAsset | Neues Manifest samt Datenversion/Attribution übernehmen |
| `MapCameraPawn.cpp` | Kameragrenzen kommen aus Katalog | Grenzen/Übersicht neu ableiten; Eingabe-/Zoomlogik weiterverwenden |
| `SimulationSubsystem.cpp` | Liest Katalog aus Level und `Content/Simulation/scenario.coisim` | MAP03: genau einen passenden frischen Szenariodatensatz auswählen |
| `Config/DefaultEngine.ini`, `DefaultGame.ini` | GreatLakes-Start-/Cookreferenzen | MAP02/03 bewusst umstellen und Offline-Cook prüfen |
| Wirtschaft, Hubmodell, UI01, Kostenrechnung, Save v6 | Vorhandene IDs/Assets/Marktlogik | Über neue Regions-/Hub-IDs anbinden; keine zweite Wirtschaft oder gratis Kapazitäten |

MAP01 verändert keine Runtime-/Wirtschaftsdaten. Der bisherige spielbare
US-Teststand bleibt bis zum beauftragten Import startbar; die **neue Zielvorgabe**
ist jetzt DE/NL. Dies ist kein vorgezogener MAP03-Szenariowechsel.
Keine neuen persistierten Simulationszustände, deshalb kein Save-Versionssprung,
keine neue Konsolenmutation und keine neuen UI-Messpunkte in diesem Planpaket.
Bei inkompatibler späterer Umstellung Version erhöhen, neue Partie verlangen;
keine Altspielstandmigration.

## Projektion und physische Geografie

Quelldaten bleiben **WGS84 / EPSG:4326**, GeoJSON-Reihenfolge `[Längengrad,
Breitengrad]`. Darstellung geplant mit der bereits verwendeten sphärischen
Lambert Conformal Conic 2SP: Ursprung 52°N/13°E, Standardparallelen 48°/55°,
Radius 6371.0088 km, 7 UE-Einheiten/km; X Nord, Y Ost, Z oben. Eigene Parameter,
kein vorgetäuschter EPSG-Code. Ausschnitt: 2.5°–23.5°E, 47°–56.5°N.
200 m ist ein erster Vereinfachungswert, **kein Genauigkeitsversprechen**.
Kleine Exklaven und die Berliner Aussparung brauchen eine feinere Toleranz.

MAP02 prüft C++ gegen unabhängig berechnete PROJ-Referenzpunkte sowie zwei
ellipsoidische Referenzdistanzen (Rotterdam–Köln und Berlin–Königsberg).
GeoNames liefert heutige WGS84-Ortsanker unter CC BY 4.0. Konkrete geonameids,
Snapshot/Archivhash und historische Einbettung werden vor Punktimport festgelegt.
Charlottenburg darf nicht auf den allgemeinen Berlin-Punkt aufgelöst werden.

Die politische Region ist nicht identisch mit ihrer sichtbaren Landfläche.
Politische Abfrage und Land-/Wasserabfrage bleiben getrennt; Grenzflüsse und
Gewässer innerhalb einer Region dürfen deren Landeszugehörigkeit nicht löschen.
Insbesondere keine moderne NL-Landmaske mit Flevoland, Afsluitdijk, IJsselmeer
und späteren Poldern als 1913 ausgeben. Die [Rijkswaterstaat-Chronik](https://www.rijkswaterstaat.nl/water/waterbeheer/bescherming-tegen-het-water/100-jaar-zuiderzeewet)
belegt die späteren Eingriffe. Natürliche Seen, Küste und Flusslinien müssen mit
den historischen Karten abgeglichen werden; Natural Earth ist nur moderne
Kontext-/Hilfsgeometrie, kein historischer Grenzlieferant.

Für Welt-Erweiterung später: Projektionsprofile je Ausschnitt, LOD/Kacheln,
Datumsgrenzen-Normalisierung und Polygonteilung sowie Behandlung der Polnähe.
Die regionale LCC wird nicht einfach auf einen Globus ausgedehnt. Unsichtbare
Regionen werden weiterhin durch denselben Simulationskern berechnet.

## Wasserumfang und erste Route

`waters.json` enthält **37 getrennte Gewässereinträge** und vier ausdrücklich
ausgeschlossene moderne Verbindungen. [Gewässerliste](MAP01_Waters.md) trennt
sichtbare Flächen/Linien, belegte historische Korridore, bloße Existenznachweise
und noch nicht freigegebene Abschnitte. Sechs optionale Kanäle bleiben ohne
Abschnittsnachweis auch aus dem vorgesehenen sichtbaren Import ausgeschlossen.
Kein Eintrag aktiviert bereits eine Transportkante.

Erste MAP04-Route: **Rotterdam–Köln**, über das historische Delta/Waal und
Lobith/Emmerich. Zeitgenössisch belegt sind Verkehr und Hafenfunktionen im
[Rheinartikel](https://en.wikisource.org/wiki/1911_Encyclop%C3%A6dia_Britannica/Rhine).
Deltaarm-Verlauf und Terminals werden erst im Netz topologisch festgelegt.
Ein Hafen/Flussknoten besitzt weder zusätzliche Bevölkerung noch einen zweiten
Markt. Ein Hub ohne Anschluss benötigt einen expliziten kostenpflichtigen
Zubringer; keine Frachtteleportation. Schiffstyp, Schleusenmaße, Kapazitäten,
Kosten, Umschlag, Warte-/Leerfahrt und Zugang sind spätere konfigurierbare
Spielwerte, keine aus alten Handelsstatistiken abgeleiteten Fantasiewerte.

## Reproduktion, Prüfschritte und offene Datengates

1. `python Tools/verify_map01.py --self-test --write-report` aus dem Repository
   ausführen. Nur Standardbibliothek, offline. Verifiziert Umfang, IDs,
   1:1-Hierarchie, Quellenreferenzen, Belegstatus und getrennte Wasserfreigaben.
2. Ergebnis und Dateidigests stehen in `Docs/Verification/MAP01.json`.
   Wiederholung ohne Änderungen muss identische Digests liefern.
   `python Tools/render_map01_docs.py --check` prüft zusätzlich, dass die drei
   Lesetabellen zu den Daten passen. Nach bewusster Datenänderung ohne `--check`
   neu erzeugen.
3. Quellen einzeln über das Quellenregister prüfen. Optional:
   `python Tools/collect_map01_evidence.py` erzeugt neue **Prüfentwürfe** unter
   `Saved/MAP01Research`, überschreibt den festgelegten Plan nicht. Moderne
   Websites werden nur manuell paraphrasiert. Kein automatischer Retry bei
   Providersperren; geänderte Quellen zuerst bewusst prüfen.
4. MAP02 beginnt mit dem Download-/Rohquellen-Lock: Originaldateien, Datum,
   Lizenznachweise, vollständiger SHA256. Die historischen Raster sind in MAP01
   nicht mitgeliefert; der Originaldateidownload wurde bei der Recherche mit
   HTTP 429 begrenzt. Referenz-/Dateiseiten sind recherchiert, aber ein lokaler
   Rasterhash oder fertiger Polygonbestand wird nicht vorgetäuscht.
5. Historische Karten georeferenzieren (Raster-CRS zunächst unbekannt, nicht
   als WGS84 deklarieren), Kontrollpunkte/Transformationsrestfehler protokollieren,
   gemeinsame Grenzbögen einmal digitalisieren, nach WGS84 exportieren.
   Belegte Grenzen, abgeleitete Vereinfachungen und unsichere Linien pro Feature
   markieren. Scans/Controlpoints/Regeln speichern; der Folgeimport muss offline
   wiederholbar sein. Keine erneute freihändige Grenzerstellung bei jedem Import.
6. Vor MAP02-Abnahme: Deutschland-/NL-Stichtag und Änderungen 1911–1913,
   Alt-Berlin, kleine/exklavierte Territorien, Inseln und NL-Küste abgleichen.
   Geometrie auf Lücken, Überlappungen, ungültige Polygone und Hubcontainment
   prüfen. Die heutige MAP01-Prüfung beweist **logische** Vollständigkeit,
   noch keine flächige Polygonvollständigkeit.
7. Vor MAP04-Abnahme: Deltaarme/Häfen und historische Fahrbarkeit des ersten
   Korridors je Kante, Maße und Zugangsregeln bestätigen. Unbelegte optionale
   Gewässer bleiben gesperrt; keine Transitfreigabe aus bloßer Linienkreuzung.

Diese Lücken sind konkret sichtbare Arbeitsgates, keine kostenpflichtigen
Beschaffungsaufträge. Es gibt keine offene Produktfrage, die MAP01 blockiert.
Die vorgeschlagenen Hubs sind als überprüfbare Startentscheidung festgehalten;
abweichende spätere Auswahl erfolgt bewusst über die Daten.

## Abnahme dieses Pakets

| Kriterium | Ergebnis |
|---|---|
| Hierarchie ohne bewusst überlappende Ebenen | 2 Länder, 39+11 exklusive logische Einheiten; geometrische Prüfung folgt MAP02 |
| Genau ein begründeter Hub je Region | 50:50, Quellen/Alternativen und unsichere Rangfälle dokumentiert |
| Historie, Vereinfachung und Balancing getrennt | Belegstatus, Stichtag, Geometrielücken und spätere Netzparameter getrennt |
| Nutzungsrechte und Konflikte sichtbar | Freigegebener Digitisierungsweg; NC/ungeklärte Quellen ausgeschlossen |
| Überprüfbare Lieferung | Vier JSON-Dateien, drei Lesetabellen, Offline-Validator samt acht Negativfällen |

Tatsächliche Prüfergebnisse stehen im Verifikationsbericht. Keine neue Karte,
Spielökonomie oder UI wurde für MAP01 gebaut; hierfür werden keine neuen
Build-/Sichtprüfungen behauptet. Zusätzlich bestanden am 20.11.2026 der bestehende
Geografietest und beide Hubtests mit UE 5.8.4; Auszug in
`Docs/Verification/MAP01_UnrealTests.txt`. Der Testname `MigrationAndTransit`
bezieht sich auf historische Szenariofixtures, nicht auf Save-Migration.
Nächstes Paket: **MAP02 – Import der Land-,
Gewässer- und Verwaltungsgeometrie** mit den genannten Rohdaten-/Detailgates.
