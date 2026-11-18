# Conflict of Interest – Entwicklungsauftrag und Arbeitspakete

Stand: 18.11.2026. Alle bisherigen Pakete bis einschließlich AP07a sind laut Projektinhaber umgesetzt, einschließlich AP06a und AP07.1–07.3. Kein Neustart und kein erneuter Bau abgeschlossener Pakete. Nächster Auftrag: AP08.1. AP08.1–08.7 schaffen zuerst ein tragfähiges Aufbauspiel; danach folgen AP09 Ereignisse, AP10 vertiefte KI, AP11 Gesamtprüfung und AP12 Skalierung. Der Projektcode wurde für diese Planung nicht geprüft.

## Masterprompt für Codex

Entwickle ein Singleplayer-Strategie-, Aufbau- und Wirtschaftsspiel mit Unreal Engine 5 und C++. Prüfe zuerst das vorhandene Projekt, seine UE-Version und Projektregeln. Verwende die vorhandene Engine-Version und kompatible APIs. Setze die folgenden Arbeitspakete nacheinander um; prüfe den tatsächlichen Projektstand und setze nur das beauftragte Paket um; abgeschlossene Pakete bis AP07a nicht erneut bauen; ab AP08.1 vorhandene Funktionen gezielt erweitern. Nach jedem Paket liefere eine lauffähige, überprüfbare Zwischenversion, eine kurze Zusammenfassung, Prüfschritte und bekannte Einschränkungen. Implementiere spätere Pakete erst nach entsprechendem Auftrag.

### Spielvision

Der Spieler übernimmt vor Spielbeginn eines von mehreren vorgegebenen, bereits existierenden Unternehmen einer simulierten Weltwirtschaft auf der echten Erde. Ein kleines Handelshaus ist eine mögliche Startposition. Er organisiert Rohstoff- und Warenlieferungen, erschließt Märkte und kann später Produktionsstätten, Lager und Transportinfrastruktur bauen oder übernehmen. Ganze Lieferketten können wirtschaftlich kontrolliert werden.

Länder, Regionen und andere Unternehmen entwickeln sich unabhängig vom Eingreifen des Spielers, solange die Simulation läuft. Offline-Fortschritt bei geschlossenem Spiel ist zunächst nicht vorgesehen. Versorgung, Konsum, Produktion und Kaufkraft erzeugen Handelsmöglichkeiten. Langfristig beeinflussen Politik, Konflikte, Konkurrenz und Währungen die Wirtschaft.

Die Welt soll nachvollziehbar und spielerisch interessant sein. Reale Geografie ist Grundlage; die anfänglichen Wirtschaftsdaten sind ausdrücklich fiktiv.

### Identität: Conflict of Interest

Ziel ist der Aufbau eines Weltkonzerns durch Handel, Produktion, Logistik, Übernahmen und wirtschaftlichen Einfluss. Versorgung verbessern, Lieferketten kontrollieren oder Abhängigkeiten ausnutzen sind mögliche Strategien.

Das Spiel ermöglicht moralisch verwerfliches Verhalten: Waren zurückhalten, auf klimabedingte Knappheit spekulieren, Konkurrenz verdrängen und später Handelsbeschränkungen über Zwischenhändler oder Drittstaaten umgehen. Dies sind abstrakte Spielsysteme; konkrete reale Umgehungsanleitungen sind nicht Teil des Designs.

Konsequenzen entstehen aus Wirtschaftsregeln und reagierenden Akteuren. Kein globaler Moralwert: Versorgung, Vertrauen, Beziehungen, regulatorische Risiken und Unternehmensgewinn sind getrennte Größen. Spekulation birgt echte Verluste, weil Prognosen unsicher sind, Waren verderben und Lager Geld kosten. Für die ersten Pakete keine vollständigen Politik- oder Reputationssysteme vorwegnehmen.

### Unternehmenswahl und Steuerungszuordnung

Die Szenariowelt enthält alle Unternehmen vor der Spielerwahl. Der Spieler übernimmt eines davon; die übrigen handeln als NPCs. Die Auswahl erzeugt keine zusätzliche Firma, kein Geld und keine zusätzlichen Assets.

- Unternehmen sind wirtschaftliche Entitäten mit stabiler CompanyId. Ob sie vom Spieler oder der KI geführt werden, gehört zur Sitzungs-/Steuerungszuordnung, nicht zum Unternehmenstyp.
- Eine zentrale PlayerCompanyId identifiziert im Singleplayer genau ein existentes und als spielbar freigegebenes Unternehmen. Controller-Zuordnung zentral verwalten, nicht mehrfach als unabhängige Flags kopieren.
- Konsumentengruppen und externe Abrechnungskonten sind keine auswählbaren Unternehmen.
- Szenariodaten enthalten eine kleine Auswahl spielbarer Firmen mit Name, Beschreibung, Startort und Ausgangsbesitz. Vorläufige Beispiele: Handelshaus, Transportbetrieb, Produzent. Ein Unternehmen darf mehrere Tätigkeiten verbinden.
- Das bisherige Startunternehmen mit Lager, Lkw und Kapital bleibt als wählbares Handelshaus erhalten. Andere Startpositionen haben eigene Besitz- und Kapitalverhältnisse.
- Ein steuerbares Unternehmen verwendet Spielerbefehle; die übrigen verwenden KI-Entscheidungen. Alle nutzen dieselben fachlichen Regeln und Befehlsprüfungen.
- Nur autonome Unternehmensplanung des übernommenen Unternehmens deaktivieren. Produktion, Löhne, bereits gebuchte Transporte und explizit vom Spieler erteilte Automatisierungen laufen weiterhin.
- Befehle tragen CompanyId und ihre Herkunft. Die zentrale Befehlsverarbeitung prüft Steuerungsberechtigung sowie Eigentum, Bestände und Deckung; ein frei gesetztes Firmenfeld in einem Widget genügt nicht.
- HUD, Tabellen, Benachrichtigungen, Kartenbesitzfilter und Auftragsaktionen leiten „eigene“ Daten aus der PlayerCompanyId ab.
- Auswahl erfolgt vor Simulationsbeginn. Laufender Unternehmenswechsel ist zunächst nicht vorgesehen. Ungültige Auswahl verhindert den Start mit verständlicher Fehlermeldung; kein stiller Rückfall auf die erste Firma.
- Zunächst reicht Auswahl per Szenariokonfiguration. Ein späteres Startmenü liest dieselben Daten und ruft dieselbe Initialisierung auf.
- Speichern/Laden erhält PlayerCompanyId und erforderliche Controller-Zuordnung. Neue Partie aus demselben Szenario erzeugt unabhängig von der Auswahl denselben wirtschaftlichen Ausgangszustand; nur die Steuerung unterscheidet sich.

### Wirtschaftsmodell: Akteure, Konsumenten und regionale Märkte

**Verbindliche Grundlage ab AP04/AP05, keine spätere Erweiterung nach AP11.**

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

### Aufbaukern: Städte, Unternehmensassets und wirtschaftliches Reich

**Vorrang vor Ereignissen:** Zuerst muss eine normale Spielsequenz aus Handel, automatischem Betrieb, Gewinn und Investition funktionieren. AP09-Ereignisse sind keine Voraussetzung dafür.

