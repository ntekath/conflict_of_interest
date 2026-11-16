# Conflict of Interest – Entwicklungsauftrag und Arbeitspakete

Stand: 16.11.2026. Vollständiger konzeptioneller Neustart mit US-Kartenausschnitt und Spielbeginn 1913. Die folgende Paketfolge ersetzt die bisherige europäische Planung; alle Pakete gelten für diesen Neustart als offen. Der bestehende Projektcode wurde hier nicht geprüft. Vorhandene Arbeit nicht automatisch löschen: für einen neuen Projektstand bzw. Branch sichern und nur passende technische Teile bewusst übernehmen.

## Masterprompt für Codex

Entwickle ein Singleplayer-Strategie-, Aufbau- und Wirtschaftsspiel mit Unreal Engine 5 und C++. Prüfe zuerst das vorhandene Projekt, seine UE-Version und Projektregeln. Verwende die vorhandene Engine-Version und kompatible APIs. Setze die folgenden Arbeitspakete nacheinander um; beginne erneut mit AP01 für den neuen US-Projektstand. Nach jedem Paket liefere eine lauffähige, überprüfbare Zwischenversion, eine kurze Zusammenfassung, Prüfschritte und bekannte Einschränkungen. Implementiere spätere Pakete erst nach entsprechendem Auftrag.

### Spielvision

Der Spieler beginnt als kleiner Händler in einer simulierten Weltwirtschaft auf der echten Erde. Er organisiert Rohstoff- und Warenlieferungen, erschließt Märkte und kann später Produktionsstätten, Lager und Transportinfrastruktur bauen oder übernehmen. Ganze Lieferketten können wirtschaftlich kontrolliert werden.

Länder, Regionen und andere Unternehmen entwickeln sich unabhängig vom Eingreifen des Spielers, solange die Simulation läuft. Offline-Fortschritt bei geschlossenem Spiel ist zunächst nicht vorgesehen. Versorgung, Konsum, Produktion und Kaufkraft erzeugen Handelsmöglichkeiten. Langfristig beeinflussen Politik, Konflikte, Konkurrenz und Währungen die Wirtschaft.

Die Welt soll nachvollziehbar und spielerisch interessant sein. Reale Geografie ist Grundlage; die anfänglichen Wirtschaftsdaten sind ausdrücklich fiktiv.

### Identität: Conflict of Interest

Ziel ist der Aufbau eines Weltkonzerns durch Handel, Produktion, Logistik, Übernahmen und wirtschaftlichen Einfluss. Versorgung verbessern, Lieferketten kontrollieren oder Abhängigkeiten ausnutzen sind mögliche Strategien.

Das Spiel ermöglicht moralisch verwerfliches Verhalten: Waren zurückhalten, auf klimabedingte Knappheit spekulieren, Konkurrenz verdrängen und später Handelsbeschränkungen über Zwischenhändler oder Drittstaaten umgehen. Dies sind abstrakte Spielsysteme; konkrete reale Umgehungsanleitungen sind nicht Teil des Designs.

Konsequenzen entstehen aus Wirtschaftsregeln und reagierenden Akteuren. Kein globaler Moralwert: Versorgung, Vertrauen, Beziehungen, regulatorische Risiken und Unternehmensgewinn sind getrennte Größen. Spekulation birgt echte Verluste, weil Prognosen unsicher sind, Waren verderben und Lager Geld kosten. Für die ersten Pakete keine vollständigen Politik- oder Reputationssysteme vorwegnehmen.

### Wirtschaftsmodell: Akteure, Konsumenten und regionale Märkte

**Verbindliche Grundlage ab AP04/AP05, keine spätere Erweiterung nach AP09.**

Unternehmen sind individuelle, aktive Marktakteure mit eigenem Geld, Warenbesitz, Standorten und Entscheidungsregeln. Sie können Konkurrent, Kunde und Lieferant zugleich sein. Produzenten, Händler und Transportanbieter verfolgen eigene Interessen. Staaten werden erst mit den Politikpaketen aktive wirtschaftspolitische Akteure; ihre spätere Rolle verlangt jetzt kein vollständiges Staatssystem.

Die Bevölkerung wird regional in wenige Konsumentengruppen mit Größe, Einkommen, Budget, Bedürfnissen und Versorgungszustand zusammengefasst. Bedürfnisse erzeugen Kaufaufträge; tatsächliche Nachfrage ist durch Kaufkraft und Zahlungsbereitschaft begrenzt. Gekaufte Konsumgüter werden verbraucht und verlassen die Warenbestände. Geld geht an den Verkäufer und verschwindet nicht. Langlebige Güter können später als Nutzungsbestand statt sofortigem Verbrauch modelliert werden.

Landeswerte aggregieren regionale Bevölkerung und Verbrauch. Kein zusätzlicher nationaler Konsum derselben Menschen. Drei regionale Einkommensgruppen genügen zunächst. Grundbedürfnisse haben Budgetvorrang vor Luxusbedürfnissen. Nahrung ist die erste Grundbedarfsware; Luxuswaren bilden eine einfache zusätzliche Kategorie. Weitere Bedürfnisarten und Substitution folgen später.

**Marktmechanik**
- Ein regionaler Markt ist ein Handelsplatz mit Kauf-/Verkaufsaufträgen und Geschäftsverlauf, kein eigentümerloser Waren- oder Geldbestand.
- Aufträge enthalten Akteur, Markt, Ware, Menge, Preislimit, Erstellungssequenz und Ablaufzeit. Marktzuordnung von Städten und Standorten eindeutig festlegen.
- Verkäufer reservieren eigene verfügbare Ware am Marktort; Käufer reservieren Geld. Teilausführung, Stornierung und Ablauf geben verbleibende Reservierungen korrekt frei.
- Passende Aufträge werden nach Preis und anschließend Eingangssequenz zusammengeführt. Eigenhandel ausschließen. Für den Prototyp gilt der Preis des älteren passenden Auftrags; Rundung und Mengeneinheiten dokumentieren.
- Eine Ausführung überträgt Geld und Wareneigentum atomar. Angebote verkaufen keine Ware aus einer anderen Region oder aus laufenden Transporten.
- Preis entsteht aus ausgeführten Geschäften und den Preisentscheidungen der Akteure. Kein zentraler Knappheitsmultiplikator als Ersatz für Handel.
- Letzten Abschlusspreis mit Alter, bestes Kauf-/Verkaufsangebot, handelbare Menge und Volumen getrennt anzeigen. Ohne Geschäfte keinen fiktiven aktuellen Marktpreis erzeugen.
- Produzenten kalkulieren aus Inputkosten, laufenden Kosten, Beständen und konfigurierter Marge. Käufer berücksichtigen Budget und Zahlungsbereitschaft. Einfaches periodisches Anpassen genügt zunächst.
- Märkte sind lokal; räumlicher Ausgleich benötigt Transportzeit und Kosten. Spieler und KI verwenden dieselben Auftrags-, Reservierungs- und Abwicklungsregeln.
- Unternehmen kennen eigene Daten und veröffentlichte Marktinformationen. Keine Einsicht in geheime Spielerpläne oder tatsächliche zukünftige Krisen.

