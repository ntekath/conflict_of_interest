# MAP03 – frisches Wirtschaftsszenario Deutschland/Niederlande

Stand 20.11.2026. Die neue Standardkarte startet jetzt mit **`scenario.de_nl.1913`**, pausiert am 01.01.1913. MAP02-Geometrie, 50 historische Regionen und ihre 50 Hubs bleiben erhalten. Es gibt genau 50 B2B-Regionsmärkte, 150 Haushaltsgruppen und zwei Länderaggregate. Die bestehenden Produktions-, Handels-, Einzelhandels-, Kosten-, Bau-, Liefer- und Speicherdienste werden weiterverwendet.

## Start und Firmenwahl

`ConflictOfInterest.uproject` → `/Game/Maps/DE_NL1913` → Play → Start. Stadt öffnet zunächst Rotterdam. Der Stadtkopf zeigt Zuid-Holland/Niederlande, danach die tatsächlich dort zugeordneten Haushalte und Assets. Märkte, Lager, Transportplanung, Lieferregeln, Investitionen, Kostenaufschläge und Entwicklerkonsole verwenden die neuen Orts-/Markt-IDs.

Die Auswahl erfolgt wie in AP06a über Szenariokonfiguration, nicht über ein neu erfundenes Auswahlmenü: `player_company_id` in `GeoData/Scenarios/de_nl1913.json` ändern und den Generator ausführen, dann eine **neue Partie** starten.

| CompanyId | Startort und vorhandener Besitz | Anfangsgeld |
| --- | --- | ---: |
| `company.player` | Handelshaus Maas & Rhein, Rotterdam: Lager, Gemischtwarenladen, ein eigener Lkw, 600 kg Nahrung und 50 kg Luxuswaren an zwei getrennten Speicherorten | 25.000 USD |
| `company.food` | Norddeutsche Nahrungswerke, Hannover: drei Farmen/Betriebslager in Hannover, Rostock und Königsberg, keine eigenen Lkw | 100.000 USD |
| `company.carrier` | Rheinland Fracht, Köln: vier Lkw in Köln, Hannover, Leeuwarden und Rotterdam | 100.000 USD |

Alle drei Firmen existieren unabhängig von der Auswahl. Die Auswahl ändert nur die Steuerung. Produzenten zahlen weiter Löhne und produzieren; autonome Planung wird ausschließlich für die übernommene Firma deaktiviert. Ungültige/nicht spielbare Auswahl wird abgewiesen. Es werden keine zusätzlichen Konten, Fahrzeuge oder Startguthaben erzeugt.

## Konfigurationsdaten und Eröffnungsbilanz

`GeoData/Scenarios/de_nl1913.json` enthält die konkreten Verteilungen und Beträge. `Tools/create_europe_scenario.py` übernimmt ausschließlich die abgeschlossenen AP09.4-Regel-/Typdefinitionen aus dem unveränderten US-Regressionsdatensatz; sämtliche geografischen Entitäten und Laufzeitzeilen werden frisch aufgebaut. Ergebnis: `Content/Simulation/scenario.de_nl1913.coisim`. Keine automatische Aktualisierung während des Spiels.

