# MAP04 – Schiffbares Netz und Hafenanschlüsse

Stand 21.11.2026, Grundlage: vollständiges Arbeitsdokument (9), MAP01–MAP03 und vorhandener Transportdienst. Kostenlos zugängliche historische Texte und amtliche Faktenquellen; keine bezahlten Daten oder Dienste.

## Umfang und Bedienung

Das neue, vom Kartenmesh unabhängige Netz `network.de_nl.rhine.1913.v1` enthält 67 Knoten, 34 gerichtete Kanten und zwei Hafenanschlüsse. 32 Kanten sind freigegeben. Genau die bestehenden 50 wirtschaftlichen Hubs bleiben erhalten; Fluss-/Hafenknoten besitzen weder Bevölkerung noch eigene Regionalmärkte, Lager, Beschäftigte oder Bestände.

Der konfigurierte Pilotkorridor verbindet **Köln und Rotterdam**: Rhein → Waal → Merwede → Noord → Nieuwe Maas. Die umgekehrte Richtung besteht aus eigenen Kanten mit geringerer Geschwindigkeit. Andere sichtbare Flüsse erhalten dadurch keine Freigabe. Merwede-Kanal und Nordseekante sind ausdrücklich gesperrte Datenkandidaten, keine verwendbaren Abkürzungen.

Die bestehende **Diagnose** beginnt mit Netz-ID, Datenstand, Anzahl Knoten/Kanten/Häfen, fehlenden Hafenanschlüssen, gesperrten/unbelegten Abschnitten und einer 1.000-kg-Proberoute. 48 Hubs haben keinen freigegebenen Wasseranschluss. In Development öffnet F10 die vorhandene lokale Entwicklerkonsole:

```text
network status
network route city.de.1913.rheinprovinz city.nl.1913.zuid_holland 1000 1913
network route city.nl.1913.zuid_holland city.de.1913.rheinprovinz 1000 1913
```

Die Vorschau ist rein lesend. Der normale Straßen-Transportdienst bleibt erhalten. Spielbare Schiffsangebote, Fahrzeuge, zeitbezogene Reservierungen, Zahlung/Ankunft und Kartendarstellung der Transportkette folgen **MAP05**. Dieses Paket stellt dafür die validierten Netzdefinitionen und die Planungsfunktion bereit.

## Regeln und Einheiten

Alle Spielparameter liegen in `GeoData/Networks/de_nl_1913/network.source.json`, Abschnitt `Rules`. Sie sind ausdrücklich Balancingwerte, keine recherchierten historischen Frachttarife oder Hafenleistungen.

| Parameter | Spielwert |
|---|---:|
| Abschnittsfreigabe einschließlich beider Grenzjahre | 1913–1913 |
| Wasser-Abschnittskapazität je vorgeschlagener Sendung | 20.000 kg |
| Landzubringer-/Umschlagkapazität je Sendung | 10.000 kg |
| Geschwindigkeit flussabwärts / flussaufwärts | 10.000 / 6.000 m je Stunde |
| Landzubringer | 20.000 m je Stunde |
| Wasserpreis / Zubringerpreis | 2 / 35 USD-Cent je km |
| Umschlag je Lade- oder Entladevorgang | 120 Minuten + 50 USD-Cent + 1 USD-Cent je kg |

Für jede Fahrkante wird die Länge als `ceil(Summe der geodätischen Distanzen auf der Kugel R=6.371.008,8 m zwischen WGS84-Stützpunkten))` berechnet. Die Laufzeit validiert diese Länge gegen die gespeicherte Linie; UE-Projektion, Zoom und Bildschirmabstände gehen nicht ein. Die Linien sind **eigene generalisierte Spielgeometrie**, keine vermessene historische Fahrrinne. Kurven werden vereinfacht; 270,917 km Wasserstrecke sind die Länge dieses Modells und dürfen nicht als reale historische Flusskilometer ausgegeben werden. Terminalpositionen und Straßenstützpunkte sind ebenfalls schematisch. Tiefgang, Pegel, Schleusenbetrieb und jahreszeitliche Sperren werden nicht simuliert.

