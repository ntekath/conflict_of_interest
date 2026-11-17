# AP06a – bestehende Unternehmen übernehmen

`PlayerCompanyId` in `Content/Simulation/scenario.coisim` wählt vor Start eines von drei bestehenden Unternehmen: `company.player` (Handelshaus Detroit, 25000 USD, Lager und ein Lkw), `company.food` (Nahrungsproduzent Chicago, 50000 USD, Betrieb/Lager, **kein Lkw**) oder `company.carrier` (Transportbetrieb, 50000 USD, zwei vorhandene Lkw). Name, Beschreibung, Startort, Playable und AccountingOnly stehen in den Firmendefinitionen. Externe Abrechnungskonten sind nicht spielbar. Der Generator enthält dieselben Stammdaten; eine dort geänderte Startauswahl wird reproduzierbar ins Szenario geschrieben.

Die vollständige wirtschaftliche Welt wird unabhängig von der Auswahl geladen. Auswahl kopiert weder Guthaben noch Besitz und erzeugt keine Firma. `FSession` hält die zentrale Laufzeitzuordnung (ScenarioId/PlayerCompanyId), getrennt von Definitionen und wirtschaftlichem Zustand. Nur das Startszenario gibt den Anfangswert vor. Alle Eigentumsfilter, Kontenansichten und Spielermeldungen lesen diesen zentralen Kontext. Ein laufender Wechsel ist nicht vorgesehen. Der Datentyp ist für AP09 serialisierbar; Speicherstände existieren noch nicht.

„player“ ist kein Unternehmenstyp mehr. Das bisherige Handelshaus verwendet ein reguläres Händlerprofil und wird als NPC aktiv, wenn ein anderes Unternehmen übernommen wird. Die gewählte Firma erzeugt keine autonomen Handels-/Leerfahrtentscheidungen. Produktion, Beschäftigung, Löhne, Fixkosten und bereits gebuchte Transporte bleiben aktiv. Die KI verplant keine freien Fahrzeuge der übernommenen Firma; neue automatische Annahme fremder Frachtaufträge im Namen der Spielerfirma ist nicht vorweggenommen.

Befehle tragen Herkunft und CompanyId. Die zentrale Queue und die Ausführung prüfen Spielerherkunft gegen die Session sowie KI-Herkunft gegen autonome Firmen; abweichende Nutzlast-ActorId wird abgewiesen. Der Spieler-Bridge setzt Herkunft und Firma selbst, statt Widget-Felder zu vertrauen. Die fachlichen Eigentums-/Deckungsprüfungen aus AP05 bleiben bestehen. Der System-Ursprung ist ausschließlich der interne Szenario-/Test-Harness und wird durch keine Spieleroberfläche angeboten. Interne autonome Planer werden ebenfalls am zentralen Firmenkontext abgegrenzt.

Ungültige, nicht existente oder nicht spielbare Auswahl scheitert an der vollständigen Szenariovalidierung vor Initialisierung. Keine Ersatzfirma, keine freie Flotte, kein Fallback auf Arrayposition. Ein fehlgeschlagener Ladeversuch verändert eine bereits vorhandene gültige Welt nicht; eine neue ungültige Instanz startet nicht.

Prüfung am 17.11.2026: Editor-Build und alle 17 Tests erfolgreich. `CoI.Control.ExistingCompanySelection` vergleicht die ökonomischen Fingerprints vor Schritt 1 bei Handelshaus/Produzent/Carrier; prüft Kontoperspektive, null Lkw beim Produzenten, vorhandene zwei Lkw beim Carrier, zentrale Berechtigungen, fehlende KI-Aufträge der gewählten Firma und weiterlaufende Produktion/Löhne. Die 16 bisherigen Tests einschließlich AP06-Handelsablauf und 28-Tage-Replay bleiben erfolgreich. Test-Fixtures erhielten eindeutige Namen, damit sie auch zusammen im UE-Unity-Build kompilieren.

Kein neues Auswahlmenü, Unternehmenswechsel, Investitions- oder Speichersystem. AP07.1 baut auf dieser zentralen Zuordnung auf.

Auch der Game-Build war erfolgreich. [Testprotokoll](Verification/AP06a_tests.txt).
