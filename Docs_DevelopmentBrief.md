# Conflict of Interest – Entwicklungsauftrag und Arbeitspakete

Stand: 20.11.2026. AP01–AP08.6 sowie AP09.1–AP09.4 sind laut Projektinhaber umgesetzt. Der Code wurde hier nicht geprüft. Bankenplanung bleibt AP09.5; neue geografische Richtung siehe MAP01–MAP06. Kartenumstellung beginnt mit MAP01, ohne abgeschlossene Wirtschaftspakete neu zu bauen. AP09–AP16 sind ausgearbeitete Erstentwürfe zur späteren Verfeinerung durch den Projektinhaber. Keine erneute Implementierung abgeschlossener Pakete. Dieser Plan ersetzt die frühere Folge AP08.7/AP09–AP12.

## Inhaltsverzeichnis und Arbeitsstand

| Kapitel / Paket | Inhalt | Stand |
|---|---|---|
| 1 | Spielvision und Grundregeln | Verbindliche Leitlinien |
| 2 | Welt, Teilnehmer und gemeinsamer Wirtschaftszustand | Leitlinien und Modellvertrag |
| 3 | Bestehende Implementierung AP01–AP08.6 | Laut Projektinhaber umgesetzt |
| AP09.1 | Bestandsaufnahme und Erweiterungsvertrag | Umgesetzt |
| AP09.2 | Bevölkerung, Beschäftigung und Einkommen | Umgesetzt |
| AP09.3 | Geschäfte und Konsumentenentscheidungen | Umgesetzt |
| AP09.4 | Kostenrechnung und aufschlagsbasierte Preise | Umgesetzt |
| AP09.5 | Banken, Kredite und Geldmenge | Entwurf |
| AP09.5a | Öffentliche Einkommen und minimaler Staatshaushalt | Neu, vor Langzeittest |
| AP09.6 | Wirtschaftlicher Langzeittest | Entwurf |
| AP10 | Konzernnetz und vollständiger Aufbauspieltest | Entwurf; ersetzt AP08.7 |
| AP11.1 | Konzernstrategien und lokale Geschäftsführung | Entwurf |
| AP11.2 | Neugründungen und Markteintritt | Entwurf |
| AP11.3 | Geschäftsaufgabe und geordneter Marktaustritt | Entwurf |
| AP11.4 | Verhandelte Übernahme lokaler Unternehmen | Entwurf |
| AP12 | Staatliche Wirtschaft und Haushalt | Entwurf |
| AP13 | Nationale/internationale Politik und Lobbyismus | Entwurf |
| AP14 | Ereignisse und Krisen | Entwurf |
| AP15 | Gesamtprüfung und Auslieferung | Entwurf |
| AP16 | Skalierung und Parallelisierung | Entwurf |
| MAP01–MAP06 | Eigene Kartenplanung: Deutschland/Niederlande 1913, Verwaltungsregionen, Stadthubs und Wassertransport | Neu; vor geografisch abhängigen Erweiterungen |
| UI01–UI06 | Bestehendes UI-Kapitel; UI05 laut Projektinhaber umgesetzt, UI06 Gesamtprüfung offen | Vorhandene Funktionen bewahren |
| UI07–UI10 | Seitliche Panels, Handelsmenü, Kauf-/Transportablauf und Logistikübersicht | Neue Ergänzungen; danach UI06 abschließend prüfen |

**Leseregel:** Die allgemeinen Leitlinien und die neuen AP09–AP16 beschreiben das Zielmodell. AP01–AP08.6 bleiben als Implementierungshistorie erhalten. Ältere Aussagen darin zu direkten Haushaltskäufen, mehreren Städten pro Region, fixer Geldmenge oder noch fehlenden Funktionen sind kein Auftrag, die neue Zielarchitektur zu umgehen. AP09.1 erfasst die tatsächlichen Abweichungen im Code. Paketnummern außerhalb der historischen Abschnitte folgen ausschließlich diesem Inhaltsverzeichnis.

**Übergreifend:** Neue Zustände und Befehle erhalten im jeweiligen Paket Save-/Load-Unterstützung, Diagnose, relevante Invariantentests und Messpunkte. Keine Aufschiebung dieser Grundlagen bis AP15/AP16. Keine politische oder krisenbedingte Reparatur eines noch nicht funktionierenden Normalbetriebs.

## Masterprompt für Codex

Entwickle ein Singleplayer-Strategie-, Aufbau- und Wirtschaftsspiel mit Unreal Engine 5 und C++. Prüfe zuerst das vorhandene Projekt, seine UE-Version und Projektregeln. Verwende die vorhandene Engine-Version und kompatible APIs. Setze die folgenden Arbeitspakete nacheinander um; prüfe den tatsächlichen Projektstand und setze nur das beauftragte Paket um; abgeschlossene Pakete bis AP08.6 nicht erneut bauen; ab AP09.5 vorhandene Funktionen gezielt erweitern. Nach jedem Paket liefere eine lauffähige, überprüfbare Zwischenversion, eine kurze Zusammenfassung, Prüfschritte und bekannte Einschränkungen. Implementiere spätere Pakete erst nach entsprechendem Auftrag.

## 1. Spielvision und Grundregeln

### Spielvision

Der Spieler übernimmt vor Spielbeginn eines von mehreren vorgegebenen, bereits existierenden Unternehmen einer simulierten Weltwirtschaft auf der echten Erde. Ein kleines Handelshaus ist eine mögliche Startposition. Er organisiert Rohstoff- und Warenlieferungen, erschließt Märkte und kann später Produktionsstätten, Lager und Transportinfrastruktur bauen oder übernehmen. Ganze Lieferketten können wirtschaftlich kontrolliert werden.

Länder, Regionen und andere Unternehmen entwickeln sich unabhängig vom Eingreifen des Spielers, solange die Simulation läuft. Offline-Fortschritt bei geschlossenem Spiel ist zunächst nicht vorgesehen. Versorgung, Konsum, Produktion und Kaufkraft erzeugen Handelsmöglichkeiten. Langfristig beeinflussen Politik, Konflikte, Konkurrenz und Währungen die Wirtschaft.

Die Welt soll nachvollziehbar und spielerisch interessant sein. Reale Geografie ist Grundlage; die anfänglichen Wirtschaftsdaten sind ausdrücklich fiktiv.

### Identität: Conflict of Interest

Ziel ist der Aufbau eines Weltkonzerns durch Handel, Produktion, Logistik, Übernahmen und wirtschaftlichen Einfluss. Versorgung verbessern, Lieferketten kontrollieren oder Abhängigkeiten ausnutzen sind mögliche Strategien.

Das Spiel ermöglicht moralisch verwerfliches Verhalten: Waren zurückhalten, auf klimabedingte Knappheit spekulieren, Konkurrenz verdrängen und später Handelsbeschränkungen über Zwischenhändler oder Drittstaaten umgehen. Dies sind abstrakte Spielsysteme; konkrete reale Umgehungsanleitungen sind nicht Teil des Designs.

Konsequenzen entstehen aus Wirtschaftsregeln und reagierenden Akteuren. Kein globaler Moralwert: Versorgung, Vertrauen, Beziehungen, regulatorische Risiken und Unternehmensgewinn sind getrennte Größen. Spekulation birgt echte Verluste, weil Prognosen unsicher sind, Waren verderben und Lager Geld kosten. Für die ersten Pakete keine vollständigen Politik- oder Reputationssysteme vorwegnehmen.

## 2. Welt, Teilnehmer und Wirtschaftsmodell

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

**Grundlage seit AP04/AP05; neue Präzisierungen werden über AP09 integriert.**

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

**Vorrang vor Ereignissen:** Zuerst muss eine normale Spielsequenz aus Handel, automatischem Betrieb, Gewinn und Investition funktionieren. AP14-Ereignisse sind keine Voraussetzung dafür.

- Das Unternehmen ist Eigentümer unterschiedlicher Assets und kann branchenübergreifend wachsen. Betriebsstätte und Unternehmen sind keine Synonyme.
- Jede Wirtschaftsregion besitzt genau einen Stadthub; Städte sind gemeinsame Wirtschaftshubs. Mehrere Unternehmen können ihren Sitz oder Niederlassungen in derselben Stadt haben. Hauptsitz ist eine einzelne Ortsreferenz; weitere Standorte bleiben möglich.
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

Jede Region hat genau einen Stadthub. Regionale Bevölkerungszahlen genau einmal diesem Hub zuordnen. Umlandvorkommen jeweils nur einmal einem physischen Gebiet/Standort zuordnen.

### Verbindliche Präzisierungen der Wirtschaftsengine

Diese später beschlossenen Vorgaben haben Vorrang vor älteren Formulierungen in abgeschlossenen Paketen.

**Raum und Einzelhandel**
- Genau ein Stadthub je Wirtschaftsregion, genau eine Region je Hub. Stadt repräsentiert alle Haushalte und Assets ihres regionalen Einzugsgebiets. Region und Stadt bleiben für Geometrie und Bedienung referenzierbar, zählen Bevölkerung aber nur einmal.
- Der Regionsmarkt ist ausschließlich B2B: Unternehmen handeln mit Unternehmen. Haushalte stellen dort keine Kaufaufträge ein.
- Endkonsumenten kaufen ausschließlich aus realen Geschäftsbeständen. Produzierende Betriebe dürfen nicht an Haushalte verkaufen, auch nicht als Direktverkaufsoption. Derselbe Konzern darf Produktion und separate Einzelhandelsassets besitzen.
- Geschäftstypen: Lebensmittelgeschäft, Bekleidungsgeschäft und Gemischtwarenladen; Sortiment, Lager-/Verkaufsleistung, Personal und Fixkosten unterscheiden sich. Tatsächlich nicht vorhandene Warenketten nicht durch Gratisware ersetzen; Warenumfang in AP09.1 festlegen.
- Innerhalb eines Hubs sind Waren weiter an Lager-/Assetorte gebunden. Regionale letzte-Meile-Transfers benötigen eine explizite, einheitliche Zeit-/Kostenregel. Kein stiller Ortswechsel; AP09.1 definiert die Integration mit bestehender Logistik.

**Konsumenten und Wohlstand**
- Regionale Haushaltsgruppen besitzen reales Geld, Beschäftigung, Löhne und Ersparnisse. Keine endlosen Kaufbudgets.
- Grundbedürfnisse zuerst, danach Freizeitkonsum und Luxus. Innerhalb der Kategorien Angebote nach Budget, Preis, Verfügbarkeit, Qualität und später weiteren Attributen bewerten.
- Einfache gewichtete Verteilung auf mehrere Geschäfte statt zwingend alle Haushalte beim billigsten Laden. Exakte Formel und Parameter sind zunächst Entwurf und werden reproduzierbar dokumentiert.
- Landes-BIP ist eine aggregierte Wertschöpfungskennzahl, keine automatische Budgetquelle. Reale Kaufkraft und längerfristiger Gruppenwohlstand steuern zusätzliche Bedürfnisse. Die zuletzt vorgeschlagene Entkopplung von BIP und direkter Geldvergabe ist ein begründeter Entwurfswert, kein historischer Fakt.

**Preise und Ziele**
- Spieler stellt Kostenaufschlag ein, keinen beliebigen Festpreis: Vollkostenbasis × (1 + Aufschlag). Beispiel 10 USD Kosten + 20 % = 12 USD.
- Beschaffung bzw. eigene Herstellung, Transport und sachgerecht zugeteilte Betriebskosten automatisch berechnen und offenlegen. Interne Transfers nicht doppelt zählen. Frühere Festpreisoptionen in AP08.5 sind in AP09.4 gezielt zu migrieren.
- Unternehmen streben nachhaltigen Gewinn/Wachstum an und berücksichtigen Zahlungsfähigkeit. Haushalte sichern Grundbedarf und erhöhen Lebensstandard. Staaten verfolgen Versorgung, Beschäftigung, Haushalt und politische Unterstützung; BIP allein genügt nicht.
- Geldmenge darf durch explizite Kreditgeldschöpfung wachsen. Reales Vermögen, Bargeld-/Einlagenbestände, Schulden und Preisniveau getrennt messen. Keine unbegründeten Bilanzzuflüsse.

**Politischer Zielumfang**
Nationale Regeln und internationale Abkommen, Zölle und Beschränkungen werden später autonom überprüft. Unternehmen können Vorhaben anstoßen und auf andere Vorhaben reagieren. Lobbyismus beeinflusst einen nachvollziehbaren Entscheidungsprozess, kauft nicht garantiert ein Ergebnis. Zunächst kein vollständiges Wahl-/Diplomatiespiel.