- Zeit pro Kante: `FixedMinutes + ceil(LengthMeters * 60 / SpeedMetersHour)`; Umschlag hat null Fahrmeter und feste Zeit, ohne Division durch null.
- Preis pro Kante: `FixedCents + ceil(LengthMeters * CentsPerKm / 1000) + Kg * CentsPerKg`.
- Gesamtdauer/-preis sind die Summen aller Teilstrecken einschließlich beider Zubringer und zweier Umschläge. Kapazität ist das Minimum der Kantenkapazitäten. Das ist eine statische Eignungsprüfung, noch keine Belegungsreservierung oder Zahlungsbuchung.
- Deterministische Wegsuche nur über explizite Knoten-/Kanten-IDs: kürzeste Dauer, dann geringster Preis, dann lexikografische Kanten-ID-Folge. Kein Mesh-Routing. Kreuzende oder deckungsgleiche Punkte mit verschiedenen IDs verbinden sich nicht automatisch.
- Jede Route muss zuerst den konfigurierten Quellzubringer und die Ladekante, dann ausschließlich freigegebene Wasserarten und zuletzt Entladung/Zielzubringer enthalten. Binnenhub ohne Hafen, fehlender Umschlag, gesperrte Kante, unzulässige Verkehrsart, Datum, Flagge oder zu große Menge verhindern die Route.
- Grenzüberschreitender Zugang liegt ausdrücklich in `access.rhine.de_nl.1913`: nur die simulierten Länder und Flaggen Deutschland/Niederlande. Historischer Referenzbeginn 1869; die konkret recherchierte Abschnittsfreigabe ist trotzdem auf 1913 begrenzt. 1914 wird abgelehnt, ohne eine historische Schließung des Rheins zu behaupten. Keine neuen Zölle oder politischen Systeme. Kein Transit durch unmodellierte Nachbarländer.

Für 1.000 kg Köln → Rotterdam liefert das Modell **273.296 m insgesamt**, davon 270.917 m Wasser, **1.879 Minuten**, **2.732 USD-Cent**, Engpass **10.000 kg**, 16 explizite Kanten. In Gegenrichtung: 2.965 Minuten. Dies sind Kosten-/Zeitvorschauen; es wird nichts abgebucht oder erzeugt.

## Quellen, Rechte und Grenzen