- Das Unternehmen ist Eigentümer unterschiedlicher Assets und kann branchenübergreifend wachsen. Betriebsstätte und Unternehmen sind keine Synonyme.
- Städte sind gemeinsame Wirtschaftshubs. Mehrere Unternehmen können ihren Sitz oder Niederlassungen in derselben Stadt haben. Hauptsitz ist eine einzelne Ortsreferenz; weitere Standorte bleiben möglich.
- Jedes Asset ist einem Stadthub zugeordnet. Mine und Farm können räumlich im Umland liegen, werden aber über die Stadt verwaltet und benötigen dort zugeordnete geeignete Ressourcen.
- Alle lokalen Kauf-, Bau-, Ausbau- und Betriebsaktionen erfolgen über den Stadtbildschirm. Globale Ansichten bleiben als aggregierte Navigation erhalten und öffnen dieselben Stadt-/Assetaktionen, keine zweite konkurrierende Logik.
- Regionen behalten ihren Markt. Stadtansicht und Angebote filtern diesen nach Ort; keine zusätzlichen unabhängigen Stadtmärkte und kein kostenloser Transfer zwischen physischen Standorten.
- Spieler besitzen Assets, nicht Städte oder Regionen. Standardkarte zeigt eigene Standorte und Verbindungen in Firmenfarbe. Eigene und fremde Präsenz können sich überlagern.
- Horizontales Wachstum: weitere Assets, Städte, Absatzmärkte und Waren. Vertikales Wachstum: vorgelagerte/nachgelagerte Produktion, Lager und Transport kontrollieren.
- Bauen, Ausbau und Kauf einzelner Assets gehören zum ersten Aufbauspiel. Übernahme ganzer Unternehmen folgt später und umfasst Besitz sowie Verpflichtungen, nicht nur kostenlose Assets.
- Spieler bestimmen Vorgaben; Assets erledigen den Alltag selbstständig. Produkte nur innerhalb technisch zulässiger Rezepte des Betriebstyps wechseln, Ausbau, Lohnangebote, Einkaufs-/Verkaufsregeln und Lagerziele festlegen.
- Standardbeschaffung erfolgt am regionalen Markt mit physischem Lieferort. Feste interne oder externe Lieferbeziehungen können diese ergänzen; Doppelbeschaffung verhindern.
- Interne Lieferungen nutzen interne Verrechnungspreise für Standortergebnisse. Geld liegt weiterhin auf Unternehmensebene; konzernintern entstehen weder zusätzliches Bargeld noch externer Umsatz oder Konzerngewinn.
- Regionale Bedeutung wird durch getrennte, erklärbare Größen sichtbar: Warenabsatzanteil, Beschäftigtenanteil und Versorgung. Kein pauschaler Macht-/Moralwert. Später können diese Werte Grundlage vereinfachten Lobbyismus sein.

### Stadtbildschirm: zentrale lokale Arbeitsoberfläche

| Bereich | Inhalt |
|---|---|
| Übersicht | Stadt, Region, Bevölkerung/zugeordnete Konsumentengruppen, Beschäftigung, Schwerpunkte, aktive Firmen und eigene Präsenz |
| Markt | Bestehender Regionsmarkt mit Stadt-/Lieferortfilter, Angebote, Bedarf, Preise und Handels-/Transportaktionen |
| Eigene Assets | Lager, Produktion und Fahrzeuge vor Ort; Auslastung, Kosten, Ergebnis und Betriebsregeln |
| Investitionen | Bauoptionen, explizit angebotene Assets und Ausbauten; Kosten, Dauer, Voraussetzungen und Bestätigung |

Regionale Bevölkerungszahlen bei mehreren Städten nicht jeder Stadt vollständig zuschlagen. Eine nachvollziehbare Zuordnung oder ausdrücklich regionale Anzeige verwenden. Umlandvorkommen jeweils nur einmal einem physischen Gebiet/Standort zuordnen.

### Darstellung und Bedienung

Die Hauptoberfläche ist eine zusammenhängende, flache 3D-Strategiekarte mit später optionalem Höhenrelief. Orientierung für Zoom, Auswahl und Informationsdichte ist Crusader Kings 3; Gestaltung und Assets sind eigenständig.

Die Karte zeigt abhängig vom Zoom Länder, Regionen, Städte, Betriebe und Transportverbindungen. Informationspanels ergänzen die Karte. Es gibt zunächst keine separate begehbare Welt und keine Stadtbauansicht.

### Spieler-HUD und Arbeitsansichten

Ab AP06 ersetzt eine zusammenhängende Spieleroberfläche das bisherige Diagnosepanel als Hauptbedienung. AP07 ergänzt Kartenanalyse und Querverbindungen. Technische Diagnose bleibt separat einblendbar; Schrittzeiten, Snapshot-Dauer und interne IDs gehören nicht ins normale HUD.

**Gestaltung und Verhalten**
- Dezente Anlehnung an 1913: ruhige dunkle Flächen, warme helle Schrift, zurückhaltende Messingakzente und klare Tabellen. Lesbarkeit hat Vorrang vor Ornamenten; keine historische Schrift für dichte Zahlentabellen.
- Einheitliche Abstände, Typografie, Buttonzustände, Tooltips und Zahlenformate in wiederverwendbaren UMG-Komponenten.
- Zunächst 1280×720 und 1920×1080 mit DPI-Skalierung prüfen. Keine abgeschnittenen Bedienelemente; Tabellen scrollen innerhalb ihres Fensters, Zeilen besitzen klar beschriftete Spalten.
- Karte bleibt Hauptfläche. Eine große Arbeitsansicht gleichzeitig; Auswahlpanel kompakt bzw. einklappbar. Escape schließt zuerst Dialog, dann Arbeitsansicht.
- UI-Eingaben einschließlich Mausrad nicht an die Karte durchreichen. Texteingabe blockiert Kamera-Hotkeys. Dialoge pausieren die Simulation nicht automatisch; Datum und Geschwindigkeit bleiben sichtbar.
- Aufträge werden erst durch explizite Bestätigung gesendet. Vorschauen sind unverbindlich; Verfügbarkeit, Kosten und Deckung bei Ausführung erneut prüfen. Ablehnungen verständlich erklären, doppelte Bestätigung desselben Befehls verhindern.
- Daten stammen aus konsistenten veröffentlichten Zuständen, nicht aus separat gerechneter UI-Wirtschaft. Filter, Auswahl und Scrollposition bei Aktualisierung möglichst behalten.

| Oberfläche | Verbindlicher Inhalt |
|---|---|
| Obere Leiste | Firmenname, verfügbares und reserviertes Geld, Gesamtkontostand im Tooltip, Datum/Uhrzeit, Pause, vorhandene Zeitgeschwindigkeiten |
| Navigation | Märkte, Lager, Transporte, Unternehmen; aktive Ansicht deutlich markieren |
| Kartenauswahl | Name, Typ und Standort des gewählten Objekts, passende Kennzahlen und direkte Aktion zum zugehörigen Markt/Lager/Transport |
| Kartensteuerung ab AP07 | Layerwahl, Warenauswahl, Legende, Datenzeitraum und Datenstand |
| Meldungen | Lieferung angekommen, Auftrag abgelehnt, Lagerkapazität erschöpft, Liquidität knapp; mit Zeit, Kontext und Sprung zum Objekt |

Verfügbares Geld = Kontostand minus finanzielle Reservierungen. Reserviertes Geld nicht nochmals vom Kontostand abziehen. Bereits implementierte Geschwindigkeitsstufen beibehalten, sofern nicht ausdrücklich geändert.