### Entwicklungsregel: Spielstände und begrenzte Finanzkomplexität

- Während der Entwicklung keine Migration alter Spielstände auf neue inkompatible Versionen. Keine Konverter, Kompatibilitätsschichten oder heuristischen Reparaturen dafür bauen.
- Save-/Schema-/Definitionsversion prüfen; inkompatible Saves mit „Neue Partie erforderlich“ ablehnen. Neue Tests mit frischem Szenario starten. Speichern und Laden innerhalb derselben kompatiblen Version bleiben verpflichtend.
- Diese Regel hat Vorrang vor früheren Save-Migrationsformulierungen. Änderungen an Szenario-/Code-Daten bleiben möglich; das ist keine Verpflichtung, alte Partien zu retten.
- Finanzmechaniken dienen Finanzierung, Risiken und Geldkreislauf. Normale Einkäufe, Löhne, Lieferungen, Steuern und Zinsen werden automatisch verbucht; kein manuelles Überweisungs-Gameplay und keine Bankwahl pro Zahlung.
- Zunächst eine vereinfachte Bank. Kreditvergabe und Tilgung verändern die definierte Geldmenge; Zinsen, Bankausgaben, zurückbehaltene Gewinne und verfügbare Kaufkraft gesondert ausweisen.
- Keine pauschale Geldvernichtung zur Inflationskorrektur. Angebot, Nachfrage, Produktionskapazität, Kreditbedingungen und Rückflüsse messen; Geldmenge und Inflation nicht gleichsetzen.
- Länder setzen nationale Regeln. Öffentliche Beschäftigung und Transfers wirken regional über Gruppen und laufen zunächst aus demselben nationalen Haushalt. Keine separaten Regionalhaushalte oder einzelnen Behörden.
- Zusätzliche Konten und Verschleierung gehören zur späteren Finanz-/Politikebene. Manuelle Umbuchungen dann nur zwischen Konten desselben wirtschaftlichen Eigentümers; normale Zahlungen an andere Akteure bleiben vertrags-/geschäftsgebunden. Ein Kontowechsel hebt keine Steuerpflicht automatisch auf.

### Lokale Unternehmen und überregionale Konzerne

**Ziel:** Eine eigenständig lebendige Unternehmenslandschaft. Konzerne handeln mit lokalen Betrieben, beliefern sie, konkurrieren mit ihnen und können sie später übernehmen.

- Dieselbe Company-Entität, dieselben Konten, Asset-, Lohn-, Waren-, Markt- und Transportregeln für alle. Kein separates Gratisversorgungssystem.
- Ein Entscheidungsprofil unterscheidet lokale Existenzsicherung und begrenztes Wachstum von überregionaler Konzernexpansion. Das Profil ist keine zweite Wirtschaftsklasse und unabhängig von PlayerCompanyId.
- Pro Stadt ein konfigurierbarer Startmix lokaler Produzenten, Geschäfte und Dienstleister. Standortvoraussetzungen gelten: nicht jede Stadt besitzt jede Ressource oder jede Produktionskette.
- Einige Waren werden lokal erzeugt, andere importiert. Anfangsversorgung entsteht aus realen Beständen, Arbeitskräften, Lieferbeziehungen und Finanzierung, nicht aus garantierten kostenlosen Lieferungen.
- Kein starres Mindestfirmenkontingent während des Spiels: Betriebsausfälle sind möglich. Neue Unternehmen reagieren verzögert auf tragfähige Geschäftsmöglichkeiten.
- Kleine Firmen sind echte Kunden, Lieferanten und Übernahmeziele; können scheitern oder bei geeigneter Entwicklung selbst wachsen. Eine Beförderung zum Konzern darf keine Assets oder Geld erzeugen.
- Einfacher lokaler Betrieb ist bereits Bestandteil AP09.3–09.6. Neue Gründungs-, Austritts- und Übernahmeentscheidungen folgen AP11.2–11.4.
- Gründung benötigt eigene Mittel eines expliziten Eigentümer-/Investorenkontos und/oder einen regulären Kredit. Keine namenlose Geldquelle, kein gleichzeitig weiter verfügbares Investorengeld.
- Firmen haben einen Eigentümer/Verkäufer. Für lokale Startszenarien genügt ein explizites aggregiertes Unternehmerkonto mit endlichen Rücklagen; bis zur Umsetzung von Ausschüttungen dessen Guthaben nicht zugleich als Haushaltskonsumbudget zählen.
- Übernahmen brauchen Verkaufsbereitschaft und Preisverhandlung. Assets und Verpflichtungen verschwinden nicht; Mindestmodell und rechtliche Simulation sind bewusst vereinfacht.
- Anzahl lokaler Firmen, Prüfintervalle, Eintrittswartezeiten und Finanzierungsgrenzen extern konfigurieren. Grenzen dürfen nur Rechenlast steuern, keine Versorgung garantieren.

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

### Bisheriges Szenario: USA 1913 – durch MAP-Kapitel als aktive Vorgabe abgelöst

Die folgenden US-Angaben dokumentieren den bisherigen Teststand. Neue Karten-/Startszenariodaten folgen ausschließlich MAP01–MAP06 (Deutschland/Niederlande 1913).

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

## 3. Bestehende Implementierung – AP01 bis AP08.6

Die folgenden Pakete sind laut Projektinhaber umgesetzt. Sie dokumentieren den bisherigen Auftrag. Spätere Präzisierungen werden über AP09 integriert, nicht durch erneute vollständige Umsetzung dieser Pakete.

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
8. Zuordnung als Bestandteil des zukünftigen Save-Zustands vorsehen. Falls Speichern bereits existiert, jetzt integrieren. Alte inkompatible Spielstände während der Entwicklung abweisen, keine Zuordnung rekonstruieren und keine Migration bauen.
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
| Ereignisse ab AP14 | event start drought REGION SEVERITY HOURS; event stop ID; aktive Ereignisse auflisten |

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
12. Ereignisregistry/-schnittstelle schlank vorbereiten, aber keine Dürrelogik vor AP14 implementieren. Unbekannte bzw. noch nicht implementierte Ereignisse klar ablehnen.

**Abnahme**
- Geld oder Ware für ein beliebiges existierendes Unternehmen gültig hinzufügen; Kontostand/Bestand und Snapshot aktualisieren sich mit einem dokumentierten Entwicklereingriff.
- Geld unter Reservierungen, Ware über Lagerkapazität, ungültige IDs, negative Zeit und Versetzen eines eingesetzten Lkw werden ohne Teiländerung abgelehnt.
- Im pausierten Spiel ändert inspect nichts. Eine gültige Mutation aktualisiert den Snapshot ohne automatische Produktion/Konsum; step 1 führt genau eine Wirtschaftsstunde aus.
- advance 24 ergibt auf derselben Plattform aus identischem Zustand/Seed/Befehlen dieselben fachlichen Ergebnisse wie 24 normale Einzelschritte. Abbruch erfolgt nur zwischen vollständigen Schritten.
- UI, Karte und offene Ansichten zeigen nach Eingriffen konsistente Zustände. Zustellung, Löhne und Reservierungsfreigabe erfolgen nicht doppelt.
- Start/Stop während laufender Worker-Jobs, PIE-Ende und erneuter Szenariostart hinterlassen keine alten Befehle, Jobs oder Objektzugriffe.
- Shipping-Build enthält keine registrierten mutierenden Konsolenbefehle und keinen alternativen Spielerzugang dazu; Test im Build dokumentieren.
- Ereignisbefehle vor AP14 melden „nicht implementiert“. Nach AP14 starten/beenden sie Ereignisse über dieselbe autoritative Schnittstelle.

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
- Bisherige vollständige Speicheranforderungen aus der Gesamtprüfung (heute AP15) schon jetzt für den vorhandenen Spielzustand implementieren.
- PlayerCompanyId, Seed/Zufallszustand, Zeit, Firmen, Konten, Bestände, Reservierungen, Orderbücher, Konsumenten, Beschäftigung, Assets und Transporte erhalten.
- Laufende Bau-/Ausbauprojekte, Zahlungen, Verkaufsangebote und Ressourcenbindungen speichern.
- An abgeschlossener Schrittgrenze speichern, Referenzen validieren, UI/Steuerung und Jobs sicher rekonstruieren; Save-Version und Szenario-/Definitionsversion dokumentieren.
- Entwicklereingriffe kennzeichnen. Beschädigte/inkompatible Zustände nachvollziehbar abweisen; keine geratenen Eigentümerzuordnungen.
- Spätere Pakete erweitern den Save-Vertrag jeweils mit ihren neuen Zuständen, nicht erst in AP15.

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

## 4. Wirtschaftsengine vervollständigen – neue Arbeitspakete

**Entwurfsstatus:** Die folgenden Regeln sind konkrete Startvorschläge, damit Implementierungsaufträge prüfbar werden. Balancingwerte sind konfigurierbar. Nicht bestätigte Detailentscheidungen werden in AP09.1 sichtbar aufgelistet; sie sind keine versteckten Pflichtannahmen. Jedes Paket erweitert dieselbe Simulation.

### AP09.1 – Bestandsaufnahme und Erweiterungsvertrag

**Voraussetzung:** AP08.6.

**Ziel:** Tatsächlichen Code und neues Zielmodell verbinden, bevor weitere Mechaniken implementiert werden.

**Aufgaben**
- Vorhandene Entitäten, Geld-/Warenflüsse, Beschäftigung, Orderbücher, Automatisierung, interne Verrechnung, Transporte, Speichern und Entwicklerbefehle inventarisieren.
- Je System festhalten: vorhandene Klassen/Daten, erfüllte Regeln, Abweichungen und kleinster notwendiger Änderungsumfang.
- Mapping Region ↔ Stadthub auf 1:1 validieren. Physische Assetorte und interne Transporte beibehalten.
- Gemeinsames Phasenschema dokumentieren: fällige Zahlungen/Ankünfte, Beschäftigung/Einkommen, Planung, B2B-Handel, Produktion, Ladenbelieferung, Einzelhandel/Konsum, Abschlüsse. Genaue Reihenfolge gegen existierenden Code prüfen und verbindlich wählen.
- Konkrete Minimalwarenkette mit Grundbedarf und Freizeit/Luxus festlegen. Kleidung als neue Ware ergänzen, falls Bekleidungsgeschäft bereits im ersten Testszenario aktiv sein soll; anderenfalls diesen Typ zunächst nicht aktiv anbieten.
- Umstellungsplan für Code und frische Szenariodaten zu Haushaltskäufen, Preissteuerung und Konten erstellen. Alte inkompatible Saves werden verworfen, nicht migriert. Wirtschaftliche Modellregeln erhalten, Schemaänderungen versionieren und für Tests neue Partien starten.
- Lokale Firmen und Konzerne inventarisieren; gemeinsame Company-Struktur und Steuerungsprofile festlegen. Bestehende Akteure nicht automatisch duplizieren.
- Je Stadthub Startmix, Einfuhrbedarf, verfügbare Arbeitskräfte, Eigentümerkonten und Kapitalherkunft dokumentieren.
- Reproduzierbare Baseline über 30 Spieltage mit Seed, Datenstand, Geld-/Mengenbilanz und Messwerten erstellen.
- Eine dokumentierte Liste verbleibender Produktentscheidungen mit empfohlenen Startwerten liefern. Routineimplementierung nicht wegen jeder Zahl blockieren; grundlegende Widersprüche vor abhängigen Änderungen klären.

**Abnahme**
- Mappingtabelle Code → Zielregel, Phasenplan, Code-/Szenarioumstellungsplan und ausführbares Baselineszenario liegen vor.
- Keine parallele zweite Wirtschaftsengine und keine Neuimplementierung bereits korrekter Systeme.
- Konsumenten-, B2B- und Einzelhandelsflüsse haben klar getrennte Verantwortlichkeiten.

### AP09.2 – Bevölkerung, Beschäftigung und Einkommen

**Voraussetzung:** AP09.1.

**Ziel:** Haushalte verdienen, sparen und planen Konsum nachvollziehbar.