| Quelle | Verwendete Aussage / Rechte |
|---|---|
| [Encyclopædia Britannica 1911: Rhine](https://en.wikisource.org/wiki/1911_Encyclop%C3%A6dia_Britannica/Rhine) | Zeitgenössische Rheinschifffahrt, Grenzübergang bei Emmerich und Häfen Köln/Rotterdam; historische Texte gemeinfrei. Keine pauschale Nebenarmfreigabe und keine Übernahme damaliger Ausbauziele als garantierter Tiefgang. |
| [EB 1911: Holland](https://en.wikisource.org/wiki/1911_Encyclop%C3%A6dia_Britannica/Holland) | Historische Verzweigungsfolge Waal/Merwede/Noord/Nieuwe Maas und Kanalbestand; gemeinfrei. Kein Beleg für beliebige moderne Kanäle oder ganzjährig beliebige Schiffstypen. |
| [EB 1911: Cologne](https://en.wikisource.org/wiki/1911_Encyclop%C3%A6dia_Britannica/Cologne) | Hafen/Kais vor 1913, gemeinfrei. Spielterminal und Straßenverlauf sind eine ausdrücklich eigene Vereinfachung. |
| [EB 1911: Rotterdam](https://en.wikisource.org/wiki/1911_Encyclop%C3%A6dia_Britannica/Rotterdam) | Hafenbecken, Lade-/Entladeeinrichtungen und Anschlüsse vor 1913; gemeinfrei. Keine genaue historische Terminalvermessung. |
| [Zentralkommission für die Rheinschifffahrt: Geschichte](https://www.ccr-zkr.org/en/history) | Mannheimer Akte und historische DE/NL-Zusammenarbeit; nur Faktenreferenz, keine Übernahme geschützter Texte oder Grafiken. Die konkrete Spiel-Zugangsregel ist eine Vereinfachung. |
| [Niederländische Vertragsdatenbank: Mannheimer Akte](https://treatydatabase.overheid.nl/en/Treaty/Details/001517.html) | Abschluss 17.10.1868, Inkrafttreten 01.07.1869; nur Faktenreferenz. Heutige konsolidierte Vertragstexte werden nicht als Wortlaut von 1913 ausgegeben. |
| [GeoNames](https://download.geonames.org/export/dump/) | Bereits in MAP02 festgeschriebene Hubkoordinaten, CC BY 4.0. Bestehende Attribution bleibt erhalten. Eigene Routenstützpunkte ergänzen diese Koordinaten. |

Nachweise und Hinweise sind zusätzlich je Quelle/Kante in den Konfigurationsdaten enthalten. Die generalisierten Strecken sind eigene Daten; keine Kopie des historischen Karten-SVG oder eines geschützten Straßen-/Gewässerdatensatzes. Die Quellen tragen den historischen Korridor und die Hafenexistenz; die konkrete Unterteilung, Punkte, Leistungswerte und Kapazitäten sind dokumentierte Spielmodellentscheidungen.

## Dateien und Reproduktion

- `Tools/build_transport_network.py`: Offline-Compiler mit Python-Standardbibliothek.
- `GeoData/Generated/de_nl_1913/transport_network.json`: deterministisch erzeugter Prüfstand.
- `GeoData/Generated/de_nl_1913/transport_validation.json`: Hash, Umfang und gesperrte Kandidaten.
- `Tools/create_europe_scenario.py`: bettet denselben Graph in die vorhandene `.coisim`-Datei ein. Das bestehende UFS-Staging von `Content/Simulation` liefert ihn ohne Netzverbindung aus; kein Karten-/Mesh-Neubau notwendig.
- `Simulation/TransportNetwork.h/.cpp`: Definition, Validierung, rein lesende Routenplanung und Diagnose.

```powershell
python Tools/build_transport_network.py
python Tools/create_europe_scenario.py
./Tools/Build.ps1 -EngineRoot E:/UE_5.8 -Target Editor
./Tools/Test.ps1 -EngineRoot E:/UE_5.8 -Filter CoI.MAP04
./Tools/Test.ps1 -EngineRoot E:/UE_5.8
```

Graph-SHA256: `81f3af750ee618124be1e210f51ae592014e5fc74f60b32e26d420ddcb312d56`. Wiederholte Kompilierung ergibt identische Daten. Ein JSON-Vergleich mit dem Paketanfang bestätigt: Nach Entfernen von `Definitions.Network` ist das Europa-Szenario unverändert. Keine zusätzlichen Konten, Geldbeträge, Waren, Wirtschaftsassets oder Bevölkerung.

**Save-Version 10**, keine Migration. Alte Versionen werden abgelehnt; neue Partie erforderlich. Dateien werden nicht ungefragt gelöscht. Save/Load und identische Fortsetzung werden innerhalb Version 10 geprüft.

## Abnahme

Vier automatisierte MAP04-Tests prüfen positive Hin-/Rückroute und Kosten-/Zeit-/Kapazitätssummen, Eingangssortierung, Inlandhub, Datum, Flagge, Mengen, Sperren, fehlenden Umschlag, unzulässige Modi, unbelegte Seefreigabe, Geometriefehler, unmodelliertes Transitland, doppelte IDs, räumlich deckungsgleiche unverbundene Knoten, reine Vorschau und Save/Load mit identischer eintägiger Fortsetzung. Nachweise und vollständige Regression stehen unter `Docs/Verification/MAP04`.

Ergebnis: **100/100 vollständige Regressionstests** bestanden, darunter die vier MAP04-Tests, Europa-Wirtschaft, mehrwöchige Bank-/Kreditläufe, Handel/Transport, UI06–UI08 und Save-Fortsetzung. Editor, Game Development und Game Shipping erfolgreich. `git diff --check` ohne Fehler. Die Prüfungen liefen headless; dieses Paket ändert keine interaktiven Layouts oder Kartenmeshes und führte keinen neuen Cook/Installer-Lauf aus.
