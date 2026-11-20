# Projektregeln

## Speicherstände während der Entwicklung

Ausdrückliche Vorgabe des Projektinhabers vom 20.11.2026:
- Während der aktiven Entwicklung werden Spielstände **nicht auf neue Versionen migriert**.
- Keine Abwärtskompatibilität, Migrationspfade oder Migrationstests für alte Save-Versionen neu bauen oder pflegen.
- Bei inkompatiblen Änderungen am persistierten Zustand oder seiner Bedeutung die aktuelle Save-/Simulations-/Definitionsversion anheben; alte Versionen verständlich ablehnen und eine neue Partie verlangen.
- Save/Load und identische Fortsetzung innerhalb der aktuellen Version weiterhin prüfen. Beschädigte Dateien dürfen den laufenden Zustand nicht verändern.
- Szenario-Testfixtures aus älteren Entwicklungsständen sind keine Save-Migration und dürfen für normale Regressionstests bestehen bleiben.

Diese neuere Nutzerentscheidung ersetzt während der Entwicklung die Migrationsanforderungen in älteren Arbeitsaufträgen und Paketdokumentationen. Erst auf ausdrücklichen neuen Auftrag wieder Save-Migration unterstützen.
