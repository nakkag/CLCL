CLCL Ver 2.2.0
--

* Einleitung
CLCL ist ein Programm, das einen Verlauf der Zwischenablage führt.

* Funktionen
- Es werden mehrere Zwischenablage-Formate unterstützt.
- Häufig verwendeter Text kann hierarchisch als Vorlage gespeichert werden.
- Das Menü, das über den Hotkey angezeigt wird, kann frei angepasst werden.
- Verlaufs- und Vorlageneinträge, die im Menü ausgewählt werden, werden
  automatisch in den Editor eingefügt, in dem Sie gerade arbeiten.
- Bilder werden im Menü als Miniaturansichten angezeigt.
- Im Menü werden Tooltips angezeigt.
- Es kann eingestellt werden, welche Formate in den Verlauf aufgenommen und
  welche gespeichert werden.
- Es kann eingestellt werden, für welche Fenster der Verlauf geführt wird
  und welche ignoriert werden.
- Die Einfüge-Taste kann für jedes Fenster festgelegt werden.
- Der Verlauf wird beim Beenden automatisch gespeichert und beim nächsten
  Start wiederhergestellt.
- Die Anzahl der Einträge im Verlauf ist nicht begrenzt.
- Verlauf und Vorlagen können in einer Explorer-ähnlichen Anzeige
  dargestellt und bearbeitet werden.
- Die Funktionen können durch Plug-ins erweitert werden.
- Unicode-Unterstützung

* Installation
CLCL läuft unter Windows 7 und neuer.

Durch Ausführen der heruntergeladenen Datei wird CLCL installiert.
Deinstalliert wird CLCL über die Systemsteuerung.
Beenden Sie CLCL, bevor Sie es deinstallieren.

Die Daten werden im folgenden Ordner gespeichert (unter Windows 10):
  C:\Users\(Benutzername)\AppData\Local\CLCL

Um die Daten am selben Ort wie CLCL.exe zu speichern, richten Sie
clcl_app.ini wie folgt ein und starten Sie dann CLCL.

[GENERAL]
portable=1

* Start
Wenn CLCL gestartet wird, erscheint ein Klammer-Symbol im Infobereich der
Taskleiste (dem Bereich, in dem die Uhr angezeigt wird).
Ein Klick auf dieses Symbol zeigt das Menü an.
Standardmäßig wird der Verlauf im Menü in aufsteigender Reihenfolge
angezeigt.
Das Menü kann in den Einstellungen angepasst werden.

Ein Rechtsklick auf das Symbol im Infobereich zeigt die Anzeige.
Auf der linken Seite der Anzeige befindet sich eine Baumansicht mit dem
Verlauf und den Vorlagen.
Auf der rechten Seite der Anzeige wird der Inhalt eines Verlaufs- oder
Vorlageneintrags dargestellt und bearbeitet. Was Sie bearbeiten, wird auf
den Eintrag angewendet, sobald der Fokus wechselt. Manche Formate können
nicht bearbeitet werden. Der aktuelle Inhalt der Zwischenablage kann nicht
bearbeitet werden.

"Zwischenablage" am oberen Ende der Baumansicht ist der aktuelle Inhalt der
Zwischenablage.
"Verlauf" in der Baumansicht ist die Liste des Verlaufs.
"Vorlagen" in der Baumansicht ist die Liste der gespeicherten Einträge
(Vorlagentexte und so weiter).

    +--[+] Zwischenablage   - Inhalt der aktuellen Zwischenablage
    |   +--- TEXT           - Format in der Zwischenablage
    |   +--- LOCALE
    |   +--- OEM TEXT
    |
    +--[+] Verlauf          - Verlauf der Zwischenablage
    |   +--- (BITMAP)       - Verlaufseintrag
    |   |   +--- BITMAP     - Format im Verlaufseintrag
    |   |   +--- DIB
    |   |
    |   +--- Hallo...
    |   |   +--- TEXT
    |   |
    |   +--- Guten Morgen...
    |   +--- (BITMAP)
    |
    +--[+] Vorlagen         - Gespeicherte Einträge
        |
        +--[+] Ordner       - Ordner
        |   +--- Adresse...
        |   +--- (BITMAP)
        |
        +--- http://www...  - Vorlageneintrag
            +--- TEXT       - Format im Vorlageneintrag

* Zwischenablage
- Was ist die Zwischenablage?
	Die Zwischenablage ist ein Bereich, über den Informationen
	zwischen verschiedenen Anwendungen ausgetauscht werden.
	Sie können zum Beispiel Text, den Sie im Editor kopiert haben, in
	Word einfügen; das funktioniert, weil dabei der Bereich namens
	Zwischenablage verwendet wird.

