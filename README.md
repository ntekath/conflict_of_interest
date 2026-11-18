# Conflict of Interest

Konzeptioneller Neustart: US-Testgebiet, Spielbeginn 01.01.1913. Verbindlicher Entwurf:
[Entwicklungsauftrag](Docs_DevelopmentBrief.md). Umsetzung derzeit **AP01 bis AP06**. Aktive Karte: Großer-Seen-US-Ausschnitt.

![US-Testkarte](Docs/MapPreview.svg)

## Voraussetzungen und Start

- Unreal Engine **5.8.4** (vorhandene lokale Installation: `E:\UE_5.8`).
- Visual Studio 2022 C++ Toolchain und Windows SDK, wie von dieser UE-Version unterstützt.
- Kein zusätzlicher kostenpflichtiger Dienst oder Asset erforderlich.
- `ConflictOfInterest.uproject` öffnen und fehlende Module bauen lassen oder vorab:

```powershell
./Tools/Build.ps1 -EngineRoot E:\UE_5.8 -Target Editor
```

Die EngineAssociation verwendet die vorhandene lokale Engine-Registrierung. Auf einem anderen
Rechner im Explorer „Switch Unreal Engine version“ auf eine kompatible UE 5.8 umstellen.
`COI_ENGINE_ROOT` oder `-EngineRoot` konfiguriert das Buildskript ohne Änderungen am Quellcode.

Falls Windows keine `.uproject`-Dateizuordnung hat: Unreal Engine starten → Meine Projekte → Durchsuchen → `ConflictOfInterest.uproject` auswählen.

Standardlevel: `/Game/Maps/GreatLakes1913`, dann **Play**. Die vier orangefarbenen Marker stehen
für Chicago, Detroit, Cleveland und Pittsburgh. Spielregionen liegen ausschließlich in den USA;
Kanada dient als geografischer Kontext. Auswahl, Ortsinformationen und Kartenansichten: [AP03](Docs/AP03.md), [AP04](Docs/AP04.md).

Die generierten `.umap`/`.uasset`-Dateien sowie Roharchive, Szenario und Importer sind versioniert.
Zum Offline-Neuimport und zur genauen Editor-Reihenfolge: [AP02](Docs/AP02.md).
`CoI.RebuildMap` baut jetzt die US-Karte; `CameraPrototype` bleibt als AP01-Level erhalten.

WASD/Pfeile bewegen die Kamera, Mausrad zoomt zum Punkt unter dem Mauszeiger. Escape beendet
Play; falls der Fokus nicht im Viewport liegt, den roten Stop-Knopf verwenden. Shift+F1 gibt den
Mausfokus frei. Nord ist +X, Ost +Y; Kartenebene Z=0.

AP01-Fokusgrenzen: X/Y jeweils ±6500; Höhe 600–22000, Start 17000.
AP02 lädt Fokusgrenzen, Zentrum und Übersichtshöhe aus dem Geokatalog. Die Fokusgrenze hat am Rand
Vorrang vor dem Mausanker. Geschwindigkeit skaliert mit Zoom und DeltaSeconds; Diagonalen sind
normalisiert. Legacy Input ist vollständig per Config eingerichtet, ohne zusätzliche Input Assets.

## Aufbau und Versionsverwaltung

- `Source/ConflictOfInterest`: Kamera, Controller, GameMode und reine Kameramathematik.
- `Source/CoIEditor`: editorgebundener, wiederholbarer Aufbau des generierten Levels.
- `Config`: Startlevel, Input, Renderer und Cooking-Referenzen.
- `Content`: tatsächlich erzeugte `.umap`/`.uasset`-Dateien, bewusst in Git versioniert.
- `Docs`: Architektur, Paketabnahme und Einschränkungen.

Engine, Binaries, Intermediate, DerivedDataCache, Saved, IDE-Dateien und lokale Python-Umgebungen
gehören nicht ins Repository. Das alte Europa-Projekt `firstgame` liegt außerhalb dieses Repos und
wurde nicht verändert. Nur die grundlegende Kameratechnik wurde bewusst neu umgesetzt.

Der Simulationskern aus [AP04](Docs/AP04.md) läuft in festen Schritten und startet pausiert. Start/Pause sowie 1x/4x/16x steuern die Spielzeit. [AP05](Docs/AP05.md) aktiviert Produzenten, Beschäftigung/Löhne, regionale Orderbücher, Haushaltskonsum und bezahlte Transporte; kein wirtschaftlicher Actor-Tick.
Geografische Definitionen bleiben statische Daten. Das externe JSON-Startszenario, getrennte Laufzeitdaten, Konten, Reservierungen und Assets laufen unter `Source/ConflictOfInterest/Simulation`. Stadt/Region auswählen und die Diagnoseleiste scrollen; ohne Auswahl erscheint die globale Bilanz. Alle Wirtschafts- und Bevölkerungswerte sind fiktive Testwerte.

## Prüfung

UE-Konsole: `Automation RunTests CoI.`, bis „Test Queue Empty“ abwarten.
Weitere Schritte und tatsächlich ausgeführte Prüfungen: [AP01](Docs/AP01.md), [AP02](Docs/AP02.md), [AP03](Docs/AP03.md), [AP04](Docs/AP04.md).


Das [Spieler-HUD aus AP06](Docs/AP06.md) ersetzt das bisherige Diagnosepanel als Hauptoberfläche. Märkte, Lager, Transporte und Unternehmen verbinden Kauf → bezahlte Lieferung → Verkauf; Bestätigungen senden echte Simulationsbefehle. Diagnose bleibt separat schaltbar. Escape schließt zuerst den Dialog, dann die Arbeitsansicht. Bei pausierter Simulation werden gesendete Befehle erst nach Start umgesetzt. Bedienung, Testwerte und bekannte Grenzen stehen in der AP06-Dokumentation.

[AP06a](Docs/AP06a.md): Vor Spielbeginn kann PlayerCompanyId im Szenario ein bestehendes spielbares Handelshaus, einen Produzenten oder den Transportbetrieb übernehmen; Besitz und Geld bleiben reguläre Firmendaten.

[AP07.1](Docs/AP07.1.md): Versorgung und echte gewichtete Tagespreise als Kartenlayer, getrennte Haushalts-/Inputansicht, konfigurierbare Preisgrenzen und klare Zustände für fehlende Daten/Nullbedarf.

[AP07.2](Docs/AP07.2.md): gerichtete Routen, freigegebene öffentliche Aktivitätsaggregation und begrenzte Lkw-Beispiele aus veröffentlichter Spielzeit.

[AP07.3](Docs/AP07.3.md): Karte/Markt/Lager/Transporte verbinden, Planung vorbelegen und Objekte auf der Karte zentrieren; Tests und Messvergleich dokumentiert.

[AP07a](Docs/AP07a.md): lokale Entwicklerkonsole mit F10 (nur Editor/Development), sicheren Eingriffen, budgetiertem Stunden-Vorschub und JSONL-Journal. `help` listet Befehle; Shipping enthält keine Entwickler-Endpunkte.

[AP08.1](Docs/AP08.1.md): gemeinsame Stadthubs, getrennte Hauptsitze/Heimatorte und begrenzte physische Umlandquellen; Migration ohne zusätzliche Assets.

[AP08.2](Docs/AP08.2.md): Stadtbildschirm mit Übersicht, bestehenden lokalen Markt-/Lager-/Transportaktionen, eigenen Assets und transparent noch nicht ausführbaren Investitionen. Einstieg über „Stadt“ oder die Kartenauswahl.
