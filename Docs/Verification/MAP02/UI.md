# Sicht- und Bedienprüfung MAP02

20.11.2026, UE5.8.4, eigenständige Editor-Spielvorschau, D3D SM5. Tatsächliche Clientauflösungen 1280×720 und 1920×1080; Screenshots enthalten zusätzlich den Windows-Fensterrahmen.

- Übersicht: historischer deutscher/niederländischer Ausschnitt und alle 50 Hubmarker; größere Auflösung erlaubt mehr kollisionsfreie Namen. Inseln, Binnengewässer und die offene Zuiderzee sichtbar.
- Hubauswahl Berlin: originale WGS84-Koordinaten, Region Berlin, Deutsches Reich und physisch Land korrekt. Die Auswahl bleibt beim Wechsel Regionen→Länder bestehen.
- Länderansicht: Deutschland grün, Niederlande ocker, Nachbarkontext grau; ausgewähltes Land wird umrandet.
- Nordseeklick: keine politische Region, physisch Gewässer, keine Schiffbarkeitsfreigabe. Politische Zuordnung innerhalb von Seen zusätzlich automatisiert am Schweriner See geprüft.
- Scrollbares Panel: langer Hub-/Quellentext bleibt erreichbar, alle drei Schaltflächen passen auch bei eingeblendeter Scrollleiste. Mausrad über dem Panel scrollt Text; Hubpositionen/Kartenmaßstab bleiben dabei unverändert.
- Mausrad über der Karte vergrößert den Ausschnitt. Regionsnamen erscheinen in näheren Zoomstufen; kein TEST-Zusatz für historische Verwaltungsregionen. Labels vermeiden Überdeckung und das linke Panel.

Keine Wirtschaftsfreigabe für Europa und keine Netz-/Schiffbarkeitsprüfung in dieser Vorschau. Quellenunsicherheiten stehen im Kartenpanel und ausführlich in `Docs/MAP02.md`. Leistungsbenchmark und der abschließende integrierte Kartenmeilenstein gehören zu MAP06; hier werden keine gemessenen FPS behauptet.

Bekannte Darstellungsgrenze: Die bestehende Label-Platzprüfung schätzt Textbreiten. Bei langen Regionsnamen und dichter Beschriftung können Hintergründe knapp aneinanderliegen bzw. Buchstaben über den dunklen Hintergrund hinausragen. Die Lesbarkeits-/LOD-Verfeinerung bleibt MAP06; IDs, Auswahl und Geometrie sind davon unabhängig.