- Zwischenablage-Formate
	Die Zwischenablage kann mehrere Formate gleichzeitig enthalten.
	Wenn Sie zum Beispiel Text im Editor kopieren, werden die
	folgenden vier Formate in der Zwischenablage abgelegt
	(unter Windows 10):
		- UNICODE TEXT
		- LOCALE
		- TEXT
		- OEM TEXT
	Beim Kopieren in Excel oder Access werden noch viel mehr Formate
	an die Zwischenablage gesendet.

	Standardmäßig nimmt CLCL die folgenden Formate in den Verlauf auf:
		- UNICODE TEXT		- Text
		- BITMAP		- Bitmap
		- DROP FILE LIST	- Dateien
	Andere Formate können über "Filter" in den Einstellungen ebenfalls
	in den Verlauf aufgenommen werden.

* Verlauf
Dies ist der Verlauf der Daten, die in die Zwischenablage kopiert wurden.
Neu kopierte Daten werden am Anfang des Verlaufs eingefügt.

Ein Verlaufseintrag enthält mehrere Zwischenablage-Formate. Von den unter
"Format" in den Einstellungen registrierten Formaten wird das Format mit
der höchsten Priorität im Menü und in der Anzeige dargestellt.

Der Verlauf behält so viele Einträge, wie unter "Verlauf" in den
Einstellungen festgelegt ist.
Nur die Zwischenablage-Formate, die unter "Filter" in den Einstellungen auf
"In Verlauf aufnehmen" gesetzt sind, werden in den Verlauf aufgenommen.

* Vorlagen
Häufig verwendete Daten wie Vorlagentexte können in den Vorlagen
gespeichert werden.
Sie können Ordner hinzufügen, um eine Hierarchie aufzubauen, und Sie können
den Einträgen Namen geben.

Um einen Vorlageneintrag hinzuzufügen, öffnen Sie die Anzeige, wählen einen
Eintrag im Verlauf aus und wählen "Zu Vorlagen hinzufügen" im Menü.
Wenn Sie in der Baumansicht den Ordner auswählen, zu dem Sie etwas
hinzufügen möchten, und "Neu" im Menü wählen, können Sie einen leeren
Eintrag erstellen oder einen Eintrag erstellen, dessen Inhalt aus einer
Datei geladen wird.

Um einen Ordner hinzuzufügen, öffnen Sie die Anzeige, öffnen an der Stelle
in den Vorlagen, an der Sie ihn hinzufügen möchten, das Kontextmenü und
wählen "Neuer Ordner".

Um einen Ordner oder einen Eintrag umzubenennen, öffnen Sie die Anzeige,
wählen den zu ändernden Eintrag aus, öffnen das Kontextmenü und wählen
"Umbenennen".
"Name zurücksetzen" löscht den von Ihnen vergebenen Namen, sodass der
Inhalt des Eintrags als sein Name angezeigt wird.
Wenn Sie einen Eintrag "-" nennen, wird er im Menü als Trennlinie
angezeigt. Die Formate und die Daten des Eintrags werden dabei ignoriert.
Wenn Sie & in einen Namen schreiben, wird das folgende Zeichen zur
Zugriffstaste im Menü. Um & selbst im Menü anzuzeigen, schreiben Sie &&.

Klicken Sie mit der rechten Maustaste auf einen Vorlageneintrag und wählen
Sie "Hotkey setzen", um dem Vorlageneintrag einen Hotkey zuzuweisen. Beim
Drücken dieser Taste wird der Vorlageneintrag direkt an die Zwischenablage
gesendet, ohne dass das Menü angezeigt wird, und direkt eingefügt, wenn
"Einfügen" aktiviert ist.
Die zugewiesenen Hotkeys können in der Listenansicht der Anzeige überprüft
werden. Sie werden außerdem in der Statusleiste angezeigt, wenn ein
Vorlageneintrag ausgewählt ist.

Die Anzahl der Vorlageneinträge und ihrer Zwischenablage-Formate ist nicht
begrenzt.

* An die Zwischenablage senden
Es gibt mehrere Möglichkeiten, einen Verlaufs- oder Vorlageneintrag an die
Zwischenablage zu senden.
- Klicken Sie auf das Symbol im Infobereich, um das Menü anzuzeigen.
  Durch die Auswahl eines Verlaufs- oder Vorlageneintrags werden die Daten
  an die Zwischenablage gesendet und automatisch in das aktive Fenster
  eingefügt.