**Arbeitsansichten ab AP06**
- Märkte: Region, Ware und physischer Angebotsort filtern. Kauf-/Verkaufsangebote mit Preislimit, Restmenge und Ort; letzte Abschlüsse mit Alter. Eigene Aufträge samt Reservierung, Restmenge und Storno anzeigen. Kauf/Verkauf mit Gesamtwertvorschau und verständlichen Grenzen.
- Lager: Standort, Eigentum/Mietstatus, belegte/freie/reservierte Zielkapazität, Fixkosten und Warenbestände. Gesamtmenge, Verkaufs-/Transportreservierungen und verfügbare Menge unterscheiden. Keine doppelte Reservierung zählen.
- Transporte: Planung, eigene Fahrzeugliste mit Standort/Belegung, buchbare Fremdangebote und laufende Aufträge. Start, Ziel, Fracht, Abfahrt, Ankunft, Status und Kosten sichtbar.
- Unternehmen: eigene Betriebe, Lager und Fahrzeuge sowie gebuchte Einnahmen, Ausgaben und Ergebnis des letzten abgeschlossenen Spieltags. Verfügbare Tage explizit nennen; noch fehlende Historie als solche ausweisen. Kein neues Bilanz-/Finanzierungssystem nur für das HUD erfinden.
- Meldungen bündeln wiederholte Probleme pro Objekt; maximale sichtbare Anzahl begrenzen. Ein begrenztes Meldungsprotokoll bleibt einsehbar. Warnschwellen in Konfiguration halten.
- Leere Listen, fehlende Marktgeschäfte, ungültige Auswahl und veraltete Vorschauen erhalten verständliche Zustände statt Nullwerten ohne Erklärung.

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
- Vordefiniertes spielbares Handelshaus an einem Ort, als vorläufiger Testwert Detroit: ein eigenes Lagerhaus, ein eigener Lkw und konfigurierbares Kapital. Es existiert auch dann, wenn der Spieler ein anderes Unternehmen übernimmt. Keine kostenlose unbegrenzte Lagerung oder Flotte.
- Mindestens zwei bereits wirtschaftlich initialisierte Unternehmen als spielbar freigeben, darunter das Handelshaus und ein weiterer Unternehmenstyp. Alle anderen Unternehmen bleiben NPCs; keine vollständigen separaten Startszenarien pro Firma bauen.
- Alle Mengen, Kosten, Kapazitäten und Startbestände liegen in Szenariodaten. Sie sind veränderbare Testwerte, keine historischen Behauptungen.
- Erster Wirtschaftskonflikt: Lebensmittelversorgung und Lagerhaltung bei unsicherem Dürre-Risiko. Ein freies Testszenario, keine vorgeschriebene moralische Handlungsfolge.

### Verbindlicher Datenmodell-Vertrag

Definitionen beschreiben Typen und Regeln; Laufzeitdaten beschreiben konkrete Entitäten und ihren Zustand. Kein Zwang zu bestimmten Klassennamen.

| Entität | Definition / Stammdaten | Laufzeitdaten / Beziehungen |
|---|---|---|
| Land / Region | ID, Name, Geometrie, Landeszugehörigkeit | Aggregierte Kennzahlen; Region besitzt genau einen Markt |
| Ort | ID, Name, Koordinaten, Region-ID | Lokale Betriebe, Lager und Fahrzeuge |
| Ware | ID, Mengeneinheit, Lagerbedarf je Einheit, Verderbsregel, Bedarfskategorie | Mengen ausschließlich in zugeordneten Beständen |
| Unternehmen | ID, Name, Entscheidungsprofil, spielbar und Auswahlbeschreibung | Konto, Besitz an Betrieben/Lagern/Fahrzeugen, Aufträge |
| Sitzung / Steuerung | Szenario-ID, Startauswahl | PlayerCompanyId und zentrale Controller-Zuordnung, getrennt von wirtschaftlichen Firmendaten |
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
- US-Startszenario mit vordefinierten Unternehmen und deren Besitz laden; das anfängliche Handelshaus besitzt Lager, Lkw und Kapital. Steuerungszuordnung gemäß AP06a anbinden; Länderwerte nur aus Regionen aggregieren.
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

### AP06 – Spieler-HUD, Arbeitsansichten und erste Handelslieferung

**Voraussetzung:** AP05.

**Ziel:** Ein bedienbares Spieler-HUD verbindet Märkte, Lager, Unternehmensbesitz und Transporte zu einem vollständigen Handelsablauf.

**Aufgaben**
- Spieler-HUD gemäß eigener Spezifikation bauen: obere Leiste, Navigation, Auswahlpanel, Meldungen und die vier Arbeitsansichten. Diagnosepanel separat schaltbar erhalten.
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
- Kauf → Transport → Ankunft → Verkauf ist vollständig über Spieleransichten bedienbar, ohne Entwicklerkonsole oder Debugbuttons.
- Verfügbares/reserviertes Geld, Waren und Kapazitäten stimmen mit dem Simulationszustand überein.
- Kernablauf bei 1280×720 und 1920×1080 visuell prüfen; keine verdeckten Schaltflächen oder Eingaben durch Fenster auf die Karte.
- Abschluss/Fehler eines Auftrags erzeugt eine verständliche Meldung mit Objektbezug.
- Zu wenig Geld, Ware oder Kapazität verhindert den Auftrag nachvollziehbar.
- Fracht wird nie doppelt erzeugt oder doppelt zugestellt.
- Lieferungen funktionieren auch außerhalb des Kamerabereichs und bei Zeitbeschleunigung.
- Eigener Lkw ist gebunden und steht anschließend am Ziel; Rückfahrt benötigt einen eigenen Auftrag.
- Fremdauftrag bezahlt den Anbieter und bindet dessen Kapazität. Leeres Lager verursacht weiterhin Fixkosten.
- Fehlende Zielkapazität lehnt die Buchung ohne Teilreservierungen ab. Geplatzter Transport storniert keinen bereits ausgeführten Kauf.

### AP06a – Bestehendes Unternehmen übernehmen statt Spielerfirma erzeugen

**Voraussetzung:** AP06. Vor AP07 umsetzen; bestehende Paketnummern bleiben erhalten.

**Ziel:** Der Spieler kann in einer neuen Partie ein vorgegebenes Unternehmen übernehmen, während alle anderen von der KI geführt werden. Zunächst Auswahl über Konfiguration.

