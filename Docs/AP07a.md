# AP07a – lokale Entwicklerkonsole

Stand: 18.11.2026. Grundlage ist der vollständige Entwicklungsauftrag vom 17.11.2026 (Version 4). AP06a-Steuerungszuordnung und AP07.1–07.3 sind vorhanden; deren Regressionstests bestehen. Es wurden ausschließlich vorhandene lokale Werkzeuge und kostenlose Informationen verwendet.

## Bedienung

Editor-/Development-Spiel starten und F10 drücken. Eingabe erhält sofort Fokus; Enter führt aus, Pfeile hoch/runter navigieren die Historie, F10/Escape schließen. Die separate Oberfläche übernimmt Tastatur und Maus. Es gibt keinen zusätzlichen Knopf im Spieler-HUD. `help` zeigt alle Syntaxformen und Einheiten. `list companies`, `list warehouses`, `list inventories`, `list vehicles`, `list orders`, `list regions`, `list goods` und `list places` finden IDs; optionaler Suchtext filtert Namen/IDs (höchstens 50 Treffer).

Beispiele im unveränderten Startszenario:

```text
inspect company company.food
money add company.food 12345
money set company.player 2500000
goods add company.player storage.player good.food 10
vehicle relocate vehicle.player city.us.chicago
validate world
step 1
advance 24
status
cancel
repro
config
export log
restart scenario.us.great_lakes.1913 42
```

Geld ist in ganzen USD-Cent, Waren in ganzen kg, Zeit in ganzen Spielstunden angegeben. `money add` erlaubt auch eine explizite negative Geldänderung oberhalb bestehender Reservierungen. Jede Firma wird ausdrücklich adressiert, auch NPCs. `inspect` unterstützt company/inventory/warehouse/vehicle/order/transport/region. Unternehmensdiagnose zeigt Besitz, Aufträge, Transporte, Anfangskapital und bis zu fünf jüngste Zahlungen; Entwicklerherkunft steht im Journal.

## Regeln und Konfiguration

`Config/DefaultGame.ini`, Abschnitt `[CoIDeveloperConsole]`:

| Schlüssel | Standard | Zugelassener Konfigurationsbereich |
|---|---:|---|
| ToggleKey | F10 | gültige UE-Taste, sonst F10 |
| OutputLines | 160 | 40–1000 |
| HistoryLines | 64 | 1–256 |
| AuditRecords | 512 | 64–4096 |
| MaxAdvanceHours | 8760 | 1–87600 |
| MaxStepsPerFrame | 4 | 1–64 |
| BudgetMicros | 2000 | 100–10000 |

Vorschub verwendet unverändert `StepOnce`, höchstens vier vollständige Stunden pro Frame und ein weiches 2-ms-Budget (ein bereits begonnener Schritt wird abgeschlossen). Keine vergrößerten Simulationsschritte. Das vorhandene Szenario verwendet HoursPerStep=1; andere Stundenraster werden für Entwickler-Vorschub ausdrücklich abgewiesen. Vorschub endet pausiert. Währenddessen werden weitere Mutationen und normale Spieleraufträge abgewiesen; Diagnose und `cancel` bleiben verfügbar. Abbruch erfolgt zwischen Schritten. `pause` hält die aktuelle abgeschlossene Grenze; laufenden Vorschub mit `cancel` beenden.

Befehle sind zentral registriert und typisiert. Maximal 512 Zeichen; feste Parameterzahlen, keine Skripte, Shell, Dateipfade oder Fernverwaltung. Geldwerte sind auf 10^12 Cent pro Konto/Änderung und kumulative Entwicklerquelle/-senke begrenzt. Warenzugaben erlauben 1–100000000 kg, maximal 10^9 kg Bestand, stets innerhalb freier Lagerkapazität einschließlich reservierter Anlieferungen. Zugang besteht bei eigenem Lager, öffentlichem Dienstleisterlager oder nachweisbarem vorhandenem Bestand/offenem Kaufauftrag des Eigentümers dort; Rückstände sperren Zugaben. Gebundene Fahrzeuge, Transporte und aktive Fahrzeugreservierungen verhindern Versetzung.

Eingriffe laufen durch eine eigene autoritative Entwicklerschnittstelle. Vorherige Weltvalidierung, Zustandskopie, Eingriff und erneute Validierung sichern atomare Änderungen; bei Fehler vollständiger Rollback. Erfolgreiche Eingriffe erhöhen Snapshot-Version und aggregieren/veröffentlichen sofort, ohne die Wirtschaftsstunde zu erhöhen. HUD, Karte und Tabellen lesen dieselbe Präsentationsversion. Normale Spielerberechtigungen bleiben zentral erhalten.

Geldquelle/-senke und DeveloperAddedKg werden separat bilanziert; DeveloperModified kennzeichnet den Zustand. Handel, Löhne, Produktion und Transportregeln bleiben unverändert. Neue Szenarien dürfen keine voreingetragenen Entwicklerzugaben enthalten. Normale Partien erzeugen keine Zugaben.

