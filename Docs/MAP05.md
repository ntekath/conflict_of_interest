# MAP05 – Wassertransport im bestehenden Planungsmenü

Stand 21.11.2026. Umsetzung nach MAP04 und dem vollständigen aktuellen Arbeitsdokument (9). Nur vorhandene kostenlose Werkzeuge, Projekt-/Engineinformationen und die bereits in [MAP04](MAP04.md) dokumentierten freien historischen Quellen. Keine neuen historischen Leistungs- oder Preisbehauptungen.

## Spielen

Eine **neue Partie** starten (Save-Version **11**, keine Migration). Im vorhandenen Menü **Transporte → Lieferung planen** eigene Ware am Quelllager wählen. Das Fahrzeugfeld bietet neben Lkw **„Rheinland Fracht · Rheinschiff + Hafenzubringer“** an. Ziellager, kg und Abfahrt wählen, Vorschau aktualisieren und ausdrücklich bestätigen. Bei Pause wird erst mit Start ausgeführt. Wiederholtes Öffnen, Vorschauen oder Anzeigen erzeugt keine Buchung.

Zum schnellen Test in der neuen Standardpartie: Nahrung aus „Eigenes Lager · Rotterdam“, Rheinschiff, „Mietlager · Köln“, 100 kg, Abfahrt in einer Spielstunde. Die Vorschau zeigt **274 gerundete Modell-km, 50 Stunden Kettendauer, Abfahrt 01.01.1913 01:00, Ankunft 03.01.1913 03:00 und 11,19 USD**. Darin sind 240 Minuten Lade-/Entladezeit, Landzubringer, 9,32 USD Betrieb und 1,87 USD Anbieteraufschlag enthalten. Je nach verfügbarer Ware/Kapazität kann die Buchung später abgewiesen werden; die Ausführung prüft erneut.

Laufende Aufträge zeigen „Wasserkette“ und den aktuellen Teilabschnitt: Hafenzubringer, Beladung, Schiffsabschnitt, Entladung, Zielzubringer oder Übergabe. **Storno ist nur vor Abfahrt möglich**. Während der Fahrt bleibt der Auftrag in Fortsetzung; eine abgewiesene Stornierung teleportiert keine Fracht. Nach Ankunft liegt die tatsächlich verbliebene Ware am gewählten Ziellager und das Schiff am Zielhub. Rückfahrt ist eine neue bezahlte Fahrt; kein automatisches Zurücksetzen. Der Fahrzeugeigentümer kann im vorhandenen Leerfahrt-Dialog auch eine kostenpflichtige Schiffsversetzung buchen. Die Frachtfirma ist weiterhin als Startfirma auswählbar.

## Endlicher Pilotumfang

Konfiguration: `GeoData/Scenarios/de_nl1913.json`, Abschnitt `water_service`, sowie `economy.WaterMaxChainHours`; Strecke und Tarife bleiben im MAP04-Netz.

| Startasset / Parameter | Wert und Herkunft |
|---|---|
| Schiff | `vehicle.carrier.rhine`, Eigentümer `company.carrier`, Start Rotterdam |
| Schiffstyp | `vehicle_type.rhine_ship`, Spielarchetyp, 10.000 kg, 10 km/h Stammdaten, 500 Cent tägliche Fixkosten |
| Eröffnungsbuchwert | 2.000.000 USD-Cent; deklarierter Anfangsbesitz, kein laufender Zuschuss |
| Hafen-Lkw Köln / Rotterdam | Bereits vorhandene `vehicle.carrier.1` / `vehicle.carrier.4`; keine zusätzlichen Lkw |
| Hafen-Lkw-Typ | Kopie des vorhandenen 2.000-kg-Lkw, `TerminalOnly=true`; nur innerhalb der Kette, keine autonome Fernfahrt oder direkte Einzelbuchung |
| Effektive Frachtkapazität | Minimum aus Schiff, beiden freien Zubringern und allen Netzkanten; hier höchstens 2.000 kg |
| Terminals | Bestehende MAP04-Anschlüsse, für die kurze Kette exklusiv gesichert; keine zusätzlichen Wirtschaftshubs oder Warenlager |
| Buchungsvorlauf | Genau `BookingLeadHours`, aktuell 1 Spielstunde; keine spätere Wunschabfahrt für diese Kette |
| Maximale Kettendauer | `WaterMaxChainHours=72`, einschließlich beider Zubringer und Umschläge |