**Aufgaben**
1. Bestehenden Code auf feste Spielerannahmen prüfen: erste Firma im Array, feste Firmen-/Asset-IDs, eigens erzeugte Spielerfirma, separate Spieler-Geldbestände, KI-Sonderregeln und fest verknüpfte Widgets.
2. Szenario zuerst vollständig initialisieren. Die Startauswahl benennt ausschließlich eine vorhandene spielbare CompanyId. Mindestens zwei geeignete Unternehmen zur Auswahl konfigurieren; Besitz, Geld und Standorte bleiben in deren regulären Szenariodatensätzen.
3. Zentrale PlayerCompanyId und eindeutige Steuerungszuordnung einführen. UI und KI fragen diese ab; wirtschaftliche Daten nicht duplizieren oder auf ein Spielerobjekt kopieren.
4. Befehlsberechtigung zentral erzwingen: Spieler darf ausschließlich im Namen des übernommenen Unternehmens handeln; KI darf keine autonomen Aufträge für dieses erzeugen. Allgemeine Eigentums- und Liquiditätsprüfungen bleiben unverändert.
5. KI-Auftragsgenerierung für die übernommene Firma deaktivieren, deren wirtschaftlichen Betrieb aber weiterlaufen lassen. Initialisierung und Befehlsqueue müssen verhindern, dass vor Spielbeginn alte autonome Aufträge der ausgewählten Firma eingespielt werden.
6. HUD, Markt-/Lager-/Transport-/Unternehmensansichten, verfügbare Aktionen, eigene Meldungen und Besitzmarkierungen auf den zentralen Kontext umstellen. Öffentlich sichtbare Fremddaten weiter gemäß Sichtbarkeitsregeln behandeln.
7. Ein initialer ungültiger oder nicht spielbarer Firmenschlüssel erzeugt einen Fehler ohne teilweise gestartete Simulation. Bei neuer Partie UI-Kontext und Meldungen sauber initialisieren.
8. Zuordnung als Bestandteil des zukünftigen Save-Zustands vorsehen. Falls Speichern bereits existiert, jetzt integrieren. Bei alten Spielständen Zuordnung nur übernehmen, wenn eindeutig rekonstruierbar; sonst verständlich abweisen, nicht raten.
9. Änderungen und Testauswahl dokumentieren. Noch kein Unternehmenswechsel während laufender Partie, kein umfassendes Auswahlmenü und kein erneuter Umbau korrekter Markt-/Transportregeln.

**Abnahme**
- Zwei neue Partien mit identischem Szenario/Seed, einmal Firma A und einmal Firma B, besitzen unmittelbar vor dem ersten Schritt identische wirtschaftliche Weltzustände. Nur Steuerungszuordnung und UI-Perspektive unterscheiden sich.
- In beiden Partien zeigt das HUD Geld, Lager, Fahrzeuge und Aufträge der tatsächlich ausgewählten Firma; ein Unternehmen ohne eigenen Lkw erhält keinen gratis.
- Auftrag für fremdes Unternehmen wird zentral abgewiesen, auch wenn die CompanyId außerhalb des Widgets verändert wird.
- Nicht gewählte Firmen erzeugen weiterhin KI-Aufträge. Die gewählte Firma erzeugt keine autonomen KI-Handelsentscheidungen, führt aber laufende Produktion, Löhne und bereits gebuchte Transporte aus.
- Ungültige/nicht spielbare Auswahl startet keine Partie und erzeugt keine Ersatzfirma.
- Keine zusätzlichen Konten, Waren oder Assets durch die Auswahl. Abgeschlossene Handels- und Transportabläufe aus AP06 funktionieren weiterhin.

**Späteres Auswahlmenü**
Eine Unternehmensliste/-karte zeigt Name, Tätigkeit, Startort, Kapital und wesentliche Assets. Auswahl markiert das Unternehmen und seinen Standort auf der Karte; „Partie starten“ bestätigt. Daten und Steuerungsinitialisierung sind dieselben wie in AP06a. Das Menü ist ein später gesondert zu beauftragender UI-Schritt, kein zusätzliches Wirtschaftssystem.

### AP07 – Wirtschafts- und Transportlayer mit HUD-Integration

**Voraussetzung:** AP06 und AP06a einschließlich Spieler-HUD und zentraler Unternehmenszuordnung.

**Ziel:** Versorgungslücken, Preisunterschiede und Transportaktivität lassen sich auf der Karte erkennen und direkt bearbeiten.

**AP07.1 – Kennzahlen und Kartenlayer**
- Vorhandene Länder-/Regionsansicht weiterverwenden; keine neue politische Simulation.
- Versorgung für gewählte Ware und Region im letzten abgeschlossenen Spieltag berechnen: Nahrung/Luxus = tatsächlich konsumierte Menge geteilt durch vor Budgetbegrenzung gewünschten Konsumbedarf; Produktionsinputs = tatsächlich eingesetzte Menge geteilt durch den vor Input-/Geldmangel geplanten Rezeptbedarf bei vorhandener Arbeitskraft/Kapazität.
- Haushalts- und Produktionsversorgung getrennt auswählbar machen, sofern dieselbe Ware beiden dient. Definition und Mengen im Tooltip erläutern.
- Bedarf null als „Kein Bedarf“ behandeln, nicht als 0 % oder 100 %. Noch kein vollständiger Spieltag = „Noch keine Tagesdaten“.
- Preis = mengengewichteter Durchschnitt tatsächlich ausgeführter Geschäfte des letzten abgeschlossenen Spieltags für Ware und Region. Bei mehreren physischen Orten Aggregation benennen; regionale Mittelwerte sind keine direkt garantierten Handelspreise.
- Aktuelle beste Kauf-/Verkaufsangebote und Mengen im Panel separat mit Ortsbezug anzeigen. Ohne Tagesumsatz „Keine aktuellen Geschäfte“; letzten älteren Abschluss nur mit Alter anzeigen.
- Versorgungsskala 0–100 %: Rot–Gelb–Grün mit festen Schwellen, zusätzlich Zahlen/Text. Preisgrenzen je Ware konfigurierbar und während des Vergleichs stabil; Werte außerhalb mit Endfarbe und tatsächlichem Zahlenwert zeigen.
- Fehlende Daten neutral grau, „Kein Bedarf“ davon durch Muster/Symbol/Text unterscheidbar. Farbe darf nicht die einzige Information sein.
- Layerwahl, Ware, Unteransicht, Einheit, Zeitfenster und Legende ins HUD aufnehmen. Datenstand und zugrunde liegenden abgeschlossenen Simulationsschritt anzeigen.
- Wirtschaftswerte über stabile Region-IDs zuordnen; Darstellung erst nach veröffentlichtem Simulationsschritt aktualisieren.

**AP07.2 – Routen und exemplarische Fahrzeuge**
- Grobe Routenkurven mit Richtung zwischen wirtschaftlichen Orten darstellen; keine detaillierte Straßenkarte, Kollision, Fahrphysik oder NavMesh.
- Filter „Eigene“ und „öffentlich sichtbare Aktivität“. Eigene Routen hervorheben. Für den Prototyp fremde Routenaktivität nur aggregiert und explizit als öffentliches Szenariowissen freigeben; keine geheimen Frachtmengen, Konten oder Einzelaufträge anzeigen.
- Eigene Routenauswahl zeigt Start/Ziel, aktive Lieferungen, eigene Frachtmenge und nächste Ankunft; Auftrag im Transportmenü öffnen können. Fremde Route zeigt nur freigegebene Aggregation.
- Wenige exemplarische Lkw auf sichtbaren aktiven Verbindungen. Anzahl pro Route und insgesamt konfigurierbar begrenzen, z. B. maximal 3 je Route und 100 insgesamt als Testwerte.
- Visualisierungen dürfen reale Auftragsfortschritte abbilden oder aggregierte Aktivität repräsentieren. Exemplarische Fahrzeuge sind nicht die eindeutige Positionsauskunft für einzelne Lieferungen.
- Beim Herauszoomen Fahrzeuge ausblenden und Routen bündeln/vereinfachen. Nahe Route muss trotz Kreuzung mit anderen Verbindungen gezielt auswählbar sein.
- Bewegung aus Simulationszeit und veröffentlichten Daten ableiten; Pause stoppt, Geschwindigkeit beeinflusst Bewegung. Keine Render-DeltaTime als unabhängige Transportuhr.
- Fahrzeuge sind Darstellungsobjekte ohne Waren-/Geldbesitz und ohne Zustellungslogik. Ausblenden oder Entfernen ändert keine Simulationsdaten.