- **60 Unternehmen:** zehn überregionale/Abrechnungsakteure plus genau ein lokaler Ladenbetreiber je Hub. Drei lokale Firmen besitzen zusätzlich Farmen. Dienstleisterkonten sind als Abrechnungskonten gekennzeichnet und nicht spielbar.
- Je Hub 600/300/100 Personen der niedrigen/mittleren/hohen Einkommensgruppe, davon jeweils die Hälfte Erwerbspersonen: **50.000 Personen, 25.000 Erwerbspersonen** insgesamt. Deutschland 39.000, Niederlande 11.000 Personen. Keine historischen Bevölkerungszahlen.
- Haushaltsanfangsguthaben je Hub: 500/1.000/2.000 USD. Lokale Firmen je 15.000 USD. Überregionale Beträge stehen einzeln in `capital_cents`. Gesamte Eröffnungsgeldmenge **1.450.000 USD**. Der externe Empfänger startet bei null. Keine periodischen Transfers oder automatische Auffüllung.
- **13 Produktionsstätten:** sieben Farmen, zwei Forstbetriebe, zwei Erzförderer, ein Werkzeugbetrieb und ein Luxuswarenbetrieb. Nahrung in Hannover, Rostock, Königsberg, Leeuwarden, Groningen, Tilburg und München; Holz in Mannheim/München; Erz in Köln/Breslau; Werkzeuge in Köln; Luxuswaren in Leipzig.
- Ressourcen sind **fiktive, bewusst asymmetrische Spielvorgaben**. Nur elf Regionen haben konfigurierte Quellen: sieben Agrarregionen mit Eignung 600–800‰, drei Waldregionen mit insgesamt 2.100.000 kg und zwei Erzregionen mit insgesamt 1.800.000 kg; Bayern zählt zu Agrar- und Waldregionen. Keine Quellen in den übrigen Regionen zur Vollversorgung hinzufügen. Baden ist der Mannheimer Hub für sein gesamtes Umland, kein Behaupten einer Mine direkt im Stadtzentrum.
- Quellen besitzen jeweils zwei maximale Stundenchargen; initiale Rohstoffbetriebe belegen je eine. Eignung, endlicher Abbau, Beschäftigung und Vorprodukte begrenzen weiter die Produktion. Unbelegte Quellen sind keine kostenlosen Warenbestände.
- Jeder Produzent beginnt mit 1.000 kg seines Outputs im Betriebslager. Werkzeuglager: 1.000 kg Erz und 500 kg Holz; Luxuslager: je 500 kg Holz/Werkzeuge. Je lokaler Laden 300 kg Nahrung; Spieler-/Rheinhandel-Läden je weitere 300 kg Nahrung und 50 kg Luxus. Handelslager des Spielers 300 kg Nahrung, Rheinhandel 1.000 kg. Diese deklarierte Eröffnungsausstattung ist keine laufende Produktion oder historische Statistik.
- **118 Lager-/Ladenassets:** 50 Lebensmittelgeschäfte, zwei konkurrierende Gemischtwarenläden, 50 endliche Dienstleisterlager, 13 Betriebslager, zwei Handelslager und ein ausdrücklich angebotenes leeres Lager in Rotterdam. Kaufpreis des angebotenen Lagers 1.500 USD, Angebot 168 Stunden; Zahlung und Besitzwechsel über den vorhandenen Investitionsdienst.
- **56 Lkw:** je ein eigener Lkw pro lokalem Betreiber plus vier beim Transportanbieter, einer beim Spieler und einer beim Rheinhandel. Auch lokale Nachschubfahrten binden reale Fahrzeuge und zahlen Kosten. Kein Fahrzeug entsteht aus der Sichtbarkeit der Karte.
- Eröffnungsbuchwerte: Lager/Läden je 500 USD, Betriebe je 3.000 USD, Lkw je 2.000 USD. Werte sind Bilanzannahmen, kein Bargeld und kein garantierter Verkaufspreis.

USD bleibt die vorhandene gemeinsame **Test-Recheneinheit**; dies behauptet keine historische deutsche/niederländische Währung. Währungsumrechnung, Banken, Zölle und politische Regeln werden hier nicht vorweggenommen. Alle wirtschaftlichen Zahlen sind fiktiv; die historischen Belege für Hubwahl und Geometrie bleiben in MAP01/MAP02.

## Unveränderte Regeln und Intervalle

