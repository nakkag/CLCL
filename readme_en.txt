CLCL Ver 2.1.4
--

* Introduction
CLCL is software that keeps a history of the clipboard.

* Features
- Multiple clipboard formats are supported.
- Frequently used text can be registered as templates in a hierarchy.
- The menu displayed by the hot key can be freely customized.
- History and Template items chosen from the menu are pasted automatically
  into the editor you are working in.
- Pictures are displayed on the menu as thumbnails.
- Tooltips are displayed on the menu.
- The formats to keep in the history and the formats to save can be set up.
- The windows to keep the history for and the windows to ignore can be set
  up.
- The paste key can be set up for each window.
- The history is saved automatically on exit and restored at the next
  start-up.
- There is no limit on the number of items kept in the history.
- The history and templates can be displayed and edited in an Explorer-like
  viewer.
- Functions can be extended by adding plug-ins.
- Unicode support

* Installation
CLCL runs on Windows 7 and later.

Running the downloaded file installs CLCL.
Uninstall CLCL from Control Panel.
Exit CLCL before uninstalling it.

The data is saved in the following folder (on Windows 10):
  C:\Users\(user name)\AppData\Local\CLCL

To save the data in the same place as CLCL.exe, set up clcl_app.ini as
follows and then start CLCL.

[GENERAL]
portable=1

* Start-up
When CLCL starts, a clip icon appears in the system tray (the area of the
taskbar where the clock is).
Clicking this system tray icon displays the menu.
By default, the history is displayed in the menu in ascending order.
The menu can be customized in the Options.

Right-clicking the system tray icon displays the Viewer.
The left side of the Viewer is a tree that shows the History and the
Template.
The right side of the Viewer displays and edits the contents of a History or
Template item. What you edit is applied to the item when the focus moves
away. Some formats cannot be edited. The current contents of the clipboard
cannot be edited.

"Clipboard" at the top of the tree is the current contents of the clipboard.
"History" in the tree is the list of the history.
"Template" in the tree is the list of the registered items (template text
and so on).

    +--[+] Clipboard        - Contents of the current clipboard
    |   +--- TEXT           - Format in the current clipboard
    |   +--- LOCALE
    |   +--- OEM TEXT
    |
    +--[+] History          - Clipboard history
    |   +--- (BITMAP)       - History item
    |   |   +--- BITMAP     - Format in the history item
    |   |   +--- DIB
    |   |
    |   +--- Hello...
    |   |   +--- TEXT
    |   |
    |   +--- Good morning...
    |   +--- (BITMAP)
    |
    +--[+] Template         - Registered items
        |
        +--[+] Folder       - Folder
        |   +--- Address...
        |   +--- (BITMAP)
        |
        +--- http://www...  - Template item
            +--- TEXT       - Format in the template item

* Clipboard
- What is the clipboard?
	The clipboard is an area used to exchange information between
	different applications.
	For example, you can paste text copied in Notepad into Word; this
	works because the area called the clipboard is used.

- Clipboard formats
	The clipboard can hold several formats at the same time.
	For example, when you copy text in Notepad, the following four
	formats are placed on the clipboard (on Windows 10):
		- UNICODE TEXT
		- LOCALE
		- TEXT
		- OEM TEXT
	When you copy in Excel or Access, many more formats are sent to the
	clipboard.

	By default, CLCL keeps the following formats in the history:
		- UNICODE TEXT		- Text
		- BITMAP		- Bitmap
		- DROP FILE LIST	- Files
	Other formats can also be kept in the history with "Filter" in the
	Options.

* History
This is the history of the data copied to the clipboard.
Newly copied data is added at the top of the history.

One history item holds several clipboard formats. Of the formats registered
in "Format" in the Options, the clipboard format with the highest priority
is displayed in the menu and in the Viewer.

The history keeps as many items as set in "History" in the Options.
Only the clipboard formats set to Add in "Filter" in the Options are added
to the history.

* Template
Frequently used data such as template text can be registered in the
Template.
You can add folders to build a hierarchy, and you can give names to items.

To add a template item, open the Viewer, select an item in the history and
choose "Add to Template" from the menu.
If you select the folder you want to add to in the tree and choose
"New Item" from the menu, you can create an empty item, or create an item
by loading the contents from a file.

To add a folder, open the Viewer, open the context menu at the position in
the Template where you want to add it, and choose "New Folder".