**Aufgaben**
- Bestehende regionale Gruppen zu drei konfigurierbaren Einkommens-/Bedürfnisprofilen erweitern, Erwerbspersonen von Gesamtbevölkerung unterscheiden.
- Beschäftigung an Produktions-, Handels-, Lager-, Transport- und Einzelhandelsassets berücksichtigen; keine mehrfach gezählten Vollzeitkräfte.
- Lohnperioden und Deckung prüfen. Arbeitgeber zahlen an Gruppen; Einkommensverteilung innerhalb aggregierter Gruppen dokumentieren.
- Arbeitslosigkeit und Ersparnisse abbilden; keine unsichtbare Auffüllung. Übergangsweise externe Transfers nur mit expliziter Herkunft.
- Tagesbedarf vor Budgetbegrenzung berechnen; Budgetpriorität Grundbedarf → Freizeit → Luxus; Reserve-/Sparziel als konfigurierbarer Entwurf.
- Gruppenwohlstand aus gleitender realer Kaufkraft und erfüllten Bedürfnissen ableiten; Wachstum zusätzlicher Bedürfnisse begrenzen und verzögern. Kein sofortiger Bedarfsprung bei einer einmaligen Geldzugabe.
- UI zeigt Einkommen, Konsumausgaben, Ersparnisse, Arbeitslosigkeit und Fehlbedarf je Gruppe; personenbezogene Einzelsimulation nicht erforderlich.
- Bestehenden Kaufweg bis AP09.3 nicht unbeaufsichtigt weiterlaufen lassen: klarer Übergangsmodus, keine doppelten Käufe nach Aktivierung des Einzelhandels.

**Abnahme**
- Lohnzahlung reduziert Arbeitgeberkonto und erhöht Haushaltskonto um denselben Betrag.
- Arbeitsplatzverlust reduziert zukünftiges Lohneinkommen; vorhandene Ersparnisse bleiben.
- Bedarf ist bei fehlendem Geld vorhanden, aber nicht automatisch gekauft.
- Ein BIP-/Statistikwert erzeugt kein Geld. Save/Load erhält Einkommenstermine und Gruppenstatus ohne Doppelzahlung.

### AP09.3 – Geschäfte und Konsumentenentscheidungen

**Voraussetzung:** AP09.2.

**Ziel:** Einzelhandel ist der einzige Warenzugang der Haushalte.

**Aufgaben**
- Einzelhandelsassets mit Typ, Sortiment, Verkaufsbestand, Personal, Lager-/Verkaufsleistung und Fixkosten integrieren. Bau/Kauf/Ausbau nutzt AP08.3 und Stadtbildschirm.
- Für das aktive Sortiment passende Produzenten und Händler konfigurieren. Keine unbegrenzten Großhandels- oder Ladenbestände.
- Beschaffung über B2B-Aufträge bzw. bestehende Lieferverträge, physischer Transport in Ladenbestand; dieselbe Ware nicht gleichzeitig im Lager und Laden führen.
- Haushaltskäufe aus dem B2B-Orderbuch entfernen. Produzenten an Haushalte zentral ablehnen; Besitz eines Ladens ist eine separate Voraussetzung.
- Tagesbudget nach Prioritätsstufen auf tatsächlich verfügbare Ladenangebote verteilen. Preis, Qualität, Verfügbarkeit und zugängliches Sortiment verwenden.
- Entscheidung deterministisch und nachvollziehbar verteilen, z. B. gewichtete Nachfrage mit erneuter Verteilung bei ausverkauften Angeboten. Keine Kaufwahrscheinlichkeit, die Preis-/Budgetlimits umgeht.
- Pro Kauf Geld und Ware atomar buchen, anschließend Konsum als Warensenke erfassen. Nicht erfüllte Menge bleibt Fehlbedarf, keine Schuld ohne eigenen Kreditvertrag.
- Für Migration einmalig sichtbare Kosten-/Preisparameter verwenden; AP09.4 ersetzt provisorische Kalkulation durch vollständige Kostenbasis.
- Spielbare Firma kann ein Geschäft besitzen oder beliefern. Alle fremden Geschäfte laufen unter denselben Regeln.
- Je Stadthub geeignete lokale Ladenbetreiber und lokale/überregionale Lieferanten als reale Unternehmen konfigurieren; Konkurrenz dort ermöglichen, wo Nachfrage und Kapital mehrere Anbieter tragen. Kein unendlicher Ladenbestand oder Garantieabsatz.
- Stadtübersicht listet lokale Firmen, Tätigkeiten und öffentliche Besitzdaten; Markt-/Lieferaktionen verwenden dieselben CompanyIds.

**Abnahme**
- Konsum ohne gefüllte Geschäfte bleibt unerfüllt, auch wenn ein Produzent Vorräte hat.
- Zwei Geschäfte konkurrieren; Preis-/Qualitätsänderungen verändern Absatz bei sonst gleichem Zustand nachvollziehbar.
- Grundbedarf erhält Budgetvorrang, Luxus bleibt bei Geldmangel aus.
- Käufer zahlen Ladenbesitzer, Ladenbestand sinkt, Verbrauch wird einmal gezählt.
- Bereits laufende B2B-/Lieferaufträge bleiben bei Migration konsistent; Kauf und Save/Load duplizieren nichts.

### AP09.4 – Kostenrechnung und aufschlagsbasierte Preise

**Voraussetzung:** AP09.3.

**Ziel:** Spieler steuert Aufschläge, die Engine erklärt Kosten und Ergebnis.

**Aufgaben**
- Einheitlichen Kostenfluss für Einkauf, interne Produktion, Transport und Betrieb implementieren; vorhandene interne Verrechnung integrieren.
- Lagerkosten zunächst mit gleitenden gewichteten Anschaffungskosten bewerten; Herstellung enthält verbrauchte Inputs und zurechenbare Lohn-/Betriebskosten.
- Zurechnung fixer Kosten über konfigurierbare normale Auslastung bzw. nachvollziehbaren Mengenbezug; Absatz null darf keine Division durch null oder endlose Preise erzeugen. Anlaufwerte und unzugeordnete Leerkosten separat zeigen.
- Keine Aufschläge auf automatisch sich selbst rückkoppelnde Kosten. Preisberechnung zu festen Zeitpunkten mit versionierter Basis, nicht pro Kauf rekursiv.
- Preis = Kostenbasis × (1 + Aufschlag); Geldrundung dokumentieren. Spieler setzt nur Aufschlag, nicht willkürlichen Festpreis.
- Bestehende Verträge mit vereinbarten Preisen zunächst erfüllen; neue Markt-/Ladenangebote nach neuer Kalkulation erstellen. Altaufträge geordnet auslaufen lassen, keine rückwirkende Preisänderung.
- Konzerninterne Bewertungsaufschläge bei Konzernabschluss eliminieren; externen Gewinn und interne Standortleistung unterscheiden.
- UI zeigt Kostenaufteilung, Aufschlag, Preis, realisierte Erlöse, Wareneinsatz und Periodenergebnis. Margenänderung ist kein Gewinnversprechen.

**Abnahme**
- Kostenbasis 10 USD, Aufschlag 20 % ergibt 12 USD. Höhere Transportkosten erhöhen nachvollziehbar die Basis.
- Interner Transfer erschafft weder Bargeld noch externen Konzernumsatz.
- Nullabsatz, Neubetrieb und fehlende Einkaufshistorie liefern definierte, sichtbare Werte.
- Tatsächlicher Periodengewinn bleibt nach externen Kosten und Leerkosten korrekt, unabhängig von kalkulatorischer Preisbasis.

### AP09.5 – Vereinfachte Banken, Finanzierung und Geldkreislauf

**Voraussetzung:** AP09.4 umgesetzt. Kostenrechnung erhalten; keine erneute Implementierung.

**Ziel:** Begrenzte Investitionsfinanzierung mit erklärbarer Geldschöpfung und Tilgung, ohne Zahlungsverkehr als zusätzliche Spieleraufgabe.

**Aufgaben**
- Eine Bankbilanz an bestehende Konten anbinden. Kein zweites Geldkonto neben vorhandener Firmenliquidität, keine Bankfilialen, kein Interbankenmarkt und keine einzelnen Haushaltsbankkonten.
- Frisches Startszenario mit konsistenter Eröffnungsbilanz konfigurieren: vorhandene Guthaben erhalten genau einen expliziten Gegenposten. Alte Saves nicht konvertieren.
- Geldaggregate fest definieren: kaufkräftige Einlagen der Nichtbankakteure, Kreditforderungen, Schulden, Bankeigenkapital/Ertrag und reale Assets getrennt erfassen. Bankgegenposten nicht als zusätzliches verfügbares Geld zählen.
- Gemeinsame validierte automatische Zahlungsbuchung für Wirtschaftsvorgänge verwenden. Periodische Verpflichtungen haben Quelle, Empfänger, Zweck, Betrag/Bemessungsregel, Fälligkeit und Status; vorhandene Zahlungslogik erweitern, kein neues universelles Framework erzwingen.
- Kreditantrag mit Betrag/Laufzeit. Bank bewertet Ergebnis-/Cashflowhistorie, bestehende Schulden, Sicherheiten und Risikobudget. Fehlende Historie hat explizite Startregeln. Kein automatisches Rettungsgeld.
- Kredit schafft gleichzeitig Kundenguthaben und Bankforderung. Tilgung reduziert beide. Zinsen separat buchen und nicht wie Tilgung behandeln.
- Einheitlicher konfigurierbarer Basiszins plus begründeter Risikoaufschlag. Keine automatische Inflationssteuerung oder Zentralbank-KI in diesem Paket.
- Bank mit kleinem aggregiertem Personal-/Betriebsbudget führen. Ein konfigurierbarer Teil verfügbarer Erträge finanziert gedeckte Banklöhne und Ausgaben an reale Haushalts-/Dienstleisterkonten; verbleibender Gewinn wird zurückbehalten.
- Bankausgaben anhand konsistenter Bilanzbuchungen behandeln: im Einbankenmodell kann Zinszahlung Einlagen reduzieren und Bankertrag erhöhen, Bankausgabe Einlagen wieder erhöhen und Bankertrag/Eigenkapital mindern. Diese Bewegungen separat ausweisen, nicht nebenher ein Bank-Girokonto mit demselben Geld führen.
- Zinsen/Bankrücklagen sind nicht gleich Kapitaltilgung. Zurückbehaltener Gewinn kann Kaufkraft binden, garantiert aber keine stabile Inflation; keine feste Löschquote ergänzen.
- Bankbeschäftigte nutzen vorhandenen regionalen Arbeitskräftepool, keine doppelte Beschäftigung. Kosten und Lohnperioden dokumentieren.
- Rückstände, nächste Fälligkeit, Schuldendienst, Sicherheiten und Risiko speichern. Ungedeckte Zahlungen bleiben offen; keine stillen Negativkonten oder Verlängerung ohne Entscheidung.
- Forderungsausfall als gesonderte Abschreibung, keine automatische Vernichtung fremder Kundenguthaben. Vollständige Insolvenz folgt bestehenden späteren Paketen.
- Unternehmens-/Investitionsansicht zeigt Kredit, Zins, Tilgung, Restschuld und künftige Belastung. NPCs nutzen dieselben Regeln. Keine neue manuelle Überweisungsansicht.

**Abnahme**
- Kredit 1.000: Kundeneinlagen und Forderungen steigen um 1.000. Tilgung 100: beide sinken um 100.
- Zinszahlung und spätere Bankausgabe sind in der gewählten Bilanzkonvention nachvollziehbar und nicht doppelt gezählt.
- Normaler Firmenkauf/Haushaltskonsum überträgt Kaufkraft, schafft keinen weiteren Kredit.
- Einbehaltene Erträge und Bankausgaben sind getrennt sichtbar; niemand erhält unfinanzierte Gratislöhne.
- Save/Load derselben Version dupliziert keine Fälligkeiten. Inkompatible alte Saves werden abgewiesen, nicht migriert.
- Kreditfinanzierte Kapazitätserweiterung ist möglich, schlechte Investitionen bleiben verlustfähig.

### AP09.5a – Öffentliche Einkommen und minimaler Staatshaushalt

**Voraussetzung:** AP09.5. Vor AP09.6 umsetzen; keine vollständige Politik vorziehen.

**Ziel:** Öffentliche Löhne und begrenzte Transfers ergänzen den Wirtschaftskreislauf ohne unbegrenzte Budgetauffüllung.