**Einkommens- und Mengenbilanz**
Löhne sind Übertragungen von Arbeitgeberkonten an regionale Konsumentengruppen. Falls der kleine Prototyp zusätzlich Transfers oder einen externen Sektor benötigt, diese aus einem expliziten Konto finanzieren oder als ausgewiesene externe Geldquelle buchen. Keine unsichtbare tägliche Budgetauffüllung. Anfangskapital ist Szenariokonfiguration. Zahlungen an Transport- oder Lageranbieter müssen ebenfalls einen Empfänger haben; ein zunächst vereinfachtes Dienstleisterkonto genügt.

Produktion, Inputverbrauch, Haushaltskonsum und Verderb sind ausgewiesene Warenquellen bzw. -senken. Handel und Transport allein verändern die Gesamtmenge nicht. Fehlende Kaufkraft, unverkaufte Ware und Verluste dürfen auftreten. Das Szenario darf nicht durch versteckte Gratislieferungen stabilisiert werden.

### Darstellung und Bedienung

Die Hauptoberfläche ist eine zusammenhängende, flache 3D-Strategiekarte mit später optionalem Höhenrelief. Orientierung für Zoom, Auswahl und Informationsdichte ist Crusader Kings 3; Gestaltung und Assets sind eigenständig.

Die Karte zeigt abhängig vom Zoom Länder, Regionen, Städte, Betriebe und Transportverbindungen. Informationspanels ergänzen die Karte. Es gibt zunächst keine separate begehbare Welt und keine Stadtbauansicht.

### Technische Leitplanken

- C++ für Simulation, Datenmodell, Import, Karteninteraktion und Spielbefehle. Blueprints, Materialien und UMG für visuelle Ausgestaltung und UI. C++ stellt gezielte Schnittstellen bereit.
- Simulationskern ohne Abhängigkeit von Actors, Widgets oder dem geladenen Kartenausschnitt entwickeln. Eine schlanke UE-Anbindung verwaltet Start, Pause und Updates.
- Simulation in festen Zeitschritten; Darstellung pro Frame. Zeitbeschleunigung führt zusätzliche Schritte aus und verändert nicht die wirtschaftlichen Formeln.
- Städte, Waren, Unternehmen und Transporte als Daten mit stabilen IDs speichern. Kein Actor pro wirtschaftlichem Datensatz und kein individueller Actor-Tick für die Wirtschaft.
- Sichtbare Actors und Komponenten stellen ausgewählte Daten dar. Wirtschaftliche Zustände bleiben unabhängig von Sichtbarkeit und Streaming bestehen.
- Statische Definitionen und veränderlichen Spielzustand trennen. Daten extern bearbeiten und beim Import validieren. Keine Stadtlisten im Darstellungscode.
- Geld vorzugsweise in ganzzahligen Untereinheiten führen; Mengeneinheiten und Rundung festlegen. Negative Bestände und doppelte Buchungen verhindern.
- Kleine, nachvollziehbare Module statt vorzeitigem Frameworkbau. Parallelisierbare Datenflüsse ab AP04 vorsehen; Worker-Ausführung nach Profiling schrittweise ergänzen. Siehe folgende Threading-Regeln.

### Multithreading, Skalierung und zeitliche Konsistenz

**Architektur ab AP04 vorbereiten; korrekte serielle Referenz zuerst.** Kein Thread pro Stadt, Unternehmen oder Transport. Keine Verpflichtung, die gesamte Wirtschaft sofort auf einen eigenen Dauerthread zu verschieben.

1. **Autoritativer Zustand:** Ein klar benannter Besitzer schreibt den Simulationszustand. UI und Worker verändern ihn niemals gleichzeitig. Ein abgeschlossener Simulationsschritt wird als konsistenter Zustand veröffentlicht.
2. **Befehle:** Spieler- und KI-Aufträge besitzen Zielschritt und stabile Sequenznummer. Ein zentraler Schritt validiert und verbucht sie in festgelegter Reihenfolge. Gleichzeitige Käufe dürfen nicht denselben Bestand mehrfach beanspruchen.
3. **Berechnen und verbuchen trennen:** Unabhängige Berechnungen lesen unveränderliche Eingaben und schreiben eigene Ergebnisbereiche. Anschließend werden Ergebnisse deterministisch zusammengeführt. Produktionsketten und Märkte erhalten dokumentierte Phasen; neue Outputs werden nicht zufällig durch Task-Reihenfolge früher verfügbar.
4. **Geeignete Kandidaten:** Regionale Bedarfsberechnungen, Produktionsvorschläge, Routenberechnung und KI-Bewertungen. Kaufabschlüsse und globale Geld-/Warenbuchungen bleiben zunächst zentral geordnet. Keine gemeinsame schreibende Map oder Liste ohne explizites Synchronisationskonzept.
5. **UE-Grenze:** Actor-, Komponenten-, Material- und UMG-Änderungen auf dem Game Thread ausführen. Worker erhalten reine Daten, keine frei verwendbaren UObject-/Actor-Zeiger. Abweichungen nur für nachweislich threadsichere Engine-APIs mit dokumentierter Lebensdauer.
6. **Ausführung:** Für begrenzte Rechenaufträge das zur installierten Version passende UE Tasks System bzw. ParallelFor nutzen. Eigene FRunnable-Threads erst bei begründetem Dauerbedarf. Parallelisierung muss abschaltbar sein; serielle und parallele Pfade verwenden dieselben Regeln.
7. **Determinismus:** Stabile Iterations- und Zusammenführungsreihenfolge; keine unkontrollierte Hash-Container-Reihenfolge. Zufallsströme aus Seed, Schritt und stabiler Entitäts-ID ableiten. Keine gemeinsam konkurrierend verwendete Zufallsquelle. Plattformübergreifende Bitgleichheit wird zunächst nicht versprochen.
8. **Snapshots:** Darstellung liest einen veröffentlichten, versionierten Ausschnitt des Zustands. Keine vollständige Weltkopie pro Frame; geänderte oder benötigte Daten übertragen. Speicherstände beziehen sich auf eine abgeschlossene Schrittgrenze.
9. **Rückstau:** Pro Frame begrenztes Simulationsbudget. Bei Überlast läuft Spielzeit langsamer, ohne Schritte still zu überspringen oder eine unbegrenzte Task-Warteschlange aufzubauen. UI zeigt die tatsächlich erreichte Geschwindigkeit.
10. **Lebensdauer:** Beim Laden, Szenariowechsel, PIE-Ende und Shutdown laufende Jobs sauber beenden oder ihre Ergebnisse über eine Szenario-Generation verwerfen. Alte Ergebnisse dürfen niemals einen neuen Spielzustand verändern. Game Thread nicht pro Frame auf lange Jobs blockierend warten lassen.
11. **Messung:** Ab AP04 Trace-Marker und Zeiten je Simulationsphase vorsehen. Zusätzlich Schrittzeit, Game-Thread-Zeit, Warteschlangenlänge und Snapshot-Kosten erfassen. Erst messen, dann die teuersten unabhängigen Berechnungen auslagern.
12. **Prüfung:** Auf derselben Plattform mit identischen Befehlen/Seeds serielle und parallele Ergebnisse nach gleicher Schrittzahl vergleichen. Auch Konkurrenz um knappe Bestände, Pause, Laden und Shutdown prüfen.