To rename a folder or an item, open the Viewer, select the item you want to
change, open the context menu and choose "Rename".
"Clear Name" deletes the name you set, so that the contents of the item are
displayed as its name.
If you name an item "-", it is displayed as a separator in the menu. The
formats and the data in the item are ignored.
If you put & in a name, the character after it becomes the shortcut key in
the menu. To display & itself in the menu, write it as &&.

Right-click a template item to open the menu and choose "Set Hot key" to
assign a hot key to the template item. Pressing that key sends the template
item directly to the clipboard without displaying the menu, and pastes it
directly if "Paste" is enabled.
The assigned hot keys can be checked in the list view of the Viewer. They
are also displayed in the status bar when a template item is selected.

There is no limit on the number of template items or on their clipboard
formats.

* Sending to the clipboard
There are several ways to send a History or Template item to the clipboard.
- Click the system tray icon to display the menu.
  Choosing a History or Template entry sends the data to the clipboard and
  pastes it automatically into the active window.

- Press the hot key (Alt + C by default) to display the menu.
  Choosing a History or Template entry sends the data to the clipboard and
  pastes it automatically into the active window.

- Select an item in the Viewer and open the context menu.
  Choosing "Send to Clipboard" sends the selected item to the clipboard.

* Menu
The entries of the menu displayed from the system tray or by the hot key are
set in "Action" in the Options.
The behavior and the appearance of the menu are set in "Menu" in the
Options.

When you move the mouse over a History or Template entry in the menu, the
detailed contents are displayed in a tooltip at the mouse position. When you
select an entry with the keyboard, the tooltip is displayed below the menu
entry.

Right-clicking a History or Template entry in the menu displays the
registered tools as a menu; the tool you choose is run on the item and the
result is sent to the clipboard.
To display the tool menu with the keyboard, select the entry with Enter
while holding down Ctrl.

History and Template entries are displayed in the menu according to
"Display format of menu text" in the Options. The numbers displayed start
from 0; to change the starting value, put the starting number between % and
the character.
    Example)
         %0d -> 0,1,2,3...
         %8x -> 8,9,a,b...
         %1n -> 1,2,3...8,9,0,1,2...
         %10B -> K,L,M,N...

* Action
The action performed when a hot key is pressed, and the action performed
when the system tray icon is clicked, are set in "Action" in the Options.

If you specify "Menu" as the action in Edit Action, set the menu entries to
be displayed in the menu settings at the bottom of the dialog.

"Call type" sets how the specified action is called.
If you specify "Hot key", set the key that calls it.
"Ctrl + Ctrl", "Shift + Shift" and "Alt + Alt" call the specified action
when the key is pressed twice.

When the action is a menu, "Paste" can be set.
With "Paste", choosing an entry in the menu automatically sends a paste
operation to the application you are working in.
If you hold down Shift while choosing a menu entry, the data is only sent to
the clipboard and is not pasted.

When the action is a menu and the call type is "Hot key", "Ctrl + Ctrl",
"Shift + Shift" or "Alt + Alt", "Show at caret position" can be set.
With "Show at caret position", the menu is displayed at the position of the
caret in the editor. If it is not set, the menu is displayed at the mouse
position.

When the action is a menu and the content is the history, the display range
can be set. The display range is specified from 1 to the maximum number of
items kept in the history. Specifying 0 as the start number means the same
as specifying 1, and specifying 0 as the end number means the same as
specifying the maximum number of items kept in the history.
If the end number is smaller than the start number, nothing is displayed. If
the end number is larger than the maximum number of items kept in the
history, entries are displayed up to that maximum.

* Clipboard Format
CLCL can handle every clipboard format, but a clipboard format that is not
registered is displayed as a binary dump in the Viewer.

Clipboard formats are registered in "Format" in the Options. Formats
registered higher in the list have priority, and the clipboard format with
the highest priority in an item is displayed in the menu and in the Viewer.

To register a format, set the format name, the DLL that processes it and the
function header. If you leave the DLL empty and press the function header
selection button, the list of the built-in function headers is displayed.
For example, of the clipboard formats produced when you copy in Excel, to
process CSV as text, set:
	Format name: CSV
	DLL:
	Function header: text_
and it is then processed as text in the menu and in the Viewer.

* Filter
The clipboard formats to be added to the history are set in "Filter" in the
Options.