Die vollständigen Werte stehen im generierten Szenario. Weiterhin Ganzzahl-Cent, kg und Stunden; ein Schritt ist eine Stunde. Seed 1913. Löhne/Kosten/Konsum täglich; Unternehmensplanung alle sechs Stunden; Preisbasis alle 24 Stunden. Aufträge laufen nach zwölf Stunden ab, Abholfrist 24 Stunden. Käufe ändern Eigentum am Quelllager; Lieferung ist ein eigener Auftrag. Lokale Transfers haben weiterhin 1 km Rechenweg und mindestens die konfigurierte Buchungs-/Fahrzeit.

Lkw: 2.000 kg, 20 km/h, 0,05 USD/km, 1 USD/Tag Fixkosten; abstrakte Entfernung WGS84-Luftlinie ×1,4, Fremdfracht mit 20% Aufschlag auf Fahrbetriebskosten. Keine Routenberechnung aus Pixeln, Küstenfarben oder Regionsmeshes. Inselverbindungen und Wasserbarrieren sind im bestehenden abstrakten Landmodell noch nicht historisch geroutet; MAP04 ersetzt/erweitert dies durch explizite Netzfreigaben. Keine Schiffbarkeit aus Flusslinien.

Maximal 2.048 offene Orders und 256 Lieferregeln für den größeren Ausschnitt. Die Kostenbasis deckt weiterhin Eigentümer × Lager × Ware ab: aktuell 35.400 Kalkulationszeilen. Ausschließlich diese Tabelle erhält eine begrenzte Obergrenze von 100.000 statt 10.000 im Validator und Save-Leser; alle anderen Arraygrenzen bleiben 10.000, der Save-Größen-/Tiefenschutz bleibt bestehen. Kein neues allgemeines Streaming-/Parallelframework.

## Länder-/Regionsstatistik und Modell-BIP

Bevölkerung, Beschäftigung und Konsummengen werden aus Haushaltsgruppen einmal je Region berechnet; Länder summieren ausschließlich ihre Regionen. Keine zweite nationale Nachfrage und kein nationales Haushaltskonto.

Da die bisherige Engine kein geografisches BIP hatte, ergänzt MAP03 eine **kumulative Modellkennzahl seit Partiebeginn**, keine amtliche BIP-Schätzung:

`Modell-BIP = bezahlter Endverbrauch + Warenbuchwertänderung – Netto-Buchwertzufluss aus anderen Regionen + fertiggestellte Neubauten/Ausbauten`.

Endverbrauch zählt ausschließlich tatsächlich bezahlte Ladenkäufe. Eröffnungsbestände werden einmal abgezogen. Frachtbestände bleiben bis Ankunft der Herkunftsregion zugeordnet; bei Ankunft wird derselbe Buchwert an Quelle und Ziel symmetrisch als Transfer statistisch erfasst. Bestehende Assetkäufe erzeugen keine Neuinvestition; Neubauten erst nach Fertigstellung. Buchwertveränderungen enthalten die vorhandene Kostenbewertung und Verderbsverluste. Mangels vollständiger Volkswirtschaft, Marktpreisbewertung und Außenwirtschaft ist dies eine **Spiel-/Diagnose-Näherung nach Buchwerten**, kein reales oder international vergleichbares BIP. Negative Werte sind zulässig. Sie erzeugt weder Geld noch Kaufkraft und erhöht keinen Bedarf.

Regionen zeigen die Kennzahl in der Kartenauswahl, Länder in Auswahl/Diagnose; Diagnose nennt die Formel. Ladenumsatzstatistik muss exakt den gebuchten `retail_sale`-Zahlungen entsprechen, regionale Nettotransfers summieren sich zu null. Die Länderkennzahl ist exakt die Summe ihrer Regionen.

## Szenariowahl, Save/Load und Reproduktion

Der Geokatalog nennt sein aktives `simulation_file`. Die Laufzeit lädt genau diese Datei bei Neustart, Entwickler-Neustart und Save-Laden; keine Kombination aus US- und Europa-Daten, kein stiller Fallback bei fehlender Europa-Datei. Dateinamen sind gegen Verzeichnispfade geprüft. Das bestehende technische `geography_only`-Flag kennzeichnet weiterhin das MAP02-Format mit getrennten politischen/physikalischen Flächen; nur ein Katalog **ohne** zugeordnete Simulationsdatei bleibt im Vorschaupanel.