**Aufgaben**
- Ein nationales Haushaltskonto mit Startreserve; pro Region ein aggregierter öffentlicher Arbeitgeber als Ausgabenstelle desselben Haushalts. Keine neue politische KI und keine separaten Regionalbudgets.
- Beschäftigte öffentlicher Dienste wie Verwaltung/Schulen abstrahieren. Begrenzte Stellen, Lohnangebot und reale Arbeitskräfte aus vorhandenen Gruppen; Konkurrenz mit privaten Arbeitgebern und keine Doppelzählung.
- Öffentliche Löhne über die gemeinsame Zahlungsbuchung an Haushaltsgruppen zahlen. Öffentliche Dienste zunächst abstrakte Leistung, keine fiktive Warenproduktion oder Ladenumsätze.
- Eine einfache konfigurierbare, gedeckelte Unterstützung für definierte berechtigte Gruppen. Anspruch, Betrag, Bezugsdauer und Priorität dokumentieren. Für 1913 als explizite Szenarioregel, nicht automatisch als historisch belegtes modernes Sozialsystem darstellen.
- Einen minimalen festen nationalen Einnahmenmechanismus, etwa eine konfigurierte Abgabe auf vorhandene positive steuerliche Firmenergebnisse, einführen. Bemessung und Fälligkeit offenlegen; AP12 erweitert dieselbe Mechanik.
- Einzahlungen, Löhne, Transfers und Schuldendienst getrennt im Haushalt ausweisen. Steuern sind Geldübertragung, kein zusätzlicher Geldschöpfungsakt.
- Finanzierung aus Steuern/Reserve, optional explizit begrenzter Staatskredit über dieselbe Bank. Ohne Deckung Ausgaben nach dokumentierter Priorität begrenzen oder Rückstände führen; niemals Konsumentenbudgets automatisch passend zur Nachfrage auffüllen.
- Keine automatische Vollbeschäftigung oder Grundversorgung. Haushalte können trotz Transfers unterversorgt bleiben.
- Konsumentenübersicht trennt private/öffentliche Löhne und Transfers. Entwickleransicht zeigt Haushaltsquelle und Wirkung; Spieler steuert den Staat hier noch nicht.
- Öffentliche Arbeitgeber, Ansprüche, Zahlungen und Haushaltsregeln in aktuellem Saveformat speichern, keine Altspielstandmigration.

**Abnahme**
- Öffentlicher Lohn vermindert Staatskonto und erhöht Gruppenguthaben exakt.
- Dieselbe Personengruppe wird nicht gleichzeitig voll privat und öffentlich beschäftigt.
- Transfer erfüllt nur konfigurierte Ansprüche und bleibt budgetbegrenzt.
- Leerer Haushalt ohne Kreditspielraum erzeugt nachvollziehbare Kürzung/Rückstand, keine versteckte Geldquelle.
- Mit und ohne öffentliche Einkommen können frische Vergleichsszenarien ohne Änderung der privaten Wirtschaftsregeln getestet werden.

### AP09.6 – Wirtschaftlicher Langzeittest

**Voraussetzung:** AP09.5a.

**Ziel:** Ein geschlossener, erklärbarer Normalbetrieb vor Konzernspiel, Politik und Krisen.

**Aufgaben**
- Testfälle über 30, 180 und 365 Spieltage mit Datenstand/Seed, Firmen, Haushalten, Geschäften, Logistik, Bank und minimalem Staatshaushalt konfigurieren.
- Geldbestände und -schöpfung, Tilgung, Bankzinserträge/-ausgaben, Warenquellen/-senken, private/öffentliche Beschäftigung, Transfers, Steuern, Preise, Reservierungen, Gewinne, Schulden und Konsum täglich protokollieren.
- Vergleichsfälle mit starkem Kreditwachstum, viel Tilgung, zurückbehaltenen Bankerträgen und knappem Warenangebot auswerten. Geldmenge, Umlauf/Kaufkraft, Preisentwicklung und reale Produktionsmenge getrennt betrachten.
- Normalszenario mit lokalen Firmen und Konzernen, Inputengpass, Konkurrenz, hohe Aufschläge und überschuldete Firma vergleichen. Ereignissystem nicht erforderlich; Anfangsbedingungen variieren.
- Zeigen, wie mehr Nachfrage zu Aufträgen und Investitionsanreizen führt, ohne garantierte Vollbeschäftigung oder Profit.
- BIP als Wertschöpfung mit dokumentiertem Umfang und Referenzpreisen berechnen; Zwischenhandel und interne Transfers nicht mehrfach addieren. Vor vollständigem Staatssystem Abdeckungsgrenzen benennen.
- Speicher-/Ladevergleich und verschiedene Zeitgeschwindigkeiten prüfen; wiederholte negative Bestände, falsche Reservierungen oder stilles Geldwachstum als Fehler behandeln.
- Balancingkorrekturen offen dokumentieren; kein verborgenes Nachfüllen von Waren/Budgets.

**Abnahme**
- Normalszenario erzeugt wiederholten Lohn–Einzelhandel–Nachschub-Kreislauf und nachvollziehbare Gewinne/Verluste.
- Ausfälle sind wirtschaftlich erklärbar; keine technisch bedingte Blockade nach wenigen Tagen.
- Alle Geld-/Warenabweichungen durch explizite Quellen/Senken oder Änderungen erklärbar.
- Lokale Firmen handeln und zahlen Löhne ohne Eingriff des Spielers. Entfernte Versorgung wird nicht automatisch kostenlos ersetzt; das Szenario zeigt deren reale Abhängigkeiten.
- Bericht benennt Zahlen, Testergebnisse, Leistungswerte und noch offene Spielentscheidungen.

### AP10 – Sichtbares Konzernnetz und vollständiger Aufbauspieltest

**Voraussetzung:** AP09.6.

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
- Mindestens ein dokumentierter Spieltest nennt Entscheidungsstellen, Wartephasen und verbleibende Probleme. Noch nicht bestandene Kriterien sperren den Übergang zu AP11 fachlich; kein bloßes Abhaken implementierter Widgets.

## AP11 – Unternehmenslandschaft, Konkurrenz und Lebenszyklus

Die Unterpakete erweitern dieselben Unternehmen aus AP09. Keine neue Population von Spezial-NPCs neben den vorhandenen Marktteilnehmern. Reihenfolge: AP11.1 → AP11.2 → AP11.3 → AP11.4.

### AP11.1 – Konzernstrategien und lokale Geschäftsführung

**Voraussetzung:** AP10.

**Ziel:** NPC-Unternehmen wachsen und reagieren aufeinander mit denselben Möglichkeiten wie der Spieler.

**Aufgaben**
- Bestehenden regelbasierten Assetbetrieb von strategischen Entscheidungen trennen; Spielerautomatisierung bleibt aktiv, autonome Konzernstrategie nur für NPCs.
- Ziele/Parameter für Liquidität, nachhaltigen Gewinn, Wachstum, Spezialisierung und Risiko definieren; nicht jedes Unternehmen identisch optimieren lassen.
- Lokale Firmen bevorzugen gesicherten Betrieb, Reserve und begrenzten Ausbau ihres Hubs; Konzerne bewerten überregionale und vertikale Expansion. Profile bleiben konfigurierbar und dürfen Entwicklung zulassen.
- Investitionsmöglichkeiten bewerten: weiterer Standort, Produktionsintegration, Laden, Kapazitätsausbau und Kreditfinanzierung.
- Erwartete Nachfrage, Kosten, Transport, Personal und Konkurrenz berücksichtigen; nur öffentliche und eigene Daten.
- Periodische Entscheidungen mit begrenztem Rechenbudget; Fehlschläge und falsch eingeschätzte Chancen zulassen.
- Neue Firmen und Übernahmen noch nicht in AP11.1 erzeugen; diese folgen gezielt AP11.2–11.4.

**Abnahme**
- Mindestens zwei Strategien erzeugen unterschiedliche, begründete Expansion.
- NPC kann ohne Sonderkapital ein Asset bauen oder kaufen und danach regulär betreiben.
- Unprofitable Investition belastet Liquidität; keine perfekte Kenntnis zukünftiger Krisen.
- Mehrmonatiger Vergleich mit/ohne strategische KI sowie Save/Load dokumentiert.

### AP11.2 – Neugründungen und Markteintritt

**Voraussetzung:** AP11.1.

**Ziel:** Neue lokale Unternehmen entstehen aus nachhaltigen Geschäftsmöglichkeiten statt garantiertem Respawn.

**Aufgaben**
- Periodisch pro Hub und geeigneter Branche beobachteten Fehlbedarf, Absatz, Preise, vorhandene Konkurrenz, Inputversorgung, Arbeitskräfte und Standortzugang bewerten.
- Erwarteten Deckungsbeitrag und Anlaufkosten über ein konfigurierbares Beobachtungsfenster prüfen. Ein hoher Preis ohne zahlungsfähige Nachfrage genügt nicht.
- Ein Vorhaben mit Besitzer, Standort, Geschäftsplan, Eigenmitteln, Kreditbedarf und Bau-/Kaufplan anlegen. Unternehmens-ID früh stabil vergeben, aber ohne abgeschlossene Finanzierung keine Betriebsaufnahme.
- Eigenkapital vom realen Investorenkonto überweisen; Kredit nur nach AP09.5. Kein finanziertes Kapital zugleich an mehreren Vorhaben binden.
- Assetbau/-kauf aus AP08.3 verwenden. Wettbewerb um geeignete Standorte, Personal und Inputs zulassen; kein Fertigbetrieb aus dem Nichts.
- Gründungsverzögerung, Abbruch und Freigabe von Reservierungen regeln. Fehler nicht durch verdecktes Geldauffüllen lösen.
- Anti-Pendel-Regeln: Beobachtungsdauer, Abklingzeit je Branche/Hub und begrenzte Vorhabenanzahl. Kein starres dauerhaftes Mindestangebot.
- Vorhaben, Eigentümermittel, zugesagte Kredite und Fertigstellung speichern. Entwicklerkonsole kann bewertete Chancen anzeigen; erzwungene Testgründung bleibt klar gekennzeichneter Entwicklereingriff.

**Abnahme**
- Längerfristig profitable Versorgungslücke kann mit gedeckter Finanzierung eine Neugründung auslösen.
- Ohne Kapital, Standort oder tragfähige Nachfrage bleibt sie aus; keine Gratisfirma.
- Eintritt verursacht echte Zahlungen und Bauzeit. Neuer Laden braucht anschließend Waren und Beschäftigte.
- Gleicher Seed/Zustand ergibt dieselben Entscheidungen; Laden während der Gründung erzeugt kein zweites Unternehmen.

### AP11.3 – Geschäftsaufgabe und geordneter Marktaustritt

**Voraussetzung:** AP11.2.

**Ziel:** Unwirtschaftliche lokale Firmen können schrumpfen oder ausscheiden, ohne Buchungen oder Verpflichtungen verschwinden zu lassen.

**Aufgaben**
- Wiederholte Verluste, Zahlungsrückstände und fehlende Finanzierungsfähigkeit über Zeit bewerten; eine schlechte Woche löst nicht automatisch sofortige Löschung aus.
- Zustände aktiv, restrukturiert/ruhend, Abwicklung und geschlossen unterscheiden. Betrieb pausieren, neue Verpflichtungen blockieren und gültige laufende Aufträge kontrolliert behandeln.
- Offene Marktangebote stornieren und Reservierungen freigeben; Waren/Assets geordnet anbieten. Waren verderben weiter nach Regeln.
- Bereits bezahlte Transporte erfüllen oder ausdrücklich abrechnen; gebundene Assets nicht löschen. Beschäftigte bei Stilllegung freigeben, unbezahlte Ansprüche als Verbindlichkeit erhalten.
- Für verbleibende Gläubigerforderungen ein dokumentiertes vereinfachtes Rang-/Verteilungsschema definieren. Bankabschreibungen gemäß AP09.5 buchen; Guthaben anderer Akteure nicht willkürlich löschen.
- Gesellschaft erst schließen, wenn wirtschaftliche Referenzen abgewickelt sind. Historische ID und Transaktionen für Berichte erhalten.
- Frei werdende Kapazitäten und Standorte stehen regulär dem Markt offen; Schließung löst keine garantierte Ersatzgründung aus.
- Vollständige Rechts-/Gerichtsinsolvenz nicht erforderlich. Der erste Umfang ist eine transparente spielinterne Abwicklung.

**Abnahme**
- Ausfall entzieht tatsächlich Angebot und Beschäftigung; neue Marktchancen können entstehen.
- Keine verwaisten Fracht-, Konto-, Asset- oder Vertragsreferenzen. Abwicklung verteilt nur vorhandene Mittel.
- Schließung erzeugt weder Gratisvermögen noch automatischen Nachfolger.
- Save/Load mitten in Abwicklung führt Zahlungen/Reservierungsfreigaben nicht doppelt aus.

### AP11.4 – Verhandelte Übernahme lokaler Unternehmen

**Voraussetzung:** AP11.3.