**AP07.3 – Karte und Arbeitsansichten verbinden**
- Klick auf Region öffnet kompaktes Kennzahlenpanel; Aktion „Markt öffnen“ übernimmt Region und gewählte Ware in die Marktansicht.
- Aktion „Transport planen“ öffnet AP06 mit sinnvollen Start-/Zielfiltern aus Auswahl bzw. eigenem Lager. Ohne Quelle/Ziel nur fehlende Felder offenlassen, keinen Auftrag automatisch erzeugen.
- Auswahl aus Lager-/Transportliste kann die Karte auf das betreffende Objekt zentrieren. Karte und Listen nutzen dieselben IDs.
- Marktauswahl und Layerfilter beim Wechsel zwischen Karte und Arbeitsansicht erhalten.
- Berechnungen für Tageskennzahlen einmal pro benötigtem Zeitraum im Daten-/Simulationsbereich durchführen; UI nicht pro Frame durch die gesamte Welt oder unbegrenzte Geschäftshistorie iterieren lassen.
- Karte, Tabellen und Auswahlpanel auf denselben Datenstand beziehen; technische Diagnosen weiterhin separat.

**Abnahme**
- Kontrollfall: 80 kg konsumiert bei 100 kg gewünschtem Bedarf ergeben 80 %; Bedarf null und fehlende Historie sind eigenständige Zustände.
- Kontrollfall: 10 kg zu 2 USD/kg und 30 kg zu 4 USD/kg ergeben 3,50 USD/kg Tagespreis. Ohne Abschlüsse erscheint kein erfundener Preis.
- Auf der Karte lässt sich ein Nahrungsengpass erkennen, der zugehörige Markt öffnen und über AP06 eine Lieferung planen.
- Karte, Legende und Informationspanel zeigen dieselben Werte und Zeiträume. Layerwechsel verändert keine wirtschaftlichen Daten.
- Pause hält exemplarische Bewegung an; Zeitbeschleunigung, Kameraposition und deaktivierte Animation verändern keine Kosten oder Ankünfte.
- Sichtbarkeitsfilter offenbaren keine geschützten Konkurrentendaten.
- UI bei 1280×720 und 1920×1080 sowie mehreren Zoomstufen visuell prüfen. Layer, Legende und Auftragsdialoge bleiben bedienbar.
- Anzahl visueller Fahrzeuge bleibt innerhalb der Limits; Schritt-/Frame-Zeit mit aktivierter und deaktivierter Routenansicht vergleichen und dokumentieren.

### AP07a – Entwicklerkonsole und kontrollierte Simulationseingriffe

**Voraussetzung:** AP07 einschließlich bestätigter Steuerungszuordnung aus AP06a. Bereits vorhandene Diagnosefunktionen wiederverwenden.

**Ziel:** Zustände untersuchen, reproduzierbare Tests vorbereiten und später Ereignisse gezielt auslösen, ohne die Simulation durch unkontrollierte Eingriffe zu beschädigen.

**Umfang und Oberfläche**
- Separate, per konfigurierbarer Taste einblendbare Konsole mit Texteingabe, begrenztem Ausgabeprotokoll und Befehlshistorie. Hilfe und Parameterbeschreibungen anbieten.
- Nicht im normalen Spieler-HUD platzieren. Geöffnete Konsole fängt Tastatur/Maus-Eingaben ab; keine gleichzeitigen Kamera-/Handelsbefehle.
- Befehle nach Lesefunktionen und Eingriffen unterscheiden; Ergebnisse mit Zustandsschritt und stabilen Objekt-IDs anzeigen. Such-/Listenbefehle helfen beim Finden gültiger IDs.
- Nur Editor-/Development-Builds. Mutierende Entwickler-Endpunkte und UI in Shipping nicht registrieren; ausgeblendetes Widget allein reicht nicht. Keine allgemeine C++-/Shell-/Dateicode-Ausführung.
- Keine Netzwerk- oder Fernadministration. Rechte gelten nur für diese lokale Entwicklungsoberfläche und sind unabhängig vom gewählten Spielerunternehmen.

**Befehlsgruppen – Namen sind Vorschläge**
| Gruppe | Funktionen / Beispiel |
|---|---|
| Hilfe / Objekte finden | help; Unternehmen, Regionen, Lager, Fahrzeuge und Aufträge nach ID/Name auflisten |
| Zustand lesen | inspect company ID; inspect inventory ID; Geld, Besitz, Reservierungen, Aufträge und Herkunft erklären |
| Simulationssteuerung | pause; step 1; advance 24; positive ganzzahlige Stunden, nach Abschluss wieder pausiert |
| Testszenario herstellen | money set/add COMPANY CENTS; goods add OWNER STORAGE GOOD KG |
| Assets | vehicle relocate ID PLACE; nur freies Fahrzeug ohne gebundene Verpflichtungen |
| Validierung | validate world; referenzielle Integrität, Konten, Reservierungen, Warenmengen und Kapazitäten prüfen |
| Reproduzierbarkeit | Szenario-ID/-Version, Start-Seed, aktuellen Schritt und verwendete Konfiguration anzeigen |
| Ereignisse ab AP09 | event start drought REGION SEVERITY HOURS; event stop ID; aktive Ereignisse auflisten |

**Befehlsverarbeitung und Konsistenz**
1. Zentral registrierte, typisierte Befehle mit Prüfung von Parameteranzahl, Wertebereichen, gültigen IDs, Einheit und Zustand. Fehlermeldung nennt das Problem; fehlgeschlagene Mutation hinterlässt keine Teiländerung.
2. Mutationen gehen durch eine eigene Entwickler-Befehlsschnittstelle in die autoritative Simulation. Sie werden geordnet an einer abgeschlossenen Schrittgrenze ausgeführt, niemals direkt aus dem Widget an Actor-/Wirtschaftsdaten.
3. Bei Pause darf ein Eingriff an der aktuellen abgeschlossenen Grenze ausgeführt werden, ohne eine zusätzliche Wirtschaftsstunde vorzutäuschen. Snapshot-Version aktualisieren und UI konsistent informieren.
4. Laufende Jobs beachten: Befehl anfordern, sichere Grenze erreichen, validieren, ausführen, veröffentlichen. Keine gleichzeitige Snapshot-Mutation und kein blockierendes langes Warten im Game Thread.
5. step/advance verwenden dieselbe Stundenlogik wie normaler Betrieb; keine vergrößerten Zeitschritte oder übersprungenen Löhne/Transporte. Lange Vorschübe budgetiert über Frames abarbeiten, Fortschritt anzeigen und an Schrittgrenzen abbrechen können. Keine weiteren mutierenden Befehle während eines Vorschubs zulassen; Status/Abbruch bleiben möglich.
6. money set darf den Kontostand nicht unter bestehende Geldreservierungen setzen. Warenzugabe prüft Eigentümer, Lagerzugangsrecht und freie Kapazität; Fahrzeugversetzung lehnt gebundene Fahrzeuge ab. Keine stillschweigende Auflösung laufender Verträge.
7. Bewusste Testzugaben von Geld und Waren sind ausgewiesene Entwicklerquellen/-senken. Bilanzen unterscheiden normale Wirtschaftsflüsse und Eingriffe; kein anschließender falscher Bilanzalarm. Bestehende Waren werden nicht dupliziert.
8. Erfolgreiche und abgelehnte Eingriffe mit eindeutiger Befehlssequenz, Schritt, Parametern und Ergebnis protokollieren; bei erfolgreichen Mutationen relevante Vorher-/Nachherwerte erfassen. Maschinenlesbaren Logexport im Entwicklungsverzeichnis anbieten. Normales Spiel erzeugt keine Entwicklerzugaben.
9. Seed anzeigen; Änderung bei laufender Partie vorerst nicht erlauben. Reproduzierbare Neustarts über explizites Szenario/Seed durchführen. Das Log unterstützt Reproduktion, ist noch kein vollständiges automatisches Replay-System.
10. Keine pauschalen Cheats für Spielerfirmen: CompanyId explizit adressieren, sodass dieselben Tests für NPCs möglich sind. Normale Spielerberechtigungen bleiben unverändert; Konsolenprivilegien dürfen nicht in deren Befehlsweg gelangen.
11. Nach Eingriffen gezielte Invarianten prüfen; validate world verändert den Zustand nicht. Fehler mit betroffenen IDs melden, nicht automatisch unbekannte Zustände reparieren.
12. Ereignisregistry/-schnittstelle schlank vorbereiten, aber keine Dürrelogik vor AP09 implementieren. Unbekannte bzw. noch nicht implementierte Ereignisse klar ablehnen.