- Drücken Sie den Hotkey (standardmäßig Alt + C), um das Menü anzuzeigen.
  Durch die Auswahl eines Verlaufs- oder Vorlageneintrags werden die Daten
  an die Zwischenablage gesendet und automatisch in das aktive Fenster
  eingefügt.

- Wählen Sie einen Eintrag in der Anzeige aus und öffnen Sie das
  Kontextmenü.
  Mit "An Zwischenablage senden" wird der ausgewählte Eintrag an die
  Zwischenablage gesendet.

* Menü
Die Einträge des Menüs, das über den Infobereich oder den Hotkey angezeigt
wird, werden unter "Aktion" in den Einstellungen festgelegt.
Das Verhalten und das Aussehen des Menüs werden unter "Menü" in den
Einstellungen festgelegt.

Wenn Sie die Maus über einen Verlaufs- oder Vorlageneintrag im Menü
bewegen, wird der ausführliche Inhalt in einem Tooltip an der Mausposition
angezeigt. Wenn Sie einen Eintrag mit der Tastatur auswählen, wird der
Tooltip unterhalb des Menüeintrags angezeigt.

Ein Rechtsklick auf einen Verlaufs- oder Vorlageneintrag im Menü zeigt die
registrierten Tools als Menü an; das ausgewählte Tool wird auf den Eintrag
angewendet und das Ergebnis an die Zwischenablage gesendet.
Um das Tool-Menü mit der Tastatur anzuzeigen, wählen Sie den Eintrag mit
der Eingabetaste aus, während Sie Strg gedrückt halten.

Verlaufs- und Vorlageneinträge werden im Menü gemäß "Anzeigeformat des
Menüs" in den Einstellungen dargestellt. Die angezeigten Nummern beginnen
bei 0; um den Startwert zu ändern, schreiben Sie die Startnummer zwischen
% und das Zeichen.
    Beispiel)
         %0d -> 0,1,2,3...
         %8x -> 8,9,a,b...
         %1n -> 1,2,3...8,9,0,1,2...
         %10B -> K,L,M,N...

* Aktion
Die Aktion, die beim Drücken eines Hotkeys ausgeführt wird, und die Aktion,
die beim Klicken auf das Symbol im Infobereich ausgeführt wird, werden
unter "Aktion" in den Einstellungen festgelegt.

Wenn Sie unter "Aktion bearbeiten" als Aktion "Menü" angeben, legen Sie die
anzuzeigenden Menüeinträge in den Menüeinstellungen im unteren Teil des
Dialogs fest.

"Aufrufart" legt fest, wie die angegebene Aktion aufgerufen wird.
Wenn Sie "Hotkey" angeben, legen Sie die Taste fest, die sie aufruft.
"Ctrl + Ctrl", "Shift + Shift" und "Alt + Alt" rufen die angegebene Aktion
auf, wenn die Taste zweimal gedrückt wird.

Wenn die Aktion ein Menü ist, kann "Einfügen" festgelegt werden.
Mit "Einfügen" wird bei der Auswahl eines Menüeintrags automatisch ein
Einfüge-Vorgang an die Anwendung gesendet, in der Sie gerade arbeiten.
Wenn Sie bei der Auswahl eines Menüeintrags die Umschalttaste gedrückt
halten, werden die Daten nur an die Zwischenablage gesendet und nicht
eingefügt.

Wenn die Aktion ein Menü ist und die Aufrufart "Hotkey", "Ctrl + Ctrl",
"Shift + Shift" oder "Alt + Alt" lautet, kann "An Einfügepunkt anzeigen"
festgelegt werden.
Mit "An Einfügepunkt anzeigen" wird das Menü an der Position der
Einfügemarke im Editor angezeigt. Ist die Option nicht gesetzt, wird das
Menü an der Mausposition angezeigt.

Wenn die Aktion ein Menü ist und der Inhalt der Verlauf ist, kann der
Anzeigebereich festgelegt werden. Der Anzeigebereich wird von 1 bis zur
maximalen Anzahl der im Verlauf behaltenen Einträge angegeben. Die Angabe
von 0 als Startnummer bedeutet dasselbe wie die Angabe von 1, und die
Angabe von 0 als Endnummer bedeutet dasselbe wie die Angabe der maximalen
Anzahl der im Verlauf behaltenen Einträge.
Ist die Endnummer kleiner als die Startnummer, wird nichts angezeigt. Ist
die Endnummer größer als die maximale Anzahl der im Verlauf behaltenen
Einträge, werden die Einträge bis zu diesem Maximum angezeigt.

* Zwischenablage-Format
CLCL kann mit jedem Zwischenablage-Format umgehen, aber ein nicht
registriertes Zwischenablage-Format wird in der Anzeige als Binärdaten
dargestellt.