**Ziel:** Lokale Firmen sind reguläre Partner und kaufbare Unternehmen, nicht ausschließlich verdrängbare Konkurrenz.

**Aufgaben**
- Kleine Firmen erhalten explizite endliche Eigentümer-/Verkäuferkonten. Verkaufsbereitschaft nach Ertrag, Risiken, Perspektive und Angebot bewerten; Ablehnung ist möglich.
- Übernahmeansicht mit freigegebenem Prüfbericht für Assets, Bestand, Geld, Schulden, Beschäftigung, Verträge und offene Verpflichtungen.
- Angebotspreis und Annahmebereich nachvollziehbar bewerten; gedecktes Angebot reservieren, Frist und Abbruch regeln. Kein garantierter Kauf zum Buchwert.
- Erstes Modell: vollständiger Erwerb durch kontrollierte Eingliederung ins Käuferunternehmen, keine börslichen Beteiligungen oder Tochterkonzern-Hierarchie.
- Kaufpreis an bisherigen Eigentümer zahlen, nicht als Umsatz der Zielbetriebe buchen. Zielguthaben und alle übernommenen Aktiva/Verbindlichkeiten genau einmal übertragen; Ziel-ID als historische Referenz erhalten.
- Laufende Lieferungen, Beschäftigung, Kredite, Sicherheiten und Verträge explizit prüfen und migrieren. Fremdverträge und Kredite nur übernehmen, wenn deren vereinfachte Zustimmungs-/Übertragungsregeln erfüllt sind; andernfalls Abschluss ablehnen oder geordnete Vorabklärung verlangen.
- Nach Eingliederung Eigenhandel verhindern: ehemalige gegenseitige Aufträge stornieren bzw. als interne Beziehungen neu bewerten, ohne Erträge oder Waren zu duplizieren.
- Keine Übernahme der Spielerfirma durch NPCs in diesem ersten Umfang. Größere Konzernfusionen und Teilbeteiligungen bleiben spätere Erweiterung.
- Abschluss atomar an Schrittgrenze; neue private Daten erst nach legitimer Prüfung/Übernahme zugänglich machen. State, Kaufpreisreservierung und Abschlussstatus speichern.

**Abnahme**
- Freiwillige Übernahme einer lokalen Firma erweitert Besitz und Verpflichtungen des Käufers, ohne Vermögensduplikation.
- Verkäufer erhält vereinbarten Preis, Käufer trägt übernommene Schulden. Externer Umsatz/Gewinn wird durch den Kauf nicht künstlich erhöht.
- Abgelehntes oder nicht finanzierbares Angebot ändert keine Eigentümer.
- Laufender Transport und Vertrag funktionieren nach Abschluss unter korrektem Besitzer weiter; automatische Regeln sind eindeutig zugewiesen.
- Save/Load vor und nach Abschluss erzeugt weder doppelte Zahlung noch erneuten Eigentumsübergang.

### AP12 – Staatliche Wirtschaft und Haushalt

**Voraussetzung:** AP11.4.

**Ziel:** Staaten werden wirtschaftliche Akteure mit endlichen Mitteln und Verpflichtungen.

**Aufgaben**
- Minimalen Staatshaushalt aus AP09.5a gezielt erweitern, nicht neu anlegen. Bestehende öffentliche Beschäftigung, Einnahmen und Transfers übernehmen.
- Regeln für weitere Einnahmen, Aufträge, Transfers, Subventionen und Schuldendienst ergänzen; Länder bestimmen Regeln, regionale Ausgaben laufen weiterhin aus einem Haushalt.
- Bestehende einfache Abgabe integrieren und fachlich erweitern: Gewinnsteuer auf definierte steuerliche Ergebnisse, gegebenenfalls Lohnabgaben und Zölle als getrennte konfigurierbare Instrumente. Nationale Regeln nicht rückwirkend ändern.
- Staatliche Aufträge für tatsächlich gelieferte Leistungen/Waren mit Budget, Annahme und Zahlung; abstrakte öffentliche Leistungen offen ausweisen.
- Transfers/Subventionen nach Bedingungen und gedecktem Budget; Auslaufen und nicht gedeckte Zusagen regeln.
- Staatskredite über vereinfachte Bank-/Anleihemechanik mit expliziten Käufern/Bilanzfolgen, keine Annahme unbegrenzten Staatsgeldes.
- Haushalts-/Versorgungs-/Beschäftigungskennzahlen für spätere Regierungsentscheidungen bereitstellen; BIP nicht als einziges Ziel.

**Abnahme**
- Steuern übertragen Geld, Staatsausgaben haben Empfänger und reale Bedingungen.
- Ein Defizit braucht vorhandene Rücklagen oder dokumentierte Finanzierung; sonst Kürzung/Rückstand.
- Zuschüsse können Branchen fördern und belasten den Haushalt.
- Haushaltsabschluss, Schulden, Zahlungsplan und Regeln über Save/Load konsistent.

### AP13 – Nationale und internationale Politik / Lobbyismus

**Voraussetzung:** AP12.

**Ziel:** Politische Regeln verändern sich eigenständig und können von Unternehmen beeinflusst werden.

**Aufgaben**
- Regierung mit Agenda und Gewichten für Versorgung, Beschäftigung, Haushalt und politische Unterstützung konfigurieren.
- Periodische Überprüfung sowie begrenzte Krisenanlässe; Vorlauf, Mindestgeltung und Abklingzeit verhindern tägliches Umschalten.
- Vorhaben mit Status vorgeschlagen, beraten, beschlossen/abgelehnt, wirksam und ausgelaufen modellieren.
- Unternehmen können Maßnahmen anregen und laufende Vorhaben unterstützen/bekämpfen. Lobbyressourcen kosten Geld/Zeit, Erfolg bleibt abhängig von Interessen und Gegenkräften.
- Einflussquellen getrennt darstellen: Arbeitsplätze, Versorgung, Investitionen, Beziehungen; keine reine Geld-gegen-Gesetz-Garantie.
- Erste Instrumente: Branchenförderung, Steuer-/Abgabenänderung, Umweltanforderungen als Produktionskosten-/Voraussetzungsregel, Zölle und begrenzte Ausnahmen.
- Für internationale Tests zweites Land als kleines explizites Handelsszenario ergänzen, keine vollständige Weltkarte voraussetzen. Abkommen, Importregeln und Gegenmaßnahmen bilateral modellieren, keine Weltregierung.
- Import-/Exportherkunft, Zeitpunkt, Route und Abgaben eindeutig prüfen. Abstrakte Umgehungsmechaniken erst nach funktionierenden Handelsregeln gesondert spezifizieren.
- Politische UI zeigt Vorhaben, Betroffene, Kosten, Unsicherheit und Wirksamkeitsdatum. Korruption ist ein später zu verfeinernder Weg, keine verpflichtend sofortige Funktion.

**Abnahme**
- Staat schlägt auch ohne Spielereingriff eine begründete Maßnahme vor.
- Spieler kann Vorhaben anstoßen und reagieren, Konkurrenten vertreten Gegeninteressen.
- Beschlossene Regeln verändern tatsächliche Buchungen/Produktionsbedingungen genau zum Geltungszeitpunkt.
- Bilateraler Zolltest zeigt gegensätzliche Interessen und mögliche Gegenreaktion.
- Politikstatus, Beziehungen, laufende Lobbyaktivitäten und Regeln werden vollständig gespeichert.

### AP14 – Ereignisse und Krisen

**Voraussetzung:** AP13; normaler Wirtschaftskreislauf bleibt ohne Krisen spielbar.

**Ziel:** Ereignisse verändern bestehende Regeln und schaffen Chancen/Risiken statt künstlicher Preiseffekte.

**Aufgaben**
- Ereignisse mit Typ, Region/Land, Stärke, Beginn, Dauer und aktiven Effekten definieren. AP07a-Ereignisregistry und Start-/Stop-/Listenbefehle jetzt implementieren.
- Zunächst Dürre mit begrenzter Nahrungsproduktion; Prognose und tatsächlichen Verlauf trennen, Unsicherheit reproduzierbar über Seed.
- Output, Transportkapazität oder Nachfrage nur über vorhandene Mechaniken verändern; keine pauschale garantierte Preisexplosion.
- Firmen, Haushalte und Regierung reagieren über ihre jeweiligen Entscheidungsregeln.
- Start/Stop und Überschneidung regeln; Stop macht bereits geschehene Verluste und Geschäfte nicht rückgängig.
- Szenario mit Eintritt und Ausbleiben erwarteter Krise; Lagerkosten/Verderb erlauben Spekulationsverluste.
- Laufende Effekte, Prognosen und Zufallszustand speichern und in UI/Diagnose kenntlich machen.

**Abnahme**
- Konsole löst Dürre kontrolliert aus; Versorgung und Produktion ändern sich nachvollziehbar.
- Lieferung lindert Knappheit, Zurückhalten kann sie verschärfen, Gewinn ist nicht garantiert.
- Ungültiger Ereignisbefehl hinterlässt keinen Teilzustand.
- Normalpartie ohne Ereignisse bleibt funktionsfähig; Save/Load verdoppelt Effekte nicht.

### AP15 – Gesamtprüfung und Auslieferung

**Voraussetzung:** AP14.

**Ziel:** Einen fortsetzbaren, paketierten Gesamtprototyp liefern.

**Aufgaben**
- Bestehende Saves aus AP08.4 und nachfolgenden Erweiterungen auf Vollständigkeit prüfen; kein neues Speichersystem daneben bauen.
- Assetbau, Marktreservierungen, Einzelhandel, Kredite, Staatsregeln, Lobbyvorhaben und Ereignisse im kombinierten Save/Load-Test prüfen.
- Development- und Shipping-Build paketieren, Daten-/Material-/UI-Verfügbarkeit offline prüfen.
- Entwicklerkonsole und privilegierte Endpunkte in Shipping entfernen; normale Spieleraktionen behalten ihre Berechtigungsprüfungen.
- Spielerablauf, Fehlermeldungen, Hilfe und Auflösungen prüfen; keine versteckten Debugaktionen als Pflichtbedienung.
- Startanleitung, Grenzen, Versionierung und bekannte Probleme liefern.

**Abnahme**
- Neue Partie mit Unternehmenswahl sowie Speichern/Programmneustart/Laden vollständig bedienbar.
- Ein wirtschaftlicher und ein politischer Langzeittest im paketierten Build bestanden.
- Keine doppelte Zahlung, Zustellung, Fertigstellung oder Ereignisanwendung nach Laden.
- Kritische offene Fehler und nicht geprüfte Punkte ausdrücklich benannt.

### AP16 – Skalierung messen und gezielt parallelisieren

**Voraussetzung:** AP15; Messinstrumentierung besteht seit AP04.

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


## 5. UI und Karte – Spieleroberfläche

Stand: 19.11.2026. Eigenständige Paketfolge UI01–UI06, ohne Umnummerierung von AP09–AP16. Diese Pakete dürfen auf dem bestehenden UI vorgezogen werden; sie warten nicht auf AP16. Fehlende wirtschaftliche Berechnungen werden fachlich angebunden oder als nicht verfügbar angezeigt, niemals im Widget erfunden.

Die folgenden Vorgaben präzisieren ältere HUD- und Stadtansichtsvorgaben. Vorhandene Spielaktionen, Rechteprüfung und Simulation erhalten. Ziel: Übersicht beantwortet „Wie läuft es und was kann ich tun?“, Details erklären warum.

### UI01 – Kennzahlenvertrag und gemeinsame Darstellung

**Voraussetzung:** Vorhandenes Firmen-HUD, PlayerCompanyId und Stadtansicht.

**Aufgaben**
- Bestehende Widgets und Datenquellen prüfen; gemeinsame Präsentationsdaten aus konsistenten Snapshots verwenden. Keine zweite UI-Buchhaltung.
- Formatierer für USD, Prozent, Mengen und Zeiträume vereinheitlichen. Keine Centrohwerte oder internen IDs im normalen Spieler-HUD.
- Mitarbeiter = aktive Beschäftigte eigener Assets, ohne offene Stellen oder Doppelzählung.
- Standorte = unterschiedliche Regionshubs mit eigenen fertiggestellten aktiven Assets; Hauptsitz nicht zusätzlich zählen.
- Ergebnis = realisiertes Ergebnis des letzten abgeschlossenen Monats nach vorhandener Kostenrechnung, interne Umsätze ausgeschlossen. Zahlungen sind nicht automatisch Gewinn.
- Nettovermögen = bewertete Vermögenswerte abzüglich Verbindlichkeiten nach sichtbarer Bewertungsmethode. Geldreservierungen nicht nochmals als Schulden abziehen; keine Gleichsetzung mit Übernahmewert.
- Fehlende Bewertungsgrundlage als „Nicht verfügbar“, fehlende Historie als „Noch keine Monatsdaten“ anzeigen. Keine scheinpräzisen Fantasiewerte.
- Wiederverwendbare Kennzahlfelder, Tooltips, Tabellenzeilen und Leerzustände bereitstellen.