Schiff und Hafen-Lkw nutzen die bestehende Eigentums-, Fixkosten-, Stellen-/Lohn- und Buchwertlogik. Der Schiffstyp ist ein **Spielwert**, keine Behauptung über einen konkreten historischen Schiffstyp oder dessen Besatzung. Das vorhandene vereinfachte Fahrzeug-Stellenmodell gilt auch hier; keine separate detaillierte Schiffsbesatzungssimulation.

Die Erstkonfiguration ergänzt ausdrücklich ein Schiff und seinen Buchwert. Alle Konten/Anfangsgelder, Warenbestände, Haushalte und übrigen Wirtschaftsassets bleiben gegenüber MAP04 unverändert. Die beiden Hafen-Lkw wechseln lediglich in den zugeordneten Hafendienst. Bei neuen Partien werden keine Schiffe aus dem Nichts nachgeliefert; bei Knappheit fehlt ein Angebot.

## Berechnung, Teilstrecken und Zahlung

`WaterQuote` verwendet dieselbe reine MAP04-Routenfunktion für Vorschau und verbindliche Buchung. Sie prüft historische Verfügbarkeit am Abfahrtstag, Flagge des tatsächlichen Anbieters und die gesamte Kette. Jahreswechsel außerhalb der recherchierten Freigabe wird abgewiesen. Kürzeste Dauer, danach Kosten und stabile IDs bestimmen den Weg.

- **Betrieb:** Summe aller konfigurierten Kantenkosten einschließlich beider lokaler Fahrten und Lade-/Entladevorgänge. Kein zusätzlicher Luftlinienpreis für die Schiffsstrecke.
- **Dauer:** `ceil(Summe Kantenminuten / 60)` Spielstunden. Stromaufwärts aktuell 50, stromabwärts 32 Stunden. Umschlag und Zubringer sind enthalten; der separate Vorlauf wird als Wartezeit angezeigt.
- **Fremdfracht:** `ceil(Betrieb * (1000 + FreightMarkupPermille) / 1000)`. Der bestehende Aufschlag beträgt 200 Promille. Eigene Fahrt kostet den Betrieb ohne Fremdfrachtaufschlag.
- **Leerfahrt:** Keine Waren-/Zielbestandsreservierung. Endliche Flotte, Terminals, Betrieb und Umschlag bleiben kostenpflichtig. Für den bestehenden MAP04-Tarif wird die Mindestbemessung von 1 kg verwendet; es wird dabei keine Ware erzeugt. Das sind aktuell 734 Cent eigener Betrieb.
- **Bei Buchung:** Auftraggeber reserviert volle sichere Kosten, Fremdanbieter zusätzlich seinen Betrieb. Quellmenge, Zielpunkte, Schiff, zwei verfügbare gleichgehörige Hafen-Lkw und beide Terminalanschlüsse werden erst nach allen Prüfungen gemeinsam gebunden. Ein zweiter Auftrag kann diese Ressourcen nicht nochmals nutzen.
- **Vor Abfahrt stornieren:** Alle zugehörigen Geld-, Waren-, Ziel-, Flotten- und Terminalbindungen werden zusammen freigegeben. Bereits erfolgte Marktgeschäfte bleiben erhalten. Andere laufende Betriebsregeln dürfen weiterhin eigene Reservierungen halten.
- **Bei Abfahrt:** Fremdfracht wird an das echte Konto des Anbieters überwiesen; der Anbieter bezahlt den konfigurierten Gesamtbetrieb an das bestehende Betriebskostenkonto. Eigene Fahrt bezahlt Betrieb direkt. Die vorhandene Buchhaltung übernimmt Frachtkosten in den Warenbuchwert beziehungsweise Leerfahrtaufwand und erfasst den Serviceertrag. Keine Geldvernichtung oder Gutschrift ohne Gegenkonto.
- **Während der Kette:** Genau ein reales Frachtinventar mit unverändertem Eigentümer. Kein zweites Terminalinventar, keine Kopie pro Teilstrecke. Verderb und Buchkosten laufen weiter. Der physische Abschnitt wird aus gespeicherter Kantenfolge und abgelaufenen Spielminuten ermittelt, unabhängig von Kamera und Sichtbarkeit.
- **Bei Ankunft:** Tatsächlich verbleibende kg/Buchwerte gehen genau einmal in den Zielbestand. Reservierte Zielpunkte und Flotte werden freigegeben; Schiff endet am Ziel, lokale Zubringer bleiben an ihrem Hafen. Terminals halten keine Ware zurück und erzeugen keine Lagerkapazität.