**Abnahme**
- Geld oder Ware für ein beliebiges existierendes Unternehmen gültig hinzufügen; Kontostand/Bestand und Snapshot aktualisieren sich mit einem dokumentierten Entwicklereingriff.
- Geld unter Reservierungen, Ware über Lagerkapazität, ungültige IDs, negative Zeit und Versetzen eines eingesetzten Lkw werden ohne Teiländerung abgelehnt.
- Im pausierten Spiel ändert inspect nichts. Eine gültige Mutation aktualisiert den Snapshot ohne automatische Produktion/Konsum; step 1 führt genau eine Wirtschaftsstunde aus.
- advance 24 ergibt auf derselben Plattform aus identischem Zustand/Seed/Befehlen dieselben fachlichen Ergebnisse wie 24 normale Einzelschritte. Abbruch erfolgt nur zwischen vollständigen Schritten.
- UI, Karte und offene Ansichten zeigen nach Eingriffen konsistente Zustände. Zustellung, Löhne und Reservierungsfreigabe erfolgen nicht doppelt.
- Start/Stop während laufender Worker-Jobs, PIE-Ende und erneuter Szenariostart hinterlassen keine alten Befehle, Jobs oder Objektzugriffe.
- Shipping-Build enthält keine registrierten mutierenden Konsolenbefehle und keinen alternativen Spielerzugang dazu; Test im Build dokumentieren.
- Ereignisbefehle vor AP09 melden „nicht implementiert“. Nach AP09 starten/beenden sie Ereignisse über dieselbe autoritative Schnittstelle.

## AP08 – Tragfähiges Unternehmens- und Aufbauspiel

Diese Pakete erweitern die vorhandene Wirtschaft. Keine zweite Simulation, keine Neuimplementierung korrekter Orderbücher, Konten, Transporte oder HUD-Komponenten. Vor jedem Paket vorhandene Schnittstellen prüfen. Testwerte extern konfigurieren; offene Balancingentscheidungen kennzeichnen.

### AP08.1 – Stadthubs und Asset-Zuordnung

**Voraussetzung:** AP07a; bisherige Pakete sind abgeschlossen.

**Aufgaben**
- CityId als Hub-Referenz für alle bestehenden Assets ergänzen; vorhandene Ort-/Region-IDs erhalten. Hauptsitz der Firma von Assetstandorten trennen.
- Betriebe, Lager, Fahrzeug-Heimatort und aktuellen Fahrzeugort eindeutig modellieren. Fahrzeuge unterwegs sind nicht doppelt an Start und Ziel gelistet.
- Minen/Farmen einem Stadthub und geeigneten Umlandressourcen zuordnen; Eignung, Reservierung und Nutzung begrenzen.
- Bestehendes Szenario migrieren: Besitz, Konten, Bestände, Aufträge und Transportziele erhalten. Keine Assets neu duplizieren.
- Entwicklerkonsole um inspect city/assets und Zuordnungsvalidierung erweitern.

**Abnahme**
- Eine Stadt enthält Assets mehrerer Firmen; eine Firma besitzt Assets mehrerer Städte.
- Jede Asset-Referenz ist gültig, jede Quelle nur einmal gezählt; kein territorialer Besitz wird behauptet.
- Vorher-/Nachher-Bilanzen der Datenmigration stimmen; laufende Transporte bleiben korrekt.

### AP08.2 – Stadtbildschirm und lokale Bedienung

**Voraussetzung:** AP08.1.

**Aufgaben**
- Stadtbildschirm mit Übersicht, Markt, eigenen Assets und Investitionen gemäß Spezifikation bauen.
- Bestehende Markt-/Transport-/Lageransichten kontextbezogen integrieren statt duplizieren.
- Ortsfilter sichtbar machen; Wechsel zum gesamten Regionsmarkt anbieten.
- Investitionsbereich vor AP08.3 nur mit echten Daten und klar gekennzeichnetem noch nicht verfügbarem Ausführungszustand anzeigen.
- Fremde Unternehmen mit freigegebenen Präsenzdaten zeigen; private Konten/Pläne nicht offenlegen.
- Stadt, Asset und globale Liste gegenseitig verlinken; Auswahl und Filter bewahren.

**Abnahme**
- Ein bestehender Kauf-/Transport-/Verkaufsablauf lässt sich von der Stadt aus bedienen.
- Eigene Assets sind sichtbar, fremde Assets nicht steuerbar. Hauptsitz ist von Niederlassungen unterscheidbar.
- 1280×720 und 1920×1080 visuell prüfen; keine verdeckten Aktionen oder Eingaben auf die Karte.

### AP08.3 – Assets bauen, kaufen und ausbauen

**Voraussetzung:** AP08.2.

**Aufgaben**
- Mindestens Lagerhaus und ein sinnvoll in die Versorgungskette eingebundener Produktionsbetriebstyp baubar machen.
- Bau-/Ausbaudefinitionen enthalten Kosten, Dauer, Kapazitätsänderung und Standortvoraussetzungen. Zunächst klar gebuchte Geldkosten an explizite Bauanbieter-/externe Konten; Materialbauwirtschaft kann später folgen.
- Bauplatz/Eignung und Zahlungsdeckung atomar reservieren. Unfertige Assets produzieren nicht; fertiggestellte genau einmal in Betrieb nehmen.
- Bestehende Assets nur kaufen, wenn ein explizites gültiges Verkaufsangebot vorliegt; Preis an Verkäufer zahlen. Kein beliebiger Zwangskauf fremden Besitzes.
- Für den ersten Kauf nur unbelastete angebotene Assets zulassen: keine fremden Waren, aktiven Transporte, Verkaufsreservierungen oder ungelösten Arbeits-/Lieferpflichten. Verkäufer muss diese zuvor geordnet abwickeln. Beschäftigte bei Abschluss freigeben und neue Stellen durch den neuen Besitzer besetzen lassen.
- Ausbau bestehender Assets: Kapazität erst nach Fertigstellung erhöhen, Effekte auf laufenden Betrieb vorher anzeigen. Bestandskapazität niemals unter Belegung verkleinern.
- Angebote von Startgeld, Waren und Jobs unabhängig gestalten; Kauf darf keinen Besitz und kein Geld duplizieren.