### Geografische Leitplanken

- Echte Grenzen, Küstenlinien und Stadtkoordinaten aus geeignet lizenzierten Geodaten verwenden. Quelle, Datenstand, Lizenz und Attribution dokumentieren.
- Geografische Quelldaten in WGS84-Längen-/Breitengraden behalten. Projektion, Kartenskalierung und UE-Koordinaten durch eine zentrale Transformation verbinden.
- Karte verkleinert darstellen; keine Erde im Maßstab 1:1. Entfernung und Reisezeit nicht aus Bildschirm- oder Meshabständen ableiten.
- Für den regionalen Prototyp eine dokumentierte Projektion wählen; ihre Austauschbarkeit vorsehen. Weltweite Darstellung, Polgrenzen und Datumsgrenze als spätere Aufgaben dokumentieren.
- Geometrie offline oder im Editor importieren, vereinfachen und triangulieren. Ländergrenzen nicht bei jedem Spielstart neu verarbeiten. Löcher, Inseln und Mehrfachpolygone korrekt behandeln.
- Statische Meshes für vorbereitete Landflächen bevorzugen. Geometry Scripting oder Dynamic Mesh nur verwenden, wenn für die installierte UE-Version sinnvoll.
- Kartenlayer ändern die Darstellung derselben Geografie. Zunächst Materialparameter pro Region verwenden; spätere Regions-ID-Textur/Farbtabelle als Optimierungsoption dokumentieren.
- World Partition, Nanite und komplexes Streaming nur bei nachgewiesenem Bedarf einsetzen.
- Laufzeit offline ermöglichen; kein kostenpflichtiger Kartendienst erforderlich.

### Vorgeschlagene Verantwortlichkeiten

Die Namen sind Vorschläge, keine Pflichtstruktur:

| Bestandteil | Verantwortung |
|---|---|
| SimulationCore | Simulationszustand, Uhr, Regeln und Befehle |
| GeoData / Import | Geodaten, Projektion, IDs und Datenvalidierung |
| MapRenderer | Meshes, Grenzen, Farben, Symbole und Routen |
| MapCameraPawn | Verschieben, Zoom und Kameragrenzen |
| PlayerController | Auswahl und Weitergabe von Spielerbefehlen |
| UMG-Oberfläche | Informationen, Layerwahl und Handelsdialog |
| Save-System | Versionierter Simulationszustand und Referenzen |

### Szenario: USA 1913

- Spielbeginn 01.01.1913 als konfigurierbarer Szenariostart. Historische Ereignisse sind zunächst nicht fest geskriptet.
- Ein zusammenhängender US-Ausschnitt rund um Detroit, Chicago, Cleveland und Pittsburgh dient als erster Entwicklungsumfang. Diese Ortsauswahl ist ein veränderbarer Testvorschlag; kein vollständiges Abbild aller vier Staaten erforderlich. Europa gehört nicht zum ersten Szenario.
- Echte Stadtpositionen, Große Seen und geografische Beziehungen verwenden. Wirtschaftsregionen sind explizit gekennzeichnete Spielgebiete innerhalb der USA, keine frei erfundenen Länder.
- Historische Verkehrs-, Produktions- und Bevölkerungswerte vor Verwendung recherchieren und Quellen nennen. Ungeprüfte Werte als Balancingwerte kennzeichnen. Kein heutiges Autobahnnetz oder moderne Technik als historische Realität darstellen.
- Rohstoffvorkommen und landwirtschaftliche Eignung sind geografisch begrenzt. Betriebe können Rohstoffe nur an geeigneten Standorten fördern bzw. erzeugen.
- Fünf anfängliche Waren: Nahrung, Holz, Erz, Werkzeuge und abstrakte Luxuswaren. Eine gemeinsame Währung USD, intern ganzzahlige Cents; Preisniveaus sind zunächst Testwerte.
- Mindestens zwei Produzenten, ein eigenständiger Händler und ein Transportunternehmen mit getrennten Konten, Besitzbeständen und einfachen Entscheidungsregeln. Für Waren außerhalb der vollständig modellierten Ketten sind deklarierte Betriebe oder ein offen bilanzierter externer Sektor zulässig.
- Spielerstart an einem Ort, als vorläufiger Testwert Detroit: ein eigenes Lagerhaus, ein eigener Lkw und konfigurierbares Startkapital. Keine kostenlose unbegrenzte Lagerung und keine unbegrenzte Flotte.
- Alle Mengen, Kosten, Kapazitäten und Startbestände liegen in Szenariodaten. Sie sind veränderbare Testwerte, keine historischen Behauptungen.
- Erster Wirtschaftskonflikt: Lebensmittelversorgung und Lagerhaltung bei unsicherem Dürre-Risiko. Ein freies Testszenario, keine vorgeschriebene moralische Handlungsfolge.

### Verbindlicher Datenmodell-Vertrag

Definitionen beschreiben Typen und Regeln; Laufzeitdaten beschreiben konkrete Entitäten und ihren Zustand. Kein Zwang zu bestimmten Klassennamen.