**Abnahme**
- Kennzahlen entsprechen dem Simulationszustand. Quelle, Zeitraum und Berechnung stehen im Tooltip.
- Bevölkerung und Firmenbeschäftigte werden bei Hub-/Regionsaggregation nicht doppelt gezählt.
- Fehlende Daten und echte Nullwerte sind unterscheidbar.

### UI02 – Unternehmensleiste und Navigation

**Voraussetzung:** UI01.

**Aufgaben**
- Links Firmenname, vorhandenes Zeichen/Firmenfarbe; daneben verfügbares Geld, Ergebnis, Mitarbeiter, Standorte und Nettovermögen.
- Reserviertes Geld und Gesamtkontostand im Geldtooltip bzw. kompakten Detail ausweisen. Verfügbar = Kontostand minus Reservierungen.
- Rechts Datum/Uhrzeit, Pause und bestehende Zeitgeschwindigkeiten mit eindeutigem Aktivzustand.
- Kennzahlen öffnen passende verfügbare Unternehmensdetails. Keine funktionslosen Sprungziele.
- Spielstände und Einstellungen ins Pausenmenü verlagern, Entwicklerdiagnose separat einblendbar halten.
- Bei geringer Breite zwei kompakte Zeilen oder Sekundärgruppe verwenden, keine abgeschnittenen Werte.

**Abnahme**
- Unternehmenslage ist ohne Diagnoseansicht verständlich.
- Lange Firmennamen, große Werte und negativer Gewinn bleiben lesbar.
- Neue Partie mit anderer PlayerCompanyId aktualisiert alle Anzeigen und Aktionen.

### UI03 – Mehr Kartenfläche und kontextbezogene Bedienung

**Voraussetzung:** UI02 und bestehende Layer/Routen.

**Aufgaben**
- Kartenanalyse einklappbar machen; nur zum Layer passende Felder zeigen. Regionenansicht benötigt keine Warenauswahl.
- Routen-/Fahrzeugoptionen in ausklappbare Anzeigeoptionen verlagern. Filterwerte beim Wechsel erhalten.
- Kleine Legende am Kartenrand mit Skala, Einheit, Zeitraum und Datenstand.
- Genau ein rechtes Auswahlpanel mit Name, Typ, Region/Land, wenigen Kennzahlen und sinnvollen Aktionen.
- „Stadt öffnen“ als primäre Hubaktion; keine suggerierte territoriale Eigentümerschaft.
- Großen leeren Erklärungskasten entfernen; unteren Hinweisbalken nur bei konkreter Aktion zeigen.
- Ein dezenter Testszenariohinweis genügt; „TEST“, Hinweise zu Exemplar-Lkw und Aggregation in Tooltip/Diagnose statt Dauertext.
- Regionsauswahl/Highlight mit Stadtfenster verbinden; Sichtbarkeitsregeln für Konkurrenten erhalten.

**Abnahme**
- Standardansicht lässt der Karte Platz statt mehrerer großer Infokästen.
- Layer und Auswahl öffnen passende Aktionen, ohne wirtschaftliche Daten zu verändern.
- UI-Klicks und Mausrad gelangen nicht versehentlich zur Karte.

### UI04 – Kompakter Stadtbildschirm

**Voraussetzung:** UI01 und UI03.

**Aufgaben**
- Kopf: „Chicago“, darunter „Wirtschaftsregion Chicago · USA“ sowie eigene Präsenz, etwa „Niederlassung · 1 Lager“.
- Doppelte Titel, breite Formularbeschriftungen und Erklärtexte wie „kein Stadtbesitz“ entfernen. Stadtwechsel kompakt weiterhin ermöglichen.
- Hover auf Regionsangabe hebt das abgedeckte Gebiet hervor; „Auf Karte zeigen“ zentriert auf Hub/Region.
- Tabs: Übersicht, Markt, Eigene Assets, Investitionen. „Global“ durch separate Aktion „Unternehmensübersicht“ ersetzen.
- Regionale Kennzahlgruppe: Bevölkerung, Arbeitslosigkeit, Grundversorgung und Kaufkraft/Konsumbudget mit Zeitraum.
- Eigene Kennzahlgruppe: Beschäftigte, Assets nach Typ, Lagerauslastung und Standortergebnis.
- Kennzahlen aus bestehenden fachlichen Definitionen übernehmen. Nicht definierte Kaufkraft, Versorgungsaggregation oder Standortergebnisse zunächst als nicht verfügbar kennzeichnen.
- „Chancen und Probleme“ mit wenigen belegten Hinweisen und Aktionen ergänzen, etwa Nahrungsmangel oder fast volles eigenes Lager. Schwellen konfigurieren.

**Abnahme**
- Hub, abgedeckte Region und eigene Rolle sind sofort erkennbar.
- Übersicht ist ohne langen Textblock verständlich.
- Hinweise haben reale Datengrundlage und führen zum betroffenen Markt/Asset.

### UI05 – Tabellen und aufklappbare Details

**Stand:** Laut Projektinhaber umgesetzt. Bestehende Komponenten für UI07–UI10 wiederverwenden.

**Voraussetzung:** UI04.

**Aufgaben**
- „Unternehmen vor Ort“ als Tabelle: Firma, Hauptsitz/Niederlassung, Tätigkeiten, öffentlich sichtbare Präsenz. Eigene Firma hervorheben.
- Kryptische kombinierte Spalten wie „Betriebe / öffentliche Lkw“ aufteilen oder ins Detail verlegen.
- Bevölkerung/Konsum aufklappbar: Einkommensgruppen als Zeilen, Einkommen, Ausgaben, Ersparnisse, Arbeitslosigkeit sowie Grund-/Freizeit-/Luxusversorgung verständlich darstellen.
- USD statt Cent, klare Perioden und benannte Skalen. „+0/1000“ ohne Erklärung vermeiden.
- Auf kleinen Auflösungen Gruppen-Details statt unlesbarer sehr breiter Tabellen.
- Technische Texte über doppelte Summierung und IDs in Diagnose verschieben. Spielerklärungen als Tooltip.
- Lager-, Markt-, Asset- und Investitionsaktionen weiter mit vorhandenen Dialogen verbinden.

**Abnahme**
- Nützliche Details bleiben erreichbar, dominieren aber nicht den Übersichtskopf.
- Öffentliche Firmenansicht offenbart keine privaten Konten oder Aufträge.
- Sortierung/Updates bewahren möglichst Auswahl und Scrollposition; leere Listen erklären ihren Zustand.

### UI06 – Gestaltung und durchgängige Bedienprüfung

**Voraussetzung:** UI02–UI05; abschließende Gesamtprüfung nach UI07–UI10 wiederholen.

**Aufgaben**
- Ruhige dunkle Flächen, warme helle Schrift, sparsame Messingakzente und Firmenfarbe konsistent anwenden; Lesbarkeit vor historischer Dekoration.
- Typografie, Abstände, Buttonzustände, Tooltips und Scrollbereiche vereinheitlichen.
- 1280×720 und 1920×1080 mit DPI-Skalierung visuell prüfen, Screenshots vergleichen und Layout korrigieren.
- Ablauf testen: Unternehmensüberblick → Stadt → Markt → Handel → Transport → Meldung → Assetdetail.
- Fälle ohne Historie, mit Verlust, langen Namen, vielen Firmen, leerem Sortiment, vollen Lagern und laufender Simulation prüfen.
- Escape schließt zuerst Dialog dann Hauptfenster; Texteingabe blockiert Kameratasten. Fenster pausieren nicht automatisch.
- Snapshotwechsel, Laden und neue Unternehmenswahl auf konsistente Werte prüfen. UI-Kosten messen; Tabellen nicht pro Frame vollständig neu aufbauen.

**Abnahme**
- Firmenlage, ausgewählte Region und nächste Aktion sind ohne Diagnose verständlich.
- Keine abgeschnittenen Schaltflächen, unlesbaren Tabellen oder Eingaben durch Fenster auf die Karte.
- HUD, Stadt und Unternehmensansicht zeigen beim gleichen Snapshot dieselben Werte.
- Handel, Lieferregeln, Investitionen, Firmenrechte und Save/Load funktionieren nach UI-Umbau.
- Diagnose bleibt erreichbar, ist aber standardmäßig verborgen; fehlende fachliche Kennzahlen sind dokumentiert.

### UI-Erweiterung vom 21.11.2026 – Seitenpanels und Handel

**Arbeitsstand:** UI05 ist laut Projektinhaber umgesetzt. Bestehende UI01–UI05-Komponenten weiterverwenden; ihren tatsächlichen Code prüfen und nicht erneut vollständig bauen. Nächster neuer Auftrag UI07. Danach UI08 → UI09 → UI10; UI06 abschließend auf das überarbeitete Gesamt-UI anwenden. Wirtschafts- und MAP-Paketnummern bleiben unverändert.

**Neue Layoutregel:** Die Karte bleibt Arbeitsfläche. Stadt und Handel öffnen in einem linken angedockten Hauptpanel unter der oberen Leiste. Sie ersetzen sich, statt mehrere zentrale Fenster zu stapeln. Maximal eine ergänzende Detail-/Auftragsvorschau daneben. Die funktionale Aufteilung der Strategiereferenzen übernehmen, nicht deren fremde Grafiken oder mittelalterliches Thema.

### UI07 – Seitliches Stadtpanel und Fensterführung

**Voraussetzung:** Vorhandene Stadtansicht einschließlich UI05.

**Aufgaben**
- Zentriertes Stadtfenster durch linkes angedocktes Panel ersetzen. Kopf mit Stadt, Region, eigener Präsenz und Schließen; Tabs Übersicht, Markt, Assets, Investitionen erhalten.
- Als Startlayout etwa ein Drittel der nutzbaren Breite; bei 1920×1080 z. B. 560–640 px, bei 1280×720 etwa 400–440 px als abstimmbare Testwerte. Keine breiten Tabellen unverändert hineinpressen.
- Im Panel Kennzahlen kompakt, Listen als Zeilen/Karten mit aufklappbaren Details. Breite Detailtabellen in ergänzendem Panel bzw. Detailansicht, kein permanentes horizontales Scrollen der Übersicht.
- Eine Hauptpanelverwaltung mit Stadt/Handel/Logistik als Ansichten nutzen. Zurücknavigation erhält Auswahl, Filter und Scrollposition. Maximal ein Detailpanel; neues Detail ersetzt das vorherige.
- Bestehendes rechtes Auswahlpanel bei geöffnetem Stadtpanel reduzieren/ausblenden, wenn es denselben Inhalt dupliziert. Karte bleibt außerhalb belegter Panelrechtecke bedienbar.
- Klick auf andere Stadt aktualisiert dasselbe Panel. Ungespeicherten Auftragsentwurf nicht still verwerfen: Entwurf erhalten oder explizit schließen lassen.
- Kamerazentrierung nutzt die freie Kartenfläche rechts des Panels; Hub darf nicht hinter dem Panel landen. Kein Kamerasprung bei jedem Kennzahlenupdate.
- Escape schließt erst Unterdetail, dann Hauptpanel; UI-Eingaben blockieren Kartenbedienung nur im passenden Fokus.

**Abnahme**
- Stadtpanel liegt seitlich und lässt ausgewählten Hub/Region sichtbar.
- Wechsel zwischen Städten erzeugt kein zusätzliches Fenster und verliert keine bestätigten Aufträge.
- UI05-Details und alle Markt-/Investitionsaktionen bleiben erreichbar.
- 1280×720 und 1920×1080 mit langen Namen und vollen Listen visuell geprüft.

### UI08 – Handelsmenü als Einstieg für Geschäfte

**Voraussetzung:** UI07, bestehende Orderbücher und Firmenberechtigungen.