**Abnahme**
- Lager bauen, fertigstellen, nutzen und ausbauen ist spielbar; Geld- und Kapazitätsfolgen nachvollziehbar.
- Rohstoffbetrieb scheitert ohne passende Ressource. Ungedeckter Bau und belasteter Assetkauf werden ohne Teiländerung abgelehnt.
- Ein Assetkauf überträgt genau einen Besitz und den vereinbarten Betrag; keine Unternehmensübernahme erforderlich.

### AP08.4 – Speichern und Laden für Aufbautests

**Voraussetzung:** AP08.3.

**Aufgaben**
- Bisherige vollständige Speicheranforderungen aus AP11 schon jetzt für den vorhandenen Spielzustand implementieren.
- PlayerCompanyId, Seed/Zufallszustand, Zeit, Firmen, Konten, Bestände, Reservierungen, Orderbücher, Konsumenten, Beschäftigung, Assets und Transporte erhalten.
- Laufende Bau-/Ausbauprojekte, Zahlungen, Verkaufsangebote und Ressourcenbindungen speichern.
- An abgeschlossener Schrittgrenze speichern, Referenzen validieren, UI/Steuerung und Jobs sicher rekonstruieren; Save-Version und Szenario-/Definitionsversion dokumentieren.
- Entwicklereingriffe kennzeichnen. Beschädigte/inkompatible Zustände nachvollziehbar abweisen; keine geratenen Eigentümerzuordnungen.
- Spätere Pakete erweitern den Save-Vertrag jeweils mit ihren neuen Zuständen, nicht erst in AP11.

**Abnahme**
- Speichern während Bau, Teilausführung und Transport; Laden führt jede Fertigstellung/Zustellung/Zahlung genau einmal aus.
- Auf demselben System ergibt Fortsetzen die gleichen fachlichen Ergebnisse wie ununterbrochene Simulation.
- Eine Partie kann nach Programmneustart mit derselben Firma fortgeführt werden.

### AP08.5 – Betriebliche Vorgaben und automatischer Betrieb

**Voraussetzung:** AP08.4.

**Aufgaben**
- Assetdetailansichten für zulässiges Rezept, geplante Auslastung, Lohnangebot, Inputzielbestand und Einkaufs-/Verkaufslimits ergänzen.
- Lagerregeln: Mindestreserve, Zielbestand, zum Verkauf freigegebene Überschüsse und Kaufbudget.
- Bestehende Entscheidungslogik in einen gemeinsamen regelbasierten Betriebsdienst überführen. Er führt explizite Spielerregeln aus und dient auch NPCs; keine neue autonome strategische KI für die Spielerfirma einschalten.
- Befehle aktivieren/deaktivieren und Ausgaben deckeln. Geld-/Warenreservierungen gemeinschaftlich prüfen; Regeln dürfen nicht mehrfach dieselben Mengen bestellen.
- Änderungen ab definierter Schrittgrenze anwenden; offene Aufträge bei Regelwechsel geordnet behalten/stornieren, Politik dokumentieren.
- Fehlende Inputs, Arbeitskräfte, Kapital oder Lagerkapazität mit klarer Ursache anzeigen.
- Neue Regeln und laufende Automatisierungszustände speichern.

**Abnahme**
- Betrieb beschafft Inputs, produziert und verkauft innerhalb gesetzter Regeln ohne tägliche Einzelklicks.
- Niedrigeres Budget oder deaktivierte Regel erzeugt keine ungedeckten Folgeaufträge.
- Zwei eigene Betriebe können nicht dieselbe Liquidität mehrfach reservieren. Regeländerung und Save/Load behalten Konsistenz.

### AP08.6 – Lieferbeziehungen, wiederkehrende Transporte und interne Verrechnung

**Voraussetzung:** AP08.5.

**Aufgaben**
- Lieferregel mit Quelle/Ziel-Asset und Stadthubs, Ware, Menge je Intervall, Beginn/Ende, Preisbedingungen und Transportzuordnung implementieren.
- Interne Versorgung und externe Verträge trennen. Externer Vertrag braucht Zustimmung eines anderen Unternehmens nach einfacher nachvollziehbarer Annahmeregel.
- Zunächst feste konfigurierbare Preise und Menge/Intervall; keine Terminbörse oder komplexe Preisindexierung.
- Wiederkehrende eigene oder fremde Transporte mit Fahrzeugverfügbarkeit, Kosten und Zielkapazität über vorhandenen Dienst buchen. Leer-/Rückfahrten berücksichtigen.
- Liefermenge je Fälligkeit genau einmal erfassen; Ausfälle/Teillieferung sichtbar protokollieren. Erstes Modell ohne Geldstrafen: fehlende Menge nicht heimlich endlos nachholen; feste Storno-/Ablaufbedingungen anzeigen.
- Für dieselbe Zielmenge Lieferzusage und Marktbeschaffung gemeinsam berücksichtigen. Bestandslücken dürfen optional regelbasiert am Markt ausgeglichen werden, ohne doppelte Bestellung.
- Interne Transfers ändern Standortbestände über reale Transporte. Interne Bewertungsbuchungen belasten/begünstigen Standortberichte symmetrisch, nicht das Unternehmenskonto.
- Externe Erlöse, externe Kosten, interne Transfers und unrealisierten Lagerwert getrennt ausweisen. Internen Gewinn auf noch nicht extern verkaufter Ware nicht als Konzerngewinn darstellen.
- Verträge, nächste Fälligkeit, offene Liefermengen, Transportverknüpfungen und interne Bewertungsdaten speichern.

**Abnahme**
- Zwei eigene Assets über mehrere Intervalle versorgen, ohne doppelte Beschaffung oder kostenlose Ortswechsel.
- Externer Lieferpartner erhält Zahlungen; kein Konto wird ohne Deckung belastet.
- Interne Verrechnung verändert Standortergebnisse, aber nicht Konzernbargeld/externe Umsätze.
- Save/Load zwischen Fälligkeit und Ankunft erzeugt keine doppelte Lieferung. Engpass und Wiederaufnahme bleiben verständlich.

### AP08.7 – Sichtbares Konzernnetz und vollständiger Aufbauspieltest

**Voraussetzung:** AP08.6.

**Aufgaben**
- Eigene Stadthubs und Verbindungen als Netz in Firmenfarbe zeigen; Marker unterscheiden Hauptsitz, Niederlassung und Größenordnung eigener Präsenz.
- Keine Stadt-/Regionsbesitzfarbe als territoriale Herrschaft. Mehrere Firmen an einem Hub darstellen, Auswahl priorisieren und Informationen erklären.
- Marktführerschaft je Ware als eigenen Layer ergänzen: Anteil extern verkaufter Menge an Endabnehmer am Verbrauchs-/Betriebsort im letzten abgeschlossenen Spieltag; Zwischenhandel nicht mehrfach als Endabsatz zählen, interne Transfers ausschließen.
- Im Marktführerschaftslayer bei fehlendem Absatz neutral anzeigen, Gleichstand kennzeichnen. Anteil, Zeitraum und Nenner im Panel sichtbar; keine allgemeine Dominanz aus einer einzigen Ware ableiten.
- Beschäftigte, Standorte, wirtschaftliche Ergebnisse und Versorgung als separate Kennzahlen anzeigen. Spätere Politik kann diese verwenden; jetzt keine Lobbyismusmechanik.
- Reproduzierbares Startbalancing mit ausreichenden anfänglichen Handelsmöglichkeiten und Investitionsangeboten erstellen. Keine versteckten Geldzugaben.
- Zwei Wege testen: weiterer Lager-/Handelsstandort (horizontal) und eigene passende Produktionsstufe (vertikal).
- UX-/Wirtschaftsprobleme aus dem Test korrigieren, bevor Ereignisse beauftragt werden.