| Entität | Definition / Stammdaten | Laufzeitdaten / Beziehungen |
|---|---|---|
| Land / Region | ID, Name, Geometrie, Landeszugehörigkeit | Aggregierte Kennzahlen; Region besitzt genau einen Markt |
| Ort | ID, Name, Koordinaten, Region-ID | Lokale Betriebe, Lager und Fahrzeuge |
| Ware | ID, Mengeneinheit, Lagerbedarf je Einheit, Verderbsregel, Bedarfskategorie | Mengen ausschließlich in zugeordneten Beständen |
| Unternehmen | ID, Name, Entscheidungsprofil | Konto, Besitz an Betrieben/Lagern/Fahrzeugen, Aufträge |
| Betriebsstätte | Typ, Produktionsrezept, Kapazität, Personalbedarf | Eigentümer, Ort, Beschäftigung, Inputs/Outputs, Auslastung |
| Konsumentengruppe | Region, Bedürfnisprofil und Einkommensgruppe | Bevölkerung, Erwerbspersonen, Beschäftigte, Geld, Konsum und Fehlbedarf |
| Lagerhaus | Lagerkapazität in Lagerpunkten, fixe Tageskosten | Eigentümer, unveränderlicher Ort, eingelagerte Bestände, reservierte Zielkapazität |
| Fahrzeug | Typ, Ladekapazität, laufende Kosten, Geschwindigkeitsparameter | Eigentümer, aktueller Ort bzw. Auftrag, verfügbar/gebunden |
| Warenbestand | Ware, Eigentümer und physischer Speicherort | Gesamtmenge und reservierte Teilmenge; keine unabhängige Doppelkopie |
| Marktauftrag | Akteur, Ware, Preislimit, Frist | Restmenge, Reservierungen, Ausführungen, Markt-ID |
| Transportauftrag | Start/Ziel, Ware, Menge, eigener/gemieteter Dienst | Fracht, Abfahrt, Ankunft, Kosten, gebundene Ressourcen, Status |

Kilogramm als ganzzahlige Warenbasiseinheit und Stunden als Simulationsschritt sind erste technische Testvorgaben. Tonnen werden in der Oberfläche umgerechnet. Lagerbedarf pro kg über ganzzahlige Lagerpunkte definieren, damit verschiedene Waren Kapazität unterschiedlich belegen können. Rundung, Zeitpunkte täglicher Kosten und Lohnzahlungen dokumentieren. Größen bleiben konfigurierbar.

### Beschäftigung und Konsum

Arbeitskräfte sind aggregierte regionale Bevölkerungsgruppen, keine einzeln bewegten Personen. Betriebe schreiben Stellen mit Lohn aus; eine einfache stabile Zuordnung bevorzugt höhere angebotene Löhne, danach eine dokumentierte ID-Reihenfolge. Beschäftigte können nicht gleichzeitig mehrere Vollzeitstellen besetzen. Pendeln zwischen Regionen ist zunächst ausgeschlossen.

Produktionsleistung ist durch Inputs, Kapazität, Standortvoraussetzungen und verfügbare bezahlte Arbeitskraft begrenzt. Löhne gehen vom Arbeitgeberkonto an die jeweilige Gruppe. Vor Produktion die nächste Lohnverpflichtung berücksichtigen; ohne Deckung keine kostenlose Arbeit. Arbeitslose nutzen vorhandene Ersparnisse und gegebenenfalls explizit finanzierte Transfers. Konsum kauft Grundbedarf zuerst und Luxus nur aus verbleibendem Budget. Unerfüllter Bedarf ist sichtbar, aber es gibt zunächst kein automatisch tödliches Bevölkerungsmodell.

### Transport und Lager: Simulation im Menü, Visualisierung auf der Karte

**Transport wird geplant und gebucht, nicht durch direktes Fahren gesteuert.**

- Planungsmenü: Startort, Zielort, eigene verfügbare Ware, Menge, Abfahrt sowie eigenes Fahrzeug oder Fremdanbieter wählen. Kapazität, Dauer, Kosten und voraussichtliche Ankunft vor Bestätigung anzeigen.
- Der eigene Lkw ist ein Unternehmensasset. Er kann während eines Auftrags nicht anders eingesetzt werden. Nach Ankunft steht er am Ziel. Rück- oder Leerfahrt ist ein eigener kostenpflichtiger Auftrag; kein Zurückteleportieren.
- Fremdtransporte werden bei einem aktiven Transportunternehmen mit begrenzter Verfügbarkeit und Kapazität gebucht. Es erhält die Zahlung und bindet eigene Ressourcen. Es handelt nicht automatisch mit der transportierten Ware.
- Zum Start reichen abstrakte Verbindungen zwischen Orten mit Entfernung, Reisezeit-/Kostenparametern und grobem Verlauf. Kein vollständiger Straßengraph, keine Fahrphysik, kein NavMesh und keine Simulation einzelner Kreuzungen erforderlich.
- Später Bahn/Schiff als weitere Dienst- und Assettypen ergänzen. Die erste Version implementiert nur Lkw; Datenmodell enthält eine Transportart, aber keine leeren umfassenden Mehrverkehrssysteme.
- Auf der Karte erscheinen später grobe Routen und exemplarische Fahrzeuge. Sie repräsentieren Transportaktivität, nicht zwingend jeden einzelnen Auftrag. Dichte und Anzahl sind begrenzt und zoomabhängig.
- Transportfortschritt und Zustellung beruhen ausschließlich auf Simulationszeit. Animation, Bildrate, Sichtbarkeit und Anzahl dargestellter Fahrzeuge beeinflussen sie nicht.
- Lager sind an konkrete Orte gebunden. Sie haben begrenzte Kapazität und zeitabhängige Fixkosten auch im leeren Zustand. Güter im Lager behalten ihren jeweiligen Eigentümer.
- Unternehmen besitzen oder mieten tatsächlich verfügbare Lagerkapazität. Produzenten haben begrenzte Betriebslager. Für Spielerlieferungen an Orte ohne eigenes Lager bietet ein konfigurierter Dienstleister begrenzte kostenpflichtige Kapazität.
- Bei einem Kauf wechselt Eigentum, nicht automatisch der Standort: Ware bleibt bis zur Abholung im bisherigen Lager, mit dokumentierter Abholfrist und offen ausgewiesenen Lagergebühren.
- Transportbuchung reserviert Ware, Fahrzeug-/Anbieterkapazität, Zahlungsdeckung und freie Zielkapazität gemeinsam oder lehnt vollständig ab. Zielkapazität bleibt bis zur Ankunft gebunden. Stornoregeln vor Abfahrt definieren; nach Abfahrt ist der Auftrag im ersten Prototyp nicht stornierbar.
- Ein Kaufabschluss wird nicht rückgängig, weil der anschließende Transport scheitert. Ware bleibt am Quellort. Angebote nennen deshalb Standort und Lagerbedingungen.
- Ein Regionsmarkt bündelt Preisfindung, ersetzt aber keine physischen Standorte: Aufträge referenzieren den Liefer-/Abholort. Zunächst nur kompatible Standortangebote direkt zusammenführen; das UI zeigt sie nach Ort gefiltert im regionalen Orderbuch. Für andere Orte muss ein Transport ausdrücklich gebucht werden.
- Haushaltskonsum erfolgt über lokale Angebote an konfigurierten Verbrauchsorten; Unternehmensinputs müssen am Betriebsort liegen. Kein unsichtbarer kostenloser Warentransfer innerhalb einer Region.
- Betriebskosten, Fixkosten und Gebühren werden an explizite Konten oder einen dokumentierten externen Sektor gezahlt. Keine verdeckten Geldlöschungen. Verderb betrifft passende Lager- und Frachtbestände gleichermaßen.
- Ohne Deckung neue Aufträge stoppen und Zahlungsrückstände sichtbar führen; keine stillen negativen Geldbestände. Vollständige Insolvenzmechanik folgt später.