Zwischenablage-Formate werden unter "Format" in den Einstellungen
registriert. Weiter oben in der Liste registrierte Formate haben Vorrang,
und das Zwischenablage-Format mit der höchsten Priorität in einem Eintrag
wird im Menü und in der Anzeige dargestellt.

Um ein Format zu registrieren, legen Sie den Formatnamen, die DLL, die es
verarbeitet, und den Funktions-Header fest. Wenn Sie die DLL leer lassen
und die Schaltfläche zur Auswahl des Funktions-Headers drücken, wird die
Liste der eingebauten Funktions-Header angezeigt.
Um zum Beispiel von den Zwischenablage-Formaten, die beim Kopieren in Excel
entstehen, CSV als Text zu verarbeiten, legen Sie Folgendes fest:
	Formatname: CSV
	DLL:
	Funktions-Header: text_
Danach wird es im Menü und in der Anzeige als Text verarbeitet.

* Filter
Die Zwischenablage-Formate, die in den Verlauf aufgenommen werden, werden
unter "Filter" in den Einstellungen festgelegt.

Wenn Sie "Alle Formate zum Verlauf hinzufügen" wählen, wird jedes
Zwischenablage-Format außer den auf "Ignorieren" gesetzten in den Verlauf
aufgenommen.
Wenn Sie "Alle Formate vom Verlauf ausschließen" wählen, werden nur die
Zwischenablage-Formate in den Verlauf aufgenommen, die auf "In Verlauf
aufnehmen" gesetzt sind.

Für ein Zwischenablage-Format, das im Filter auf "In Verlauf aufnehmen"
gesetzt ist, kann außerdem ein Größenlimit für die Aufnahme in den Verlauf
festgelegt werden. Daten, die größer als dieses Limit sind, werden nicht in
den Verlauf aufgenommen.

Für ein Zwischenablage-Format, das im Filter auf "In Verlauf aufnehmen"
gesetzt ist, verhindert die Einstellung "Nicht speichern", dass es beim
Beenden von CLCL in einer Datei gespeichert wird.
Sie können CLCL zum Beispiel so einrichten, dass sowohl Text als auch
Bitmaps in den Verlauf aufgenommen, aber nur der Text gespeichert wird.

* Fenster
Um das Verhalten von CLCL abhängig von der verwendeten Anwendung zu ändern,
legen Sie das Fenster und das Verhalten unter "Fenster" in den
Einstellungen fest.

Geben Sie den Titel und den Klassennamen des Fensters an; "*" kann als
Platzhalter für beliebige Zeichen verwendet werden.
Zum Beispiel für den Editor:
	Titel: * - Editor
	Klassenname: Notepad
Mit dieser Einstellung ändert sich das Verhalten von CLCL, solange der
Editor aktiv ist.
Es muss nur eines von beiden, Titel oder Klassenname, eingegeben werden;
eines davon leer zu lassen, bedeutet dasselbe wie die Angabe von nur "*".

- Fenster ignorieren
	Daten, die im angegebenen Fenster kopiert werden, werden nicht in
	den Verlauf aufgenommen.
	Wenn eine Anwendung nicht korrekt arbeitet, sobald ihre Daten in
	den Verlauf gelangen, geben Sie diese Option an, damit Kopien aus
	dieser Anwendung ignoriert werden.

- Fokus nicht setzen
	Der Fokus wird nicht gesetzt, nachdem das angegebene Fenster
	aktiviert wurde.
	Wenn der Fokus an eine andere Stelle wandert, während ein
	ausgewählter Menüeintrag eingefügt wird, und das Einfügen deshalb
	nicht korrekt funktioniert, kann diese Option Abhilfe schaffen.

- Einfügen bei Abbruch des Tools
	Normalerweise wird das anschließende Einfügen nicht ausgeführt,
	wenn Sie ein abbrechbares Tool abbrechen; mit dieser Option wird
	das Einfügen auch dann ausgeführt, wenn Sie abbrechen.
	Wenn Sie in den Tasteneinstellungen für die einzelnen Fenster die
	Kopier-Taste auf die Ausschneiden-Taste setzen, verhindert diese
	Option, dass die Zeichen verloren gehen, wenn Sie das Tool
	abbrechen.

* Tasteneinstellungen für die einzelnen Fenster
Wenn Sie über einen Hotkey einen Verlaufs- oder Vorlageneintrag auswählen
und dieser automatisch eingefügt wird, wird eine Einfüge-Taste an das
Fenster gesendet.
Standardmäßig wird an jedes Fenster Strg + V gesendet, aber in manchen
Fenstern kann die Einfüge-Taste eine andere sein.