**Aufgaben**
- Hauptnavigation „Märkte“ zu „Handel“ weiterentwickeln, vorhandene Marktlogik integrieren statt ersetzen.
- Drei Bereiche: Angebote, Handelsplanung, Laufende Geschäfte. Unterliegende Regionsmärkte, B2B-Regeln und physische Angebotsorte erhalten.
- Angebote nach Ware, Region/Hub, Anbieter und Menge filtern. Preis, verfügbare Menge, Quellort und Datenstand zeigen; keine globale erfundene Sofortlieferung.
- Stadtaktion „Markt/Handel öffnen“ übernimmt Ort und Warenauswahl ins gleiche seitliche Handelsmenü.
- Handelskauf kann eigene Produktion versorgen, Bestände auffüllen oder späteren Weiterverkauf vorbereiten; Weiterverkauf nicht voraussetzen.
- Laufende Geschäfte verknüpfen bestehende Markt- und Transport-IDs mit Mengen. Zustände abbilden: Auftrag offen, teilweise gekauft, gekauft/Abholung erforderlich, Transport geplant, unterwegs, angekommen, Verkaufsauftrag offen, abgeschlossen; Fehler/Teilmengen sichtbar.
- Eine neue Handelsvorgangs-ID dient nur der Organisation und erzeugt keinen zweiten Waren-/Geldbestand. Saveformat bei neuem dauerhaften Zustand erweitern, keine Altspielstände migrieren.
- Normale Ladenverkäufe an Haushalte bleiben eigenständiger Einzelhandel; Handelsmenü ist B2B-/Unternehmensbedienung.

**Abnahme**
- Spieler findet Angebot, kennt dessen tatsächlichen Ort und beginnt daraus eine Planung.
- Derselbe Marktauftrag erscheint nicht als zwei unterschiedliche Geschäfte durch mehrfaches Öffnen.
- Fremde private Aufträge bleiben geschützt. Aktuelle Simulation statt alter Vorschau bestimmt Verfügbarkeit.

### UI09 – Kauf und anschließenden Transport zusammen planen

**Voraussetzung:** UI08 und vorhandener Transportdienst; Wasserangebote nur falls MAP05 bereits umgesetzt.

**Aufgaben**
- Geführter Ablauf im Handelsmenü: Angebot → Menge → Ziellager/-Asset → eigener oder gemieteter Transport → Kostenübersicht → ausdrückliche Bestätigung.
- Preislimit/Kaufbedingungen aus vorhandenen Orderbüchern verwenden. Bereits gekaufte Ware nicht neu kaufen; Transportentwurf separat möglich.
- Vorschau zeigt Warenpreis, Transport/Umschlag, bekannte Lagerkosten, Gesamtausgabe, Ankunft und benötigte Kapazität. Laufende zukünftige Lagerkosten nur als Zeitraumannahme, nicht als garantierter Fixbetrag.
- Optionalen Weiterverkauf mit unverbindlichem geschätztem Erlös/Ergebnis kennzeichnen. Kein Verkauf fremder oder noch nicht verfügbarer Ware und keine garantierte Marge.
- Vor Bestätigung Kauf und Transport gemeinsam auf Plausibilität prüfen. Diese Vorschau reserviert nicht automatisch alles und ist kein versprochenes globales atomisches Geschäft.
- Ausführung über vorhandene validierte Befehle: erst tatsächliche Kaufmenge bestimmen, dann nur erworbene Menge für Transport buchen. Teilfüllungen und asynchrone Bestätigung abbilden.
- Scheitert Kauf: kein Frachtauftrag. Gelingt Kauf, scheitert danach Transport: Ware und Eigentum bleiben am Quellort, Status „Gekauft – Abholung erforderlich“, Ursache und Aktion „Transport erneut planen“.
- Transportbuchung selbst weiterhin atomar reservieren: Ware, Zahlung, Fahrzeug/Anbieter und Zielkapazität gemeinsam oder gar nicht. Keine Teilreservierungen nach Ablehnung.
- Keine automatische Wiederholung des Kaufs beim erneuten Transportversuch. Doppelklick/erneutes Öffnen mit identischer Befehls-ID erzeugt keine Doppelbestellung.
- Änderungen von Preis, Anbieterangebot oder Lagerkapazität beim laufenden Spiel erneut prüfen und in aktualisierter Vorschau bestätigen lassen, sofern sie die autorisierten Bedingungen überschreiten.
- Schließen des Plans storniert keine bereits ausgeführten Käufe. Offene tatsächliche Aufträge nur über explizite Stornoaktion ändern.
- Kaufmenge, Abholstatus und Transportverknüpfungen innerhalb derselben kompatiblen Saveversion speichern.

**Abnahme**
- Kauf mit Lieferung vom Handelsmenü aus vollständig bedienbar, ohne Quellort/Menge im Transportfenster nochmals abzuschreiben.
- Kontrollfälle: keine Ware, zu wenig Geld, Teilfüllung, Ziel voll, eigenes Fahrzeug belegt, Fremdanbieter inzwischen ausgebucht.
- Transportfehler nach Kauf zeigt korrekten verbleibenden Warenbesitz; erneute Planung kauft nichts doppelt.
- Save/Load zwischen Kauf und Transport bestätigt denselben Abholbedarf, keine zweite Zahlung/Zustellung.
- Bei keinem Weiterverkaufsplan wird kein scheinbarer Handelsgewinn angezeigt.

### UI10 – Logistikübersicht, Karte und Gesamtintegration

**Voraussetzung:** UI09.

**Aufgaben**
- Bestehendes Transportmenü als Logistikübersicht erhalten: Flotte, Standort, Auslastung, laufende Lieferungen, Leer-/Rückfahrten und wiederkehrende Verbindungen.
- Transport bereits eigener Ware und interne Lieferungen bleiben ohne neuen Marktkauf planbar.
- Handelsvorgang öffnet zugehörigen Transport, Logistik öffnet zugehöriges Geschäft, Quelle/Ziel lässt sich auf der Karte markieren.
- Im Planungsmodus Route und Start/Ziel als Vorschau visualisieren; kein Simulationsauftrag allein durch Kartenklick. Abgebrochene Vorschau entfernen.
- Hauptpanelwechsel und Meldungen erhalten Kontext. „Abholung erforderlich“ führt zur konkreten verbleibenden Menge und ihrem Quelllager.
- Vorhandene Vertrags-/Automatisierungslogik unverändert nutzen. Ein UI-Handelsvorgang darf nicht zusätzlich zu einer Lieferregel dieselbe Ware disponieren.
- Durchgängige UI06-Abnahme wiederholen: Seitenlayout, Kartenfläche, Unternehmenskontext, Fokus, Fehlermeldungen, Performance und beide Zielauflösungen.
- Kein historisches Save-Migrationssystem; bei inkompatibler Schemaänderung frische Partie verlangen.

**Abnahme**
- Standardfall Angebot → Kauf → Lieferung → Lager ist aus Handel heraus erreichbar.
- Bestandsumlagerung/Leerfahrt bleibt aus Logistik erreichbar, ohne künstlichen Handel.
- Karte bleibt neben Panels nutzbar; eigene/fremde Datenzugriffe sind korrekt.
- Wiederkehrende Lieferungen, Investitionen und alle UI05-Details funktionieren nach dem Umbau.
- UI06 gilt erst nach Prüfung des neuen Layouts und Handelsablaufs als abschließend bestanden.

## 6. Karte und geografisches Transportmodell – MAP01 bis MAP06

**Stand: 20.11.2026. Verbindliche neue Zielrichtung.** Die ganze Erde bleibt das langfristige Ziel. Erster aktiver Testausschnitt wird Deutschland und Niederlande zum Szenariostart 01.01.1913. Der bisherige US-Datensatz bleibt höchstens als abgeschlossener Testbestand, nicht als aktive geografische Vorgabe. Wirtschaft, UI, Kameralogik und bestehende Aufgabenpakete werden weiterverwendet; kein neuer Spielkern.

**Vorrang und Reihenfolge**
- Dieses Kapitel ersetzt frühere allgemeine Vorgaben zum US-Startszenario und fiktiven Wirtschaftsregionen. Historische abgeschlossene AP bleiben Implementierungshistorie.
- MAP01 legt verbindlich Quellen und Gebietseinteilung fest; MAP02–MAP04 integrieren Geografie und neue Szenariodaten. MAP05 erweitert den bestehenden Transportdienst; MAP06 prüft alles zusammen.
- MAP01–MAP04 vor neuen geografisch abhängigen Inhalten beauftragen. Das Bankenpaket bleibt fachlich unabhängig, verwendet aber nach Kartenumstellung das neue Szenario. MAP05 nach stabilen Hub-/Hafendaten und vor spielbarer Wasserlogistik.
- Keine Migration alter Spielstände. Neues Szenario-/Saveformat versionieren und neue Partie verlangen. Keine vorhandenen Projektstände oder Dateien ungefragt löschen.

### Verbindliches Geografiemodell

| Ebene | Funktion |
|---|---|
| Land | Nationale Regeln, Wirtschaftsauswertung und spätere Politik |
| Verwaltungsregion | Bundesstaat, Bundesland, Provinz oder historisch vergleichbare Spieleinheit innerhalb eines Landes |
| Stadthub | Genau ein wirtschaftlich repräsentativer Hub je Region; keine zusätzlichen Wirtschaftshubs für Hafen- oder Flussknoten |
| Land-/Gewässergeometrie | Küsten, Inseln, Meere, Seen und Flüsse; getrennt von politischer Zugehörigkeit |
| Transportnetz | Nutzbare Land- und Wasserverbindungen mit Zugang, Zeit, Kosten und Kapazität |

- RegionId und HubId bleiben getrennte stabile Referenzen mit validierter 1:1-Zuordnung. Alle regionalen Haushalte und Assets werden genau einmal zugeordnet.
- Hub ist die wirtschaftlich wichtigste Stadt der jeweiligen Region um 1913, nicht automatisch die Hauptstadt. Auswahlkriterien: Industrie/Handel, Beschäftigung bzw. Bevölkerung und Logistik. Quellen und kurze Begründung pro Hub; bei mehreren plausiblen Kandidaten Wahl transparent dokumentieren.
- Historische nationale Staatsgrenzen verwenden. Deutschland 1913 nicht einfach mit heutigen Bundesländern und Grenzen abbilden.
- Historische Verwaltungsebenen sind nicht überall vergleichbar. MAP01 erstellt eine bewusste, lückenlose Spieleinteilung: Provinzen und kleinere Gliedstaaten können als dokumentierte vergleichbare Regionen dienen. Kein stilles Vermischen überlappender Verwaltungsebenen.
- Ziel Deutschland/Niederlande als zusammenhängendes Szenario; Nachbarländer bei Bedarf als nicht simulierten Hintergrund darstellen, ohne ihnen eine kostenlose Transitfreigabe zu geben.
- Land/Wasser und Verwaltung sind unterschiedliche Dimensionen: Wasser kann innerhalb einer Region oder entlang einer Grenze liegen. Eine Landmaske entscheidet nicht allein über Staatszugehörigkeit.
- Wasserfläche oder Flusslinie bedeutet nicht schiffbar. Nur ausdrücklich für 1913 freigegebene Netzabschnitte dürfen Wasserfracht tragen.
- Hubs, Häfen, Umschlagpunkte und Flusskreuzungen sind unterschiedlich: Ein Hafenknoten kann im Umland eines Hubs liegen, besitzt aber keine zweite Bevölkerung oder einen eigenen zusätzlichen Regionsmarkt.
- Transportplanung bleibt im Menü. Keine Fahrsteuerung, detaillierte Straßensimulation oder Schiffsphysik. Kartenfahrzeuge bleiben exemplarische Darstellung.

### MAP01 – Historischer Datenplan und regionale Einteilung

**Voraussetzung:** Bestehender Kartenimport, Hubmodell und funktionierende Simulation.

**Aufgaben**
- Vorhandene Import-/Darstellungs-/Szenariostruktur prüfen und kleinste erforderliche Anpassung planen.
- Quellen für nationale Grenzen, regionale Unterteilungen, Stadtkoordinaten, Küsten, Seen und historisch schiffbare Gewässer recherchieren. Datenstand, Lizenz, Attribution, Weiterverwendbarkeit und Genauigkeit dokumentieren.
- Eine vollständige Liste der im Testgebiet verwendeten Verwaltungsregionen mit Land, historischem Typ, Gültigkeit und Hubvorschlag erstellen.
- Für Deutschland historische Ebenen bewusst harmonisieren; fehlende Grenzdaten als Lücke ausweisen. Keine moderne Grenze als 1913 ausgeben.
- Jeden Hub wirtschaftlich begründen. Unklare Fälle und vorgeschlagene Spielvereinfachungen ausdrücklich kennzeichnen.
- WGS84 als Quelldatenbasis erhalten; Projektion und Skalierung für den Ausschnitt dokumentieren, weltweiten Ausbau/Datumsgrenze als spätere Voraussetzung einplanen.
- Gewässerliste mit sichtbarer Geometrie einerseits und belegter Schiffbarkeit andererseits erstellen. Kanäle nur einbeziehen, wenn 1913 vorhanden und nutzbar belegt.
- Datenmanifest, IDs, Reproduktionsschritte und überprüfbare Umfangsliste liefern. Grundlegende Quellen-/Einteilungskonflikte vor MAP02 sichtbar machen.