## Arbeitspakete

### AP01 – Projektgrundlage und Kameraprototyp

**Ziel:** Ein neuer startbarer UE5-C++-Projektstand für Conflict of Interest mit bedienbarer Strategiekamera.

**Aufgaben**
- Vorhandenen Stand sichern und für den konzeptionellen Neustart einen neuen Branch oder Projektstand verwenden. Keine vorhandenen Dateien ohne Not löschen.
- UE-Version, Projektregeln und passende Build-Abhängigkeiten festlegen.
- Leeres Kartenlevel, PlayerController, Kamera-Pawn und Eingabebelegung einrichten.
- Verschieben, Zoom unter Berücksichtigung einer nachvollziehbaren Zoomposition und Kamerabegrenzung implementieren.
- Eine einfache Platzhalterfläche verwenden; geografischen Import noch nicht vorwegnehmen.
- Kurze Architektur- und Startdokumentation anlegen.

**Abnahme**
- Projekt kompiliert und startet im Editor.
- Kamera lässt sich flüssig bewegen und innerhalb definierter Grenzen zoomen.
- Bedienung und notwendige Editor-Konfiguration sind dokumentiert.

### AP02 – Reale Geodaten und Importpipeline

**Voraussetzung:** AP01.

**Ziel:** Wiederholbar erzeugbare US-Testkarte mit echten Ortspositionen.

**Aufgaben**
- Geeignete Datenquellen und Lizenzen prüfen und dokumentieren.
- Import-/Vorverarbeitungsskript für Grenzen und Stadtkoordinaten erstellen.
- Projektion, Skalierung, Ursprung und geografische Distanzfunktion zentral implementieren.
- Vereinfachte Land-/Seenmeshes, Wirtschaftsregionen und die vier Teststadtpositionen erzeugen. US-Ausschnitt in einer geeigneten regionalen Projektion darstellen.
- Stabile IDs und Importvalidierung für Geometrie und Ortszuordnung einführen.
- Falls ein Editor-Importschritt notwendig ist, dessen Ablauf exakt dokumentieren; generierte Daten für paketierte Builds verfügbar machen.

**Abnahme**
- Große Seen und US-Testgebiet sind erkennbar, Stadtpositionen plausibel. Kein europäischer Szenariodatensatz bleibt als aktive Grundlage bestehen.
- Inseln und Löcher erzeugen keine offensichtlichen Geometriefehler.
- Import ist ohne manuelles Nachzeichnen reproduzierbar.
- Zwei Referenzdistanzen werden gegen unabhängig berechnete geografische Entfernungen geprüft.
- Datenquelle, Stand und Lizenz sind enthalten.

### AP03 – Auswahl, Ortsinformationen und politische Ansicht

**Voraussetzung:** AP02.

**Ziel:** Die Karte wird zur bedienbaren Spieloberfläche.

**Aufgaben**
- Länder, Wirtschaftsregionen und Städte unterscheidbar auswählbar machen.
- Auswahl priorisieren: sichtbares Stadtsymbol vor darunterliegender Fläche.
- Hervorhebung und UMG-Informationspanel implementieren.
- Regionen-/Besitzansicht sowie lesbare, zoomabhängige Beschriftungen ergänzen. Eine reine Länderfärbung reicht im US-Ausschnitt nicht.
- Testregionen und fiktive Wirtschaftsdaten klar kennzeichnen.

**Abnahme**
- Klick auf Stadt oder Fläche zeigt die korrekten Daten.
- UI-Klicks lösen keine versehentlichen Kartenbefehle aus.
- Auswahl bleibt beim Zoomen konsistent; Beschriftungen überladen die Karte nicht.

**Meilenstein A:** Echte, interaktive Karte. Hier zunächst Stabilität und Bedienung bewerten.

### AP04 – Simulationskern und Spielzeit

**Voraussetzung:** AP03.

**Ziel:** Eine von der Darstellung unabhängige, fortlaufende Welt.

**Aufgaben**
- Definitionen und Laufzeitdaten für Orte, Waren, Unternehmen, Betriebsstätten und regionale Konsumentengruppen einführen.
- Konten, Eigentumsbestände je Standort, Reservierungen, Markt-IDs und Auftrags-/Geschäftsdatentypen vorbereiten. Matching und Wirtschaftsregeln folgen in AP05.
- Den Datenmodell-Vertrag vollständig in Typen und Beispieldaten umsetzen: Beschäftigung, Grund-/Luxusbedarf, geografische Ressourcen, Lager/Fahrzeugassets und Transportangebote einschließen. Noch keine wirtschaftlichen Entscheidungsregeln implementieren.
- US-Startszenario mit Spielerfirma, Lagerhaus, einem Lkw und Startkapital laden; Länderwerte nur aus Regionen aggregieren.
- Feste Simulationsschritte, Pause und mehrere Geschwindigkeiten implementieren.
- Festgelegte Reihenfolge für Zustandsänderungen und einen expliziten Zufalls-Seed vorsehen.
- UI über Zustandsänderungen informieren, ohne Simulationsregeln im Widget auszuführen.
- Game-Thread-Anbindung, autoritativen Besitzer, Befehlsreihenfolge und Schrittphasen dokumentieren.
- Serielle Referenzausführung mit trennbaren Berechnungs-/Verbuchungsphasen bauen. Noch keine Worker-Pflicht.
- Datum, Pause und Geschwindigkeitssteuerung in der Oberfläche anzeigen.
- Trace-Marker, Schrittbudget und Diagnosewerte einführen; Lebensdauer bei PIE-Ende sauber behandeln.