Wenn ein Tool über einen Hotkey aufgerufen wird, führt CLCL Kopieren ->
Tool-Verarbeitung -> Einfügen aus, sodass die Kopier-Taste (Strg + C) an
das Fenster gesendet wird.

Die Kopier- und Einfüge-Tasten für die einzelnen Fenster werden unter
"Tasten" in den Einstellungen festgelegt.
Legen Sie den Titel und den Klassennamen des einzurichtenden Fensters fest
und legen Sie die Kopier- und Einfüge-Tasten fest.

Sind die Kopier- und Einfüge-Tasten nicht festgelegt, werden die
Standard-Tasteneinstellungen verwendet.

Für ein Fenster können mehrere Tasten festgelegt werden. Sind mehrere
festgelegt, werden die Tasten der Reihe nach von oben in der Liste
gesendet.

* Tool (Plug-in)
Um die Daten von Verlaufs- oder Vorlageneinträgen zu verarbeiten oder die
Funktionen von CLCL zu erweitern, richten Sie diese unter "Tool" in den
Einstellungen ein.

Wenn Sie eine DLL und einen Funktionsnamen auswählen, werden der Tool-Name
und der Ausführungszeitpunkt automatisch gesetzt.
"Aktionsmenü" beim Ausführungszeitpunkt macht das Tool über das Menü
verfügbar, das unter "Aktion" in den Einstellungen festgelegt ist.
"Menü im Betrachter" beim Ausführungszeitpunkt macht das Tool über das
Tools-Menü der Anzeige verfügbar.

"Kopier- und Einfüge-Aktionen senden" beim Ausführungszeitpunkt sendet
einen Kopier-Vorgang an das aktive Fenster, wendet das Tool auf die
kopierten Daten an und fügt das Ergebnis in das aktive Fenster ein.
Ist diese Option nicht aktiviert, wird das Tool auf den neuesten
Verlaufseintrag angewendet und das Ergebnis an die Zwischenablage gesendet.
Im Tool-Menü, das beim Rechtsklick auf einen Eintrag im Aktionsmenü
erscheint, wird das Tool auf den ausgewählten Eintrag angewendet und das
Ergebnis an die Zwischenablage gesendet.
Ist in den Aktionseinstellungen "Einfügen" nicht aktiviert, wird nach dem
Kopieren und Ausführen des Tools nicht eingefügt.

Wenn Sie eine DLL auf das Fenster der Tool-Liste ziehen und dort ablegen,
wird die Liste der Tools angezeigt, die registriert werden können, und Sie
können mehrere davon auswählen und auf einmal registrieren.

* Befehlszeile
Sie können beim Start von CLCL eine Befehlszeile angeben, um festzulegen,
was nach dem Start geschehen soll.
Läuft CLCL bereits, wird der Befehl an das laufende CLCL gesendet.

[Format]
CLCL.exe [/vwnx]
	/v Anzeige öffnen
	/w Zwischenablage-Überwachung einschalten
	/n Zwischenablage-Überwachung ausschalten
	/x Beenden

* Änderungsverlauf

- Ver 2.1.3 -> Ver 2.2.0
	- Unterstützung für den dunklen Modus von Windows hinzugefügt.
	- Unterstützung für Bildschirme mit hoher DPI-Auflösung
	  verbessert.
	- Option hinzugefügt, das Menü anzuzeigen, ohne dem Fenster, in
	  dem Sie gerade arbeiten, den Fokus zu entziehen.
	- Speichervorgang beim Beenden verbessert.
	- UNICODE-Unterstützung zur Binärdaten-Anzeige hinzugefügt.
	- CLCL so korrigiert, dass Daten, die ein Passwort-Manager in der
	  Zwischenablage ablegt, nicht im Verlauf behalten werden.
	  (kashima-eyetech)
	- Formulierungen der englischen Version verbessert.
	- Sicherheitsrelevante Probleme behoben.

- Ver 2.1.2 -> Ver 2.1.3
	- Symbol im Infobereich geändert, das angezeigt wird, während die
	  Zwischenablage nicht überwacht wird.
	- Schaltflächen zum Verschieben nach oben und unten in den
	  Einstellungen verbessert.
	- CLCL so verbessert, dass das Hauptfenster nicht angezeigt wird,
	  während das Menü dargestellt wird.

--

Der Autor übernimmt keine Verantwortung für Probleme, die durch dieses
Programm verursacht werden.
Es wird dringend empfohlen, eine Sicherungskopie wichtiger Dateien
aufzubewahren.

Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
	https://www.nakka.com/

2026/8/19