**Abnahme**
- Länder und Regionen bilden eine dokumentierte Hierarchie ohne bewusste Überlappungsfehler.
- Jede gewählte Region besitzt genau einen begründeten Hub.
- Historisch belegte Daten, vereinfachte Geometrien und Balancingannahmen sind unterscheidbar.
- Quellen/Lizenzen erlauben den vorgesehenen Import; Lücken sind nicht durch erfundene Fakten verdeckt.

### MAP02 – Land, Gewässer und Verwaltungsregionen importieren

**Voraussetzung:** MAP01 mit festgelegtem Datenmanifest.

**Aufgaben**
- Reproduzierbare Offline-/Editorpipeline für Länder, Regionen, Land-/Wasserflächen und Flussverläufe erweitern.
- Geometrie vereinfachen/triangulieren, Inseln, Löcher, Grenzflüsse und Küsten behandeln. Darstellung darf keine politische Fläche ungeprüft mit Wasserfläche gleichsetzen.
- Eindeutige Region-/Land-ID-Abfrage und getrennte Land-/Wasserabfrage bereitstellen.
- Kleine Flüsse dürfen als Linien visualisiert werden; Maske/Fläche und Netzlinie müssen nicht dieselbe Auflösung besitzen. Kein Navigieren anhand bloßer Bildschirmfarbe.
- Hubs nach geografischen Koordinaten positionieren. LOD-/Zoomdarstellung vorbereiten, keine Erde in Originalgröße.
- Referenzen und Geometriequalität automatisch prüfen: ungültige Polygone, doppelte IDs, unzugeordnete Regionen und unplausible Hubpositionen.
- Externe Datendateien/Assets für paketierten Offlinebetrieb verfügbar machen.

**Abnahme**
- Deutscher/niederländischer Ausschnitt mit nachvollziehbaren Grenzen, Küste, Wasser und allen vorgesehenen Hubs sichtbar.
- Land-/Wasserklick und politische Auswahl stimmen an repräsentativen Testpunkten; Fluss-/Küsten-Grenzfälle dokumentiert.
- Erneuter Import erzeugt dieselben IDs und Zuordnungen, keine manuelle Neuerstellung erforderlich.

### MAP03 – Stadthubs und frisches Wirtschaftsszenario

**Voraussetzung:** MAP02.

**Aufgaben**
- US-Startszenario als aktive Startvorgabe durch Deutschland/Niederlande ersetzen. Szenarioauswahl und Datenversion aktualisieren, keine laufende Partie konvertieren.
- Alle Regionen mit genau einem Hub und einem B2B-Regionsmarkt initialisieren; vorhandene Konsumenten-/Ladenregeln beibehalten.
- Eigene und lokale Firmen, Assets, Rohstoffe, Konten und Arbeitskräfte auf neue Hubs verteilen. Historische Belege und Spielwerte unterscheiden.
- Minen/Farmen weiter im zugeordneten Umland; keine kostenlosen Vorkommen nur zur Vollversorgung jeder Region erzeugen.
- Mindestens zwei bestehende spielbare Firmen mit definiertem Startbesitz konfigurieren. Standort nicht mehr fest auf Detroit referenzieren.
- Aggregierte Bevölkerung, Beschäftigung und BIP genau einmal Land/Region zuordnen.
- Karten-/UI-Auswahl sowie Verträge, Kostenrechnung und Finanzdienste anhand von IDs anbinden. Keine neuen geografiespezifischen Wirtschaftsregeln duplizieren.

**Abnahme**
- Frische Partie startet mit gültiger Firmenwahl auf der neuen Karte.
- Jede Region hat einen Hub; Häfen und Flussknoten sind keine zusätzlichen Regionen.
- Konten/Waren werden nicht aus US- und Europa-Szenario gemeinsam doppelt geladen.
- Bestehender Handel und Stadtbildschirm funktionieren mit den neuen IDs.

### MAP04 – Schiffbares Netz und Hafenanschlüsse

**Voraussetzung:** MAP03, Schiffbarkeitsdaten aus MAP01.

**Aufgaben**
- Transportgraph mit expliziten Land-/Fluss-/Kanal-/Küstenkanten, Länge, Transportart, Verfügbarkeit, Kapazität sowie Zeit-/Kostenparametern.
- Erste nutzbare deutsche/niederländische Wasserroute anhand Quellen festlegen; keine pauschale Schiffbarkeit ganzer benannter Flüsse annehmen.
- Flussabschnitte und Anschlüsse topologisch verbinden. Kreuzende Linien sind ohne echten Knoten keine Verbindung.
- Hafen-/Umschlaganschlüsse an Hubs, Zugang und Landzubringer dokumentieren. Binnenhub ohne Zugang erhält keinen direkten Wassertransport.
- Ausländischer Abschnitt im Testgebiet zulässig über explizite Zugangsregeln; Zölle/Politik nur aus vorhandenen Systemen, nicht neu erfinden.
- Nicht simulierte Randgebiete nicht als kostenloser beliebiger Abkürzungsweg verwenden.
- Küsten-/Seeverbindungen im Datenmodell vorsehen; nur belegte und bewusst konfigurierte Testkanten aktivieren. Kein weltweiter Seehandel erforderlich.
- Netzdiagnose zeigt fehlende Verbindungen, unzulässige Verkehrsarten und unbelegte Abschnitte. Keine Laufzeit-Wegsuche über große Geometriemeshes.

**Abnahme**
- Ein definierter Hub-zu-Hub-Wasserweg existiert, ein Hub ohne Anschluss wird korrekt abgewiesen.
- Nicht schiffbarer Abschnitt und fehlender Umschlag verhindern unzulässige Route.
- Kantenlängen/-zeiten kommen aus konfigurierter geografischer/netzbezogener Grundlage, nicht Bildschirmabstand.
- Netz-ID und historische Verfügbarkeit sind reproduzierbar.

### MAP05 – Wassertransport im bestehenden Planungsmenü

**Voraussetzung:** MAP04 und bestehender Transport-/Reservierungsdienst.

**Aufgaben**
- Vorhandene Transportplanung um Wasserangebote und optional kombinierte Zubringer–Schiff–Weitertransport-Kette erweitern.
- Zunächst ein geeigneter Schiffstyp bzw. begrenzter Fremdanbieter als Testumfang. Schiffe als Assets nach vorhandener Eigentumslogik, nicht als kostenlose Visualisierung behandeln.
- Kapazität, Abfahrts-/Ankunftszeit, Umschlag, Wartezeit und Kosten vor Bestätigung zeigen. Günstigere Langstrecke darf durch Umschlag höhere Gesamtkosten erhalten.
- Fracht und Verpflichtungen je Teilstrecke eindeutig halten. Anschluss-, Terminal- und Zielkapazität entweder für geplante Zeitfenster sichern oder nur aktuell sicher planbare Kette anbieten; keine unrealistische permanente Fahrzeugbindung über Monate.
- Fehler/Storno vor Abfahrt atomar behandeln; während Fahrt definierter Fortsetzungs-/Wartezustand ohne Frachtteleportation.
- Fremdanbieter erhalten Zahlungen, eigene Fahrzeuge/Schiffe bleiben gebunden und stehen nach Einsatz am korrekten Ort. Rück-/Leerfahrten weiterhin kostenpflichtig.
- Routen auf Karte plausibel entlang Landverbindungen bzw. Gewässernetz darstellen. Wasserverkehr nicht quer über Land, Straßen nicht ohne konfigurierte Querung über offene Gewässer.
- Exemplarische Schiffe ohne Simulationsbesitz visualisieren. Ausblenden, Zoom und Zeitbeschleunigung verändern keine Ankunft.
- Aktuelle Saves um neue Verkehrsträger und Kettenschritte ergänzen; alte inkompatible Saves ablehnen.

**Abnahme**
- Lieferung über schiffbare Route mit korrektem Eigentum, Kosten, Fracht und Ankunft vollständig spielbar.
- Kombination aus Land/Wasser zeigt zusätzliche Umschlagkosten/-zeit; fehlender Zugang liefert verständliche Fehlermeldung.
- Save/Load derselben Version während Wasserfahrt liefert genau einmal.
- Kein Gratisgeld, doppelt gebuchtes Schiff oder versehentlich unbegrenzter Hafenbestand.

### MAP06 – Kartenbedienung, Welt-Erweiterbarkeit und Abnahme

**Voraussetzung:** MAP02–MAP05.

**Aufgaben**
- Layer Länder, Verwaltungsregionen, Versorgung/Preise und Verkehr mit bestehendem UI-Kapitel verbinden; Firmenbesitz bleibt Asset-/Netzbesitz.
- Länder-/Regionsgrenzen und Hubnamen nach Zoom unterschiedlich gewichten. Gewässer und schiffbare Netzabschnitte unterscheidbar darstellen.
- Stadtkopf nennt Region/Land und hebt die genaue Region auf Hover/Auswahl hervor.
- Schiffbarkeit/Hafenanschluss in verständlichen Tooltips zeigen; technische Datenquellen in Diagnose/Attribution statt Dauertext.
- Tests für Insel, Landgrenze, Grenzfluss, Hafenhub, Binnenhub, Netzunterbrechung und Kartenrand.
- Datenimport nach Ländern/Ausschnitten erweiterbar halten. Politische Hierarchie und Simulationsdaten unabhängig von Projektions-/Meshdetails.
- Für spätere Weltkarte Anforderungen an LOD, Datenkacheln, Projektion/Datumsgrenze und nicht sichtbare Simulation dokumentieren; jetzt keinen kompletten Globus/Streaming-Framework bauen.
- Bestehende Wirtschafts-, Finanz-, Handels- und UI-Funktionen auf neuem Szenario als Regression prüfen.

**Abnahme / Kartenmeilenstein**
- Deutschland/Niederlande nach dokumentiertem historischem Datenstand, ein wirtschaftlich begründeter Hub pro Region.
- Land-/Gewässerdarstellung, politische Auswahl und erlaubte Transportwege sind konsistent.
- Mindestens eine wirtschaftlich nutzbare Land- und Wasserlieferung ohne Debugeingriff möglich.
- Gleiche Wirtschaftsergebnisse mit aktivierter/deaktivierter Fahrzeugdarstellung.
- 1280×720 und 1920×1080 visuell geprüft, Leistungswerte und Daten-/Quellenlücken dokumentiert.
- Erweiterung weiterer Länder erfordert neue Daten und bewusste Szenariokonfiguration, keinen Umbau der Company-/Konsumentenengine.

## Später zu verfeinernde Erweiterungen

- Größere Konzernfusionen, Beteiligungen und detailliertere Insolvenz. Einfache lokale Übernahmen und Abwicklung bestehen seit AP11.3–11.4.
- Weitere Waren, Technikfortschritt und historisch geeignete Branchen.
- Weitere Länder, Verkehrsarten und größere Kartenausschnitte.
- Detaillierter Arbeitsmarkt, Qualität/Markenbindung und soziale Reaktionen.
- Vertiefte Banken-/Zentralbankpolitik und weitere Finanzierung.
- Zusätzliche politische Einflusswege und abstrakte Regelumgehung.

Diese Punkte sind keine Voraussetzung für die ersten geschlossenen Spieltests.

## Arbeitsregel für jeden einzelnen Auftrag

- Vollständige Leitlinien und beauftragtes Paket lesen, tatsächlichen Code und bestehende Daten prüfen.
- Abgeschlossene Systeme integrieren statt ersetzen; nur erforderliche Code-/Szenarioumstellungen vornehmen; keine Altspielstandmigration.
- Entwurfsentscheidungen, Änderungen und konfigurierbare Testwerte knapp dokumentieren.
- Save/Load, Entwicklerkonsole, Invarianten und Messpunkte für neue Zustände im selben Paket erweitern.
- Wirtschaftliche Buchungen gezielt prüfen, UI visuell testen. Keine behaupteten Builds/Tests, wenn die Umgebung sie nicht ausführen konnte.
- Nach jedem Paket liefern: umgesetzt, geprüft, Einschränkungen, offene Produktfragen und nächstes Paket.