**Abnahme / Meilenstein B**
Eine fortsetzbare Partie ermöglicht: Unternehmen übernehmen → Versorgungslücke erkennen → wiederkehrende profitable Lieferung aufbauen → Geld verdienen → Asset bauen/kaufen → horizontal oder vertikal erweitern.
- Spieler erkennt Besitz, Kosten, Engpässe und Gewinnursachen in der Oberfläche ohne Konsole.
- Regelbetrieb funktioniert ohne erneute Bestellung jeder einzelnen Fahrt. Eingriffe konzentrieren sich auf Strategie, Ausbau und Ausnahmen.
- Beide Wachstumswege sind erreichbar und mit Kosten/Risiken verbunden; keine garantierte Profitmaschine.
- Mindestens ein dokumentierter Spieltest nennt Entscheidungsstellen, Wartephasen und verbleibende Probleme. Noch nicht bestandene Kriterien sperren den Übergang zu AP09 fachlich; kein bloßes Abhaken implementierter Widgets.

### AP09 – Klimarisiko, Vorräte und erster Interessenkonflikt

**Voraussetzung:** AP08.7 bestanden. Ereignisse nutzen die validierten Entwicklerbefehle aus AP07a; das Basisspiel muss ohne Ereignisse funktionieren.

**Ziel:** Die besondere Identität des Spiels wird im Wirtschaftskern erlebbar.

**Aufgaben**
- Konfigurierbare regionale Dürre mit zeitlich begrenzter Auswirkung auf Nahrungsproduktion ergänzen.
- Start-/Stop-/Listenbefehle in der Entwicklerkonsole aus AP07a registrieren. Stärke, Dauer, Region, Prognose und aktiven Zustand validieren; Stop entfernt nur den laufenden Effekt und macht bereits geschehene Wirtschaftsfolgen nicht rückgängig.
- Prognose und tatsächliches Ereignis trennen: Prognosen nennen Risiko und Zeitraum, nicht den sicheren zukünftigen Zustand. Mit festen Seeds reproduzierbare Testfälle ermöglichen.
- Produktionseinbruch, Bestände, Marktpreise und unerfüllten Bedarf über bestehende Regeln verbinden; kein pauschaler Krisenpreis-Multiplikator.
- Versorgung, Lagerbestand, Verderb, Kosten und Gewinn im Panel oder einer kurzen Historie sichtbar machen.
- Die seit AP05 aktiven Produzenten, Händler und Konsumentengruppen auf veränderte Produktion und beobachtete Angebote reagieren lassen. AP10 vertieft ihre Planung; keine neue künstliche Krisennachfrage erzeugen.

**Abnahme**
- Zwei feste Testfälle zeigen eingetretene und ausgebliebene Dürre.
- Konsole kann eine Dürre starten und stoppen; ungültige Parameter hinterlassen keinen Teilzustand. Geskriptete Tests und normale Ereignisauslösung verwenden dieselben Wirtschaftsregeln.
- Früher Einkauf verändert Angebot sofort; spätere Lieferung kann Mangel reduzieren.
- Zurückhalten kann Gewinnchancen und Versorgungsprobleme erhöhen, garantiert aber keinen Gewinn.
- Ausbleibende Krise kann durch Lagerkosten, Verderb und Preisentwicklung Verluste verursachen.
- Keine erfundenen Reputationsstrafen ohne implementiertes Reaktionssystem.

### AP10 – Strategien und Konkurrenzverhalten vertiefen

**Voraussetzung:** AP09. Autonome Unternehmen bestehen bereits seit AP05.

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

### AP11 – Paketierter Prototyp und Gesamtprüfung

**Voraussetzung:** AP10.

**Ziel:** Den aufgebauten Prototyp als Gesamtspiel verifizieren; bestehendes Speichern aus AP08.4 um alle späteren Zustände vervollständigen.

**Aufgaben**
- PlayerCompanyId, Szenario-ID und Controller-Zuordnung beim Laden validieren; KI-Zuordnung vor dem ersten weiteren Simulationsschritt wiederherstellen. Kein Rückfall auf eine feste Standardfirma.
- Das seit AP08.4 bestehende versionierte Speicherformat prüfen und ergänzen; nicht neu entwickeln. Vollständigkeit für Zeit, Konten, Eigentumsbestände, Betriebsstätten, Konsumentengruppen, Bedürfnisse/Versorgung, Orderbücher, Reservierungen, Preisverlauf, Unternehmensentscheidungen, aktive Transporte, Fahrzeugstandorte/-belegung, Lagerkapazitätsreservierungen, Fixkosten-/Lohnzeitpunkte, Zahlungsrückstände, Klimazustand, Kennzeichnung bisheriger Entwicklereingriffe und Zufallszustand ergänzen.
- Geschäftshistorie begrenzen oder archivieren; offene Aufträge und abrechnungsrelevante Zustände vollständig erhalten.
- Referenzen über stabile IDs speichern; Darstellung beim Laden rekonstruieren.
- Inkompatible oder beschädigte Spielstände verständlich abweisen.
- Einen paketierten Development-Build prüfen und Startanleitung vervollständigen.
- Kleine Messung der Simulations- und Darstellungszeiten durchführen; nur belegte Engpässe optimieren.

**Abnahme**
- Speichern während einer Lieferung und Laden stellt den korrekten Zustand wieder her.
- Laden bewahrt die übernommene Firma und deren HUD-Perspektive; keine gleichzeitige autonome KI-Steuerung dieser Firma.
- Derselbe Transport wird nach dem Laden genau einmal zugestellt.
- Offene und teilgefüllte Aufträge behalten korrekte Restmengen und Reservierungen; Laden bucht weder Löhne noch Handel oder Konsum doppelt.
- Paketierter Build enthält Geodaten, UI und Materialien und läuft ohne Editor sowie ohne Kartendienst.
- Offene Einschränkungen und Messergebnisse sind dokumentiert.

**Meilenstein C:** Paketierter, fortsetzbarer Wirtschaftskern mit realer Karte, akteursbasierten Märkten, regionalen Konsumenten, Lebensmittelhandel, Klimarisiko, Lagerentscheidungen und mehreren autonomen Unternehmen.

### AP12 – Skalierung messen und gezielt parallelisieren

**Voraussetzung:** AP11; Messinstrumentierung besteht seit AP04.

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

1. Ganze Unternehmen übernehmen, einschließlich Assets, Verträgen und Verpflichtungen. Bau, Assetkauf und Ausbau sind bereits AP08.3.
2. Komplexere Verträge, Exklusivität, Beteiligungen und weitere Produktionsketten. Grundlegende Lieferbeziehungen existieren seit AP08.6.
3. Zusätzliche Flotteninvestitionen, weitere Fahrzeugtypen, Eisenbahn/Schiff und Infrastrukturausbau. Eigener Start-Lkw und mietbare Fremdtransporte existieren bereits.
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