If you choose "Add all formats to history", every clipboard format except
the ones set to Ignore is added to the history.
If you choose "Exclude all formats from history", only the clipboard formats
set to Add are added to the history.

For a clipboard format set to Add in the filter, you can also set a limit
size for adding it to the history. Data larger than the limit size is not
added to the history.

For a clipboard format set to Add in the filter, setting "Do not save"
prevents it from being saved to file when CLCL exits.
For example, you can set CLCL up to add both text and bitmaps to the history
but to save only the text.

* Window
To change the behavior of CLCL depending on the application you use, set the
window and the behavior in "Window" in the Options.

Specify the title and the class name of the window; "*" can be used to match
any characters.
For example, for Notepad:
	Title: * - Notepad
	Class name: Notepad
With this setting, the behavior of CLCL changes while Notepad is active.
Only one of the title and the class name needs to be entered; leaving one of
them empty means the same as specifying only "*".

- Do not add to history
	Data copied in the specified window is not added to the history.
	If an application does not work correctly when its data is put in
	the history, specify this option so that copies from that
	application are ignored.

- Do not set focus
	The focus is not set after the specified window has been activated.
	If the focus goes somewhere else when a selected menu entry is
	pasted and the paste does not work correctly, specifying this option
	may make it work.

- Paste even if the tool is cancelled
	Normally the paste that follows is not performed when you cancel a
	tool that can be cancelled, but with this option the paste is
	performed even if you cancel.
	If you set the copy key to the cut key in the key settings for each
	window, specifying this option keeps the characters from being lost
	when you cancel the tool.

* Key settings for each window
When you choose a History or Template item from a hot key and it is pasted
automatically, a paste key is sent to the window.
By default Ctrl + V is sent to every window, but in some windows the paste
key may be a different key.

When a tool is called from a hot key, CLCL performs copy -> tool processing
-> paste, so the copy key (Ctrl + C) is sent to the window.

The copy and paste keys for each window are set in "Key" in the Options.
Set the title and the class name of the window to be configured, and set the
copy and paste keys.

If the copy and paste keys are not set, the default key settings are used.

Several keys can be set for one window. If several are set, the keys are
sent in order from the top of the list.

* Tool (plug-in)
To process the data of History or Template items, or to extend the functions
of CLCL, set them in "Tool" in the Options.

When you select a DLL and a function name, the tool name and the execution
timing are set automatically.
"Action menu" in the execution timing makes the tool available from the menu
set in "Action" in the Options.
"Viewer menu" in the execution timing makes the tool available from the
Tools menu of the Viewer.

"Send Copy and Paste" in the execution timing sends a copy to the active
window, runs the tool on the copied data, and pastes the result into the
active window.
If this option is not checked, the tool is run on the newest history item
and the result is sent to the clipboard. In the tool menu that appears when
you right-click an item in the action menu, the tool is run on the selected
item and the result is sent to the clipboard.
If "Paste" is not enabled in the action settings, no paste is performed
after copying and running the tool.

If you drag and drop a DLL onto the tool list window, the list of the tools
that can be registered is displayed, and you can select several of them and
register them all at once.

* Command line
You can specify a command line when starting CLCL, to specify what it does
after starting.
If CLCL is already running, the command is sent to the running CLCL.

[Format]
CLCL.exe [/vwnx]
	/v Display the Viewer
	/w Turn on Clipboard Watch
	/n Turn off Clipboard Watch
	/x Exit

* Update history

- Ver 2.1.3 -> Ver 2.2.0
	- Added support for the Windows dark mode.
	- Improved to support for high-DPI displays.
	- Added an option to show the menu without taking the focus away
	  from the window you are working in. 
	- Improved the save processing performed on exit.
	- Added UNICODE support to Binary View.
	- Fixed CLCL so that data a password manager saves to the
	  clipboard is not kept in the history. (kashima-eyetech)
	- Improved the wording of the English version.

- Ver 2.1.2 -> Ver 2.1.3
	- Changed the system tray icon shown while the clipboard is not
	  being watched.
	- Improved the up and down buttons in the Options.
	- Improved CLCL so that the main window is not shown when the menu
	  is displayed.

--

The author takes no responsibility for any trouble caused by this program.
You are strongly advised to keep a backup of important files.

Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
	https://www.nakka.com/

2026/8/19