**Abnahme**
- Pause verändert keinen wirtschaftlichen Zustand.
- Gleicher Startzustand und gleiche Befehle ergeben bei gleicher Schrittzahl dasselbe Ergebnis, unabhängig von Bildrate und Zeitbeschleunigung.
- Orte außerhalb des sichtbaren Bereichs werden genauso simuliert.
- Befehle werden genau einmal in dokumentierter Reihenfolge ausgeführt.
- Überlast erzeugt weder übersprungene Schritte noch unbegrenzten Rückstau.

### AP05 – Aktive Unternehmen, Konsumenten und lokale Märkte

**Voraussetzung:** AP04.

**Ziel:** Eine durch reale Akteursgeschäfte laufende Wirtschaft, auch ohne Spieler.

**Aufgaben**
- Fünf Waren, drei Konsumentengruppen je Region, Anfangskapital und asymmetrische Betriebe konfigurieren.
- Mindestens zwei Produzenten, einen Händler und einen Transportanbieter mit eigenen Konten, Assets, Waren und einfachen Regeln aktiv simulieren. Kein späterer Austausch künstlicher Märkte gegen Akteure.
- Produktion mit Inputbedarf, Outputs, Kapazitäten, Standortressourcen und Beschäftigung implementieren. Produzenten kaufen Inputs und bieten eigene Outputs an.
- Stellen, begrenzte regionale Erwerbspersonen, einfache Lohnzuordnung und gedeckte Lohnzahlungen implementieren; fehlende bezahlte Arbeitskraft reduziert Produktion.
- Regionales Orderbuch mit Preis-/Zeitpriorität, Preis des älteren Auftrags, Reservierungen, Teilausführung, Storno und Ablauf implementieren.
- Haushaltseinkommen aus Löhnen und gegebenenfalls explizit gebuchten Transfers zuführen. Herkunft und Empfänger jeder Zahlung dokumentieren.
- Grund- und Luxusbedürfnisse, durch Budget begrenzte Kaufaufträge, tatsächlichen Konsum und Fehlbedarf getrennt führen. Grundbedarf zuerst; kein Luxuszwang bei fehlendem Geld.
- Bedarfsmengen auf Konsumenten verteilen, ohne dieselbe Bevölkerung auf Stadt-, Regions- und Landesebene doppelt zu zählen.
- Preislimits der Akteure anhand einfacher Kosten-, Bestands- und Budgetregeln periodisch anpassen; weder garantierte Vollversorgung noch garantierte Gewinne.
- Einen minimalen abstrakten Transportdienst mit Verbindungen, Reisezeit, eigenen Fahrzeugassets und buchbarem Fremdanbieter bauen. Autonome Händler handeln und transportieren bereits; AP06 ergänzt die Spielerbedienung.
- Lagerkapazität, Reservierungen, Fixkosten und physische Warenstandorte von Beginn an durchsetzen. Spielerkäufe dürfen keine später unmögliche Lagerlogik voraussetzen.
- Marktpanel/Diagnose um letzte Geschäfte samt Alter, aktuelle Angebote, Volumen, Reservierungen und Versorgung erweitern.
- Szenario mehrere Spielwochen ohne Spielereingriff laufen lassen; Geld- und Warenbilanzen erklären, keine künstliche Stabilisierung verstecken.

**Abnahme**
- Nichtspieler produzieren, beschäftigen, kaufen und verkaufen ohne Spielereingriff. Der Händler transportiert zwischen Regionen und zahlt dem Anbieter.
- Standort ohne Vorkommen erzeugt keine Förderware; fehlende Beschäftigte begrenzen den Output.
- Konsumentengruppen erhalten gebuchte Löhne und kaufen Grundbedarf vor Luxus; Zahlungsunfähigkeit und Fehlbedarf werden sichtbar.
- Lager laufen nicht über, Fahrzeuge sind nicht mehrfach gebunden und Anbieter besitzen keine unbegrenzte Kapazität.
- Unternehmensausgaben werden Einnahmen anderer Akteure; Konsum entfernt Ware und zahlt an Verkäufer.
- Fehlende Inputs begrenzen Produktion, fehlendes Geld begrenzt Käufe.
- Zwei Käufer können denselben knappen Bestand nicht doppelt kaufen; reserviertes Geld kann nicht doppelt ausgegeben werden.
- Teilausführung, Storno und Ablauf liefern korrekte Restmengen/Reservierungen.
- Kein Geschäft findet außerhalb beider Preislimits statt; kein Eigenhandel.
- Kein Umsatz bedeutet keinen neuen Abschlusspreis. Ein alter Preis wird als veraltet angezeigt.
- Handels-/Transportbuchungen erhalten Gesamtgeld und Gesamtwaren; Produktion, Konsum, Verderb und externe Quellen werden separat bilanziert.
- Landesaggregation zählt Konsum nicht doppelt.

### AP06 – Transportmenü, eigene Assets und erste Handelslieferung

**Voraussetzung:** AP05.

**Ziel:** Der Spieler verdient durch räumlichen Warenausgleich Geld.

**Aufgaben**
- Spielerkonzern und Handelsdialog auf bestehende Akteurskonten und Orderbücher aufsetzen. Dieselben Regeln wie bei Nichtspielern verwenden.
- Kaufaufträge mit Preislimit, reserviertem Budget und Teilausführungen anbieten. Nur tatsächlich erworbene Ware ist transportierbar.
- Transportbuchung separat atomar validieren. Ein abgelehnter Transport macht einen bereits ausgeführten Marktkauf nicht rückwirkend ungültig; Ware bleibt im Eigentum des Spielers am Startort.
- Den Transportdienst aus AP05 im Planungsmenü nutzbar machen: eigener Lkw oder Fremdanbieter, Start/Ziel, Ware, Menge, Dauer, Kosten und Verfügbarkeit.
- Unternehmensübersicht für Lagergröße, Auslastung, Fixkosten sowie Fahrzeugstandort, Ladekapazität und Belegung ergänzen.
- Nach einer Marktausführung Waren dem Spieler am tatsächlichen Quelllager zuordnen. Erst bei Abfahrt in den Frachtbestand übertragen; Eigentum bleibt beim Spieler.
- Bei Ankunft im Spielerbestand am Ziel einlagern. Verkaufsauftrag ins dortige Orderbuch einstellen; Abnahme und Preis folgen vorhandenen Kaufgeboten und dem gesetzten Limit. Keine garantierten Abnehmer.
- Geplante Erlöse klar von sicheren Kosten unterscheiden.
- Lagerkosten und Lebensmittelverderb anhand expliziter Regeln und Einheiten ergänzen; Verderb als Verlust ausweisen.
- Spielerbestand bleibt bis zum Verkaufsauftrag außerhalb des öffentlichen Marktangebots; Zurückhalten ist eine reale Entscheidung.
- Keine Fahrsteuerung oder detailliertes Straßennetz implementieren. Routendarstellung und exemplarische Animation folgen in AP07.