Diese erste Kette ist bewusst konservativ: Beide Terminals und Zubringer bleiben für die gesamte kurze Fahrt gebunden, auch zwischen ihren einzelnen Einsätzen. Damit sind Anschlüsse jetzt sicher planbar. Es gibt noch keinen parallelen Terminaldurchsatz, langfristigen Fahrplan oder eine unbegrenzt im Voraus reservierbare Flotte. Das Modell begrenzt die Bindung auf 72 Stunden und bietet nur die nächste Abfahrt an; keine monatelangen Fahrzeugbindungen.

## Karte und Zeit

Wasseraufträge verwenden ausschließlich die gespeicherten MAP04-Stützpunkte, einschließlich konfigurierter lokaler Anschlüsse, ohne frei gebogene Hub-Luftlinie. Das exemplarische blaue Schiffssymbol bewegt sich nur im Wasserteil dieser Linie. Es ist kein zusätzliches Simulationsasset und zeigt keine metergenaue Schiffsposition. Linien/Symbole, Zoom, Ausblenden und Zeitraffer sind rein lesend; Ankunft und Fracht werden ausschließlich in der Simulation bestimmt. Nicht projizierbare Linien werden als Ganzes ausgelassen.

Im Europa-Szenario werden alte schematische Straßenkurven ohne freigegebene Netzgeometrie nicht mehr gezeichnet: Sie würden sonst ungesicherte offene Wasserquerungen behaupten. Die bestehenden Straßenangebote und ihre Transportabwicklung bleiben verwendbar und in den Auftragslisten sichtbar. Historisch konfigurierte Fernstraßengeometrien für sämtliche 50 Hubs sind weiterhin eine Datenlücke. Die US-Regressionsdarstellung bleibt unverändert. Die Genauigkeitsgrenzen der generalisierten Wasserstützpunkte aus MAP04 bleiben bestehen; Linien sind keine historische Tiefen- oder Ufervermessung.

## Persistenz und Prüfung

Save-Version **11**, keine Migration, alte Partien werden verständlich abgelehnt. Neue persistierte Felder am vorhandenen Auftrag: Netz-ID, geordnete Kanten-IDs und zwei Zubringer-Asset-IDs. Die vorhandenen Abfahrts-/Ankunftsstunden definieren die Teilstreckenfortsetzung. Laden prüft erneut Route, Zeiten, Kosten, Modus, Kapazität, Flotten-/Terminalbindungen und Gesamtbilanzen; beschädigte Dateien verändern den laufenden Zustand nicht. Die Helfer-/Markerfelder im Snapshot erzeugen keinen weiteren Savezustand.

**104/104 Gesamttests** bestanden, einschließlich vier neuer MAP05-Tests. Nach den abschließenden Bezeichnungs-/Darstellungsänderungen nochmals **4/4 MAP05-Tests** bestanden. Editor, Game Development und Shipping erfolgreich. Native Sicht-/Klickprüfung bei 1280×720 und 1920×1080, einschließlich Vorschau/Scrollen, Bestätigen und aktiver Kette bei 720p. Die Native-Prüfung benutzt echte laufende Simulation; die vollständige Ankunft und Save-Fortsetzung sind zusätzlich automatisiert geprüft. Kein neuer Cook/Installer-Lauf in diesem Paket.

Die Tests prüfen Zahlung an endliche Konten, Betrieb, tatsächliche Fracht/Verderb, Zielkapazität, fehlenden Wasseranschluss, Zubringerkapazität, zu späte Abfahrt, doppelte Buchung, atomaren Storno, abgewiesenen Storno während Fahrt, beschädigte Kanten-/Zubringerreferenzen, bezahlte eigene Leerfahrt, Save/Load während eines Schiffsabschnitts und identische Fortsetzung bis über die einmalige Ankunft hinaus. Beispieldurchlauf: 100 kg gebucht, 98 kg nach Verderb geliefert, 1.119 Cent Fremdfracht, 932 Cent Betrieb, Abfahrt Stunde 1, Ankunft Stunde 51. Die Wirtschaftsbilanzen bleiben gültig.

Nachweise: [Verification/MAP05](Verification/MAP05/README.md). Die historischen Quellen werden aus [MAP04](MAP04.md) weiterverwendet; Kosten, Anfangsasset und Betrieb sind ausdrücklich neue konfigurierbare Spielannahmen, keine kostenpflichtig recherchierten Daten.