Save-/Simulations-/Definitionsversion **7** wegen der neuen geografischen Statistikzähler. Version 6 und ältere Spielstände werden mit „Neue Partie“ abgewiesen, ohne Konverter. Save/Load innerhalb Version 7 erhält auch Modellstatistik, Firmensteuerung und laufende Lieferungen. Beschädigte oder fremde Szenarien ändern den laufenden Zustand nicht. Der US-Datensatz bleibt als separater Regressionsfall über GreatLakes1913 erreichbar; er verwendet ebenfalls ausschließlich aktuelle Version-7-Saves.

```powershell
& Saved/MAP02Python/Scripts/python.exe Tools/import_europe.py
& Saved/MAP02Python/Scripts/python.exe Tools/create_europe_scenario.py
./Tools/Build.ps1 -EngineRoot E:\UE_5.8 -Target Editor
& E:/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe "$PWD/ConflictOfInterest.uproject" -run=CoIBuildMap -Europe -unattended -nop4 -nullrhi
./Tools/Test.ps1 -EngineRoot E:\UE_5.8
```

Geometrie und Hubs sind gegenüber MAP02 unverändert; nur Szenario-ID und Dateizuordnung im Katalog ändern sich. Die Szenariogenerierung ist offline und byteidentisch wiederholbar. Im paketierten Spiel werden die `.coisim`-Dateien über das bestehende UFS-Staging von `Content/Simulation` mitgeliefert. Keine bezahlte Quelle oder neuer Dienst erforderlich.

## Abnahme und verbleibende Grenzen

Nachweise unter `Docs/Verification/MAP03`: vollständige Regression, frische Firmenwahl mit identischem Weltzustand, 14 Tage ohne Eingriffe mit täglicher Geld-/Mengenprüfung, Kauf → bezahlte Leerfahrt → Transport → Verkauf und identische Save-Fortsetzung über Lohnlauf, Konsum und Ankunft. UI-Prüfung 1280×720 und 1920×1080. Reale Anfangsbilanzen und Tagesmesswerte sind eingecheckt.

Ergebnis: **76/76 Tests** bestanden; nach dem finalen Datenstand nochmals **3/3 Europa-Szenario- und 3/3 Geografietests**. Editor, Game Development und Shipping erfolgreich. Windows-Cook: null Fehler, eine bekannte Warnung zum fehlenden NTFS-Journal des Engine-Laufwerks; 666 Pakete gekocht. Ein vollständiger gestagter Installer wurde hier nicht ausgeführt. Tag 14: 50.000 Personen, 531 Beschäftigte, 167.600 kg produzierte Nahrung, 20.537 kg tatsächlicher Nahrungskonsum und unverändert 145.000.000 Cent; Produktionsinputs/Verderb stehen in den Laufzeitbilanzen. Die Welt bleibt deutlich unterversorgt, insbesondere außerhalb funktionierender Nachschubketten. Zeitmesswerte der CSV sind Einzelmessungen unter paralleler Rechnerlast, kein Leistungsbenchmark.

Die Welt ist absichtlich nicht vollversorgt. Entfernte Standorte können trotz vorhandener globaler Produktion unterversorgt bleiben, Lager füllen sich und Unternehmen zahlen endliche Kosten. Kein automatisches Rettungskapital; kein Anspruch auf langzeitstabiles Balancing. Schiffbares Netz/Hafenanschlüsse sind **MAP04**, Schiffsplanung MAP05 und abschließende Karten-/LOD-/Bedienverfeinerung MAP06. Die vorhandenen Bildschirme werden integriert, kein neues UI-Gesamtdesign.