**Abnahme**
- Kauf → Transport → Ankunft → Verkauf ist vollständig spielbar.
- Zu wenig Geld, Ware oder Kapazität verhindert den Auftrag nachvollziehbar.
- Fracht wird nie doppelt erzeugt oder doppelt zugestellt.
- Lieferungen funktionieren auch außerhalb des Kamerabereichs und bei Zeitbeschleunigung.
- Eigener Lkw ist gebunden und steht anschließend am Ziel; Rückfahrt benötigt einen eigenen Auftrag.
- Fremdauftrag bezahlt den Anbieter und bindet dessen Kapazität. Leeres Lager verursacht weiterhin Fixkosten.
- Fehlende Zielkapazität lehnt die Buchung ohne Teilreservierungen ab. Geplatzter Transport storniert keinen bereits ausgeführten Kauf.

### AP07 – Wirtschafts- und Transportlayer

**Voraussetzung:** AP06.

**Ziel:** Chancen und Engpässe werden direkt auf der Karte sichtbar.

**Aufgaben**
- Layer für Politik, Versorgung, Preise und Transport einführen.
- Warenauswahl, Legende, Einheiten und eindeutige Kennzeichnung fehlender Daten ergänzen.
- Wirtschaftswerte über Region-IDs mit der Kartendarstellung verbinden.
- Grobe Routen, Kapazitäten und Transportaktivität anzeigen. Begrenzte exemplarische Fahrzeuge auf Routen bewegen; nicht zwingend ein sichtbares Fahrzeug pro Auftrag.
- Animationsfortschritt aus veröffentlichten Simulationsdaten ableiten, zoomabhängig vereinfachen und bei Bedarf ausblenden. Keine Kollision, Fahrphysik oder NavMesh-Abhängigkeit.
- Für Vergleiche eine konsistente Farbskala verwenden.

**Abnahme**
- Ein Nahrungsengpass kann auf der Karte gefunden und über einen Handelsauftrag bedient werden.
- Layer stimmen mit Informationspanel und Simulationszustand überein.
- Layerwechsel verändert keine wirtschaftlichen Daten.
- Deaktivierte Routenanimation ändert keine Ankunft, Kosten oder Wirtschaftsergebnisse.

### AP07a – Klimarisiko, Vorräte und erster Interessenkonflikt

**Voraussetzung:** AP07. Neue Ergänzung; bestehende Paketnummern bleiben erhalten.

**Ziel:** Die besondere Identität des Spiels wird im Wirtschaftskern erlebbar.

**Aufgaben**
- Konfigurierbare regionale Dürre mit zeitlich begrenzter Auswirkung auf Nahrungsproduktion ergänzen.
- Prognose und tatsächliches Ereignis trennen: Prognosen nennen Risiko und Zeitraum, nicht den sicheren zukünftigen Zustand. Mit festen Seeds reproduzierbare Testfälle ermöglichen.
- Produktionseinbruch, Bestände, Marktpreise und unerfüllten Bedarf über bestehende Regeln verbinden; kein pauschaler Krisenpreis-Multiplikator.
- Versorgung, Lagerbestand, Verderb, Kosten und Gewinn im Panel oder einer kurzen Historie sichtbar machen.
- Die seit AP05 aktiven Produzenten, Händler und Konsumentengruppen auf veränderte Produktion und beobachtete Angebote reagieren lassen. AP08 vertieft ihre Planung; keine neue künstliche Krisennachfrage erzeugen.

**Abnahme**
- Zwei feste Testfälle zeigen eingetretene und ausgebliebene Dürre.
- Früher Einkauf verändert Angebot sofort; spätere Lieferung kann Mangel reduzieren.
- Zurückhalten kann Gewinnchancen und Versorgungsprobleme erhöhen, garantiert aber keinen Gewinn.
- Ausbleibende Krise kann durch Lagerkosten, Verderb und Preisentwicklung Verluste verursachen.
- Keine erfundenen Reputationsstrafen ohne implementiertes Reaktionssystem.

### AP08 – Strategien und Konkurrenzverhalten vertiefen

**Voraussetzung:** AP07a. Autonome Unternehmen bestehen bereits seit AP05.

**Ziel:** Bestehende Akteure planen vorausschauender und verfolgen unterschiedliche Interessen.

**Aufgaben**
- Einfache Regeln aus AP05 um mehrperiodige Handelsbewertung, Liquiditätsreserven und Bestandsziele erweitern.
- Unterschiedliche Parameterprofile für vorsichtige Versorgung, schnellen Warenumschlag und risikoreiche Spekulation ergänzen.
- Beobachtete Preise, transportierbare Mengen, Reisekosten, Verderb und Prognoseunsicherheit berücksichtigen.
- Konkurrenz um knappe Angebote und Kapazitäten über die bestehenden Marktregeln austragen.
- Entscheidungen periodisch statt pro Frame treffen und anhand beobachtbarer Informationen erklären.
- Investitionsvorhaben als spätere Erweiterung vorbereiten, aber Betriebsbau/Übernahmen nicht vor deren eigenen Paketen implementieren.

**Abnahme**
- Unterschiedliche Profile erzeugen in identischen Szenarien nachvollziehbar unterschiedliche Entscheidungen.
- Unternehmen können Verluste erleiden und bei fehlender Liquidität keine ungedeckten Aufträge finanzieren.
- Kein Sonderwissen über zukünftige Ereignisse und keine exklusiven Marktregeln für KI.
- Der Spieler ist zugleich möglicher Kunde, Lieferant und Konkurrent.
- Bestehende autonome Aktivität funktioniert weiterhin, wenn strategische Planung deaktiviert wird.

### AP09 – Speichern, Laden und spielbarer Prototyp

**Voraussetzung:** AP08.

**Ziel:** Eine zusammenhängende Spielsitzung kann fortgesetzt werden.

