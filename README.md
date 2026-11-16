# Conflict of Interest

Konzeptioneller Neustart: US-Testgebiet, Spielbeginn 01.01.1913. Verbindlicher Entwurf:
[Entwicklungsauftrag](Docs_DevelopmentBrief.md). Umsetzung derzeit **AP01 bis AP03**. Aktive Karte: Großer-Seen-US-Ausschnitt.

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
Kanada dient als geografischer Kontext. Auswahl, Ortsinformationen und Kartenansichten: [AP03](Docs/AP03.md).

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

Der Simulationskern ist noch nicht implementiert. Kein wirtschaftlicher Actor-Tick.
Die geografischen Definitionen (einschließlich vier Städten) sind statische Daten, kein Spielzustand. Auch spätere Entitäten werden hier nicht als
leere Framework-Klassen vorweggenommen.

## Prüfung

UE-Konsole: `Automation RunTests CoI.`, bis „Test Queue Empty“ abwarten.
Weitere Schritte und tatsächlich ausgeführte Prüfungen: [AP01](Docs/AP01.md), [AP02](Docs/AP02.md), [AP03](Docs/AP03.md).