## Journal und Reproduktion

Jede Sitzung erhält `Saved/Developer/<Session-ID>/`. `ConsoleJournal.jsonl` enthält auch abgewiesene Befehle: sequence, generation, step, command, accepted, phase, before, after und message. Ein Vorschub hat korrelierte Start-/Abschlussdatensätze; Abbruch erhält einen eigenen Datensatz. Der Arbeitsspeicher hält nur die letzten AuditRecords, die Datei erhält alle Datensätze. `export log` kopiert das bisherige Journal nach ConsoleExport.jsonl; die Exportbestätigung selbst steht erst im fortlaufenden Journal.

`Configuration_<Generation>.json` enthält die vollständige Szenariokonfiguration einschließlich Seed, Definitionen, Wirtschaftsparametern und Anfangszustand. `repro` nennt Szenario-ID, Schema, Seed, Generation, Schritt und Konfigurationshash. `restart` akzeptiert nur das fest konfigurierte Szenario mit Seed 0–2147483647; keine Änderung des laufenden Seeds. Erfolgreicher Neustart verwirft alte Jobs, Befehle und Historie und startet pausiert. Das Journal bleibt zur Nachvollziehbarkeit erhalten. Schreibfehler werden im Entwicklungslog gemeldet und verhindern einen als erfolgreich gemeldeten Export. Dies ist kein vollständiges automatisches Replay und noch kein AP09-Spielstandformat.

Die bestehende Simulation läuft seriell. InStep-Grenzprüfung, Generationstoken, World-End/Deinitialize-Reset und schwache UI-Referenzen verhindern alte Zugriffe. Es wurden keine neuen Worker eingeführt; echte künftige parallele Worker erfordern die AP10-Erweiterung.

## Abnahme und Belege

Am 18.11.2026 erneut ausgeführt: Editor-Build erfolgreich und alle 23 CoI-Automationstests bestanden, einschließlich vierwöchiger Bilanz-/Replayprüfung. `CoI.Developer.AtomicMutationsAndAudit` prüft NPC-/Spielergeld, Warenquelle, unmittelbaren Snapshot, keine fingierte Stunde, Reservierungsuntergrenze, Kapazität/Zugriff/IDs, negative Zeit, gebundenes Fahrzeug, atomare Ablehnungen und Journal. `CoI.Developer.AdvanceReplayAndLifecycle` vergleicht advance 24 gegen 24 normale Stunden per Zustandsfingerprint, step 1, Framebudget, Sperre weiterer Mutationen, Abbruch, Generationswechsel, Shutdown und begrenzte Historie. Beleg: `Verification/AP07a_tests.txt`.

Native Development-Oberfläche bei 1280×720 geprüft: F10 mit automatischem Eingabefokus; Enter, Historie und Escape; Geldänderung bei Schritt 0 im HUD sichtbar (25000,00 → 25123,45 USD), unveränderte Kartenkamera; advance 24 mit genau einem Tageswechsel und pausiertem Ende. Abschließend Neustart mit Seed 42/Generation 3, advance 8760 bei Stunde 8039 abgebrochen, Weltvalidierung erfolgreich, danach weiterhin pausiert. `event list` meldet ausdrücklich „nicht implementiert (AP07b)“. Logexport erzeugt gültige JSONL-Datei und Konfigurationsdateien. Kleiner reproduzierbarer Beleg: `Verification/AP07a_console_sample.jsonl`. Eine gesonderte Full-HD-Abnahme wurde nicht durchgeführt.

Development-Game und tatsächlicher Win64-Shipping-Build erfolgreich gebaut. `Tools/VerifyDeveloperShipping.py` prüft positive Marker im Editor-Development-Modul gegen deren Abwesenheit im Shipping-Executable (UTF-8 und UTF-16), einschließlich Entfernung des alten mutierenden Exec SimRestart. Zusätzlich erzwingt ein im Shipping übersetztes static_assert die Compile-Grenze. UI, Controller-Endpunkte, Bridge und Service liegen vollständig unter COI_WITH_DEV_CONSOLE = !UE_BUILD_SHIPPING && !UE_BUILD_TEST. Beleg: `Verification/AP07a_shipping.json`. Dies ist ein Build-/Binärtest; ein paketierter Shipping-Lauf wurde nicht behauptet und gehört nicht zu dieser Abnahme.

Wiederholen:

```powershell
./Tools/Build.ps1 -Target Editor -EngineRoot E:\UE_5.8
./Tools/Test.ps1 -EngineRoot E:\UE_5.8
./Tools/Build.ps1 -Target Game -EngineRoot E:\UE_5.8
./Tools/Build.ps1 -Target Game -Configuration Shipping -EngineRoot E:\UE_5.8
python Tools/VerifyDeveloperShipping.py
```

Ereignisbefehle sind vorbereitet und lehnen ohne Änderung ab; Dürrelogik bleibt AP07b. Persistentes Speichern/Laden bleibt AP09. Der bestehende gemeinsame Präsentationsweg und die 21 bisherigen Tests bleiben erhalten.