**Aufgaben**
- Versioniertes Speicherformat für Zeit, Konten, Eigentumsbestände, Betriebsstätten, Konsumentengruppen, Bedürfnisse/Versorgung, Orderbücher, Reservierungen, Preisverlauf, Unternehmensentscheidungen, aktive Transporte, Fahrzeugstandorte/-belegung, Lagerkapazitätsreservierungen, Fixkosten-/Lohnzeitpunkte, Zahlungsrückstände, Klimazustand und Zufallszustand ergänzen.
- Geschäftshistorie begrenzen oder archivieren; offene Aufträge und abrechnungsrelevante Zustände vollständig erhalten.
- Referenzen über stabile IDs speichern; Darstellung beim Laden rekonstruieren.
- Inkompatible oder beschädigte Spielstände verständlich abweisen.
- Einen paketierten Development-Build prüfen und Startanleitung vervollständigen.
- Kleine Messung der Simulations- und Darstellungszeiten durchführen; nur belegte Engpässe optimieren.

**Abnahme**
- Speichern während einer Lieferung und Laden stellt den korrekten Zustand wieder her.
- Derselbe Transport wird nach dem Laden genau einmal zugestellt.
- Offene und teilgefüllte Aufträge behalten korrekte Restmengen und Reservierungen; Laden bucht weder Löhne noch Handel oder Konsum doppelt.
- Paketierter Build enthält Geodaten, UI und Materialien und läuft ohne Editor sowie ohne Kartendienst.
- Offene Einschränkungen und Messergebnisse sind dokumentiert.

**Meilenstein B:** Spielbarer Wirtschaftskern mit realer Karte, akteursbasierten Märkten, regionalen Konsumenten, Lebensmittelhandel, Klimarisiko, Lagerentscheidungen und mehreren autonomen Unternehmen.

### AP10 – Skalierung messen und gezielt parallelisieren

**Voraussetzung:** AP09; Messinstrumentierung besteht seit AP04.

**Ziel:** Mehr Regionen und Unternehmen simulieren, ohne Wirtschaftsregeln oder UI-Konsistenz zu verändern.

**Aufgaben**
- Reproduzierbare synthetische Lastszenarien, z. B. 100 und 1.000 Regionen mit zugeordneten Betrieben und Unternehmen, ergänzen. Sie ersetzen keine realen Kartendaten und sind keine feste Leistungszusage.
- Referenzhardware, Build-Konfiguration, Datenmengen, Geschwindigkeiten und serielle Baseline dokumentieren.
- Unreal Insights nutzen, um den größten geeigneten Rechenengpass zu bestimmen.
- Diesen Bereich über begrenzte UE-Tasks parallelisieren; kleine Lasten bei Bedarf seriell ausführen.
- Zustandsbesitz, deterministische Zusammenführung, Snapshot-Übergabe, Rückstau und Job-Lebensdauer gemäß Architekturregeln implementieren und prüfen.
- Vorher/Nachher-Schrittzeiten, Frame-Zeiten, Speicherverbrauch und Parallelisierungs-Overhead vergleichen.

**Abnahme**
- Serielle und parallele Ausführung ergeben auf derselben Plattform bei gleichen Befehlen nach gleicher Schrittzahl dieselben fachlichen Ergebnisse.
- Kein Überverkauf bei gleichzeitig konkurrierenden Kaufaufträgen.
- Laden/PIE-Ende mit laufenden Tasks führt zu keinem Zugriff auf ungültige Objekte und keiner Übernahme alter Ergebnisse.
- Messdaten begründen den Nutzen; wirkungslose oder langsamere Parallelisierung wird nicht standardmäßig aktiviert.

## Technische Referenzen

Bei Umsetzung mit der installierten Engine-Version abgleichen:
- Epic: Tasks System – https://dev.epicgames.com/documentation/en-us/unreal-engine/tasks-systems-in-unreal-engine
- Epic: CPU-/Speicher-Performance – https://dev.epicgames.com/documentation/en-us/unreal-engine/common-memory-and-cpu-performance-considerations-in-unreal-engine
- Epic: Threaded Rendering / Thread-Zuständigkeit – https://dev.epicgames.com/documentation/en-us/unreal-engine/threaded-rendering-in-unreal-engine


## Spätere Erweiterungen – noch nicht implementieren

1. Zusätzliche Lager und Produktionsstätten kaufen, bauen, erweitern oder übernehmen. Ein eigenes Startlager und Eigentumsdaten existieren bereits.
2. Mehrstufige Lieferketten, Verträge und exklusive Lieferbeziehungen.
3. Flottenhandel, weitere Fahrzeugtypen, Eisenbahn/Schiff und Infrastrukturausbau. Eigener Start-Lkw und mietbare Fremdtransporte existieren bereits.
4. Bevölkerungswachstum, vertiefter Arbeitsmarkt, weitere Bedürfnisse und Substitution. Beschäftigung, Löhne, Grund- und Luxusbedarf bestehen bereits seit AP05.
5. Größere Unternehmenslandschaft, fortgeschrittene Strategien, Markteintritt und Insolvenz. Mehrere einfache Unternehmen bestehen bereits seit AP05.
6. Zölle, Handelsrechte, Politik, Konflikte und Unterbrechungen; abstrakte Drittstaaten-/Zwischenhändlergeschäfte mit Kosten, Entdeckungsrisiken und Reaktionen.
   Interessen und Reaktionen getrennt nach Regierung, Bevölkerung, Partnern und Investoren modellieren.
7. Mehrere Währungen und wirtschaftlich begründete Wechselkurse.
8. Weltweite Ausdehnung, weitere Detailstufen und Profiling größerer Datenmengen.
9. Visuelle Ausarbeitung: Relief, Wasser, Modelle und Kartenatmosphäre.

Diese Themen sollen auf dem gemeinsamen Datenmodell aufbauen. Sie sind keine Anforderung, bereits jetzt vollständige Systeme oder leere Klassen dafür anzulegen.

## Arbeitsregel für jeden einzelnen Auftrag

Wenn ich „Setze APxx um“ schreibe:
- Lies Masterprompt und das betreffende Paket.
- Prüfe die Voraussetzungen im tatsächlichen Projekt.
- Implementiere nur das Paket und notwendige Korrekturen an seinen Voraussetzungen.
- Halte Änderungen kompiliert und überprüfbar.
- Prüfe wirtschaftliche Invarianten mit gezielten Tests, Kartenbedienung visuell.
- Wenn Build oder Editor hier nicht verfügbar sind, benenne das ausdrücklich und gib konkrete lokale Prüfschritte an. Behaupte keine durchgeführten Prüfungen.
- Liefere knapp: umgesetzt, geprüft, Einschränkungen und nächstes Paket.
