# CLCL

CLCL Ver 2.1.3
--

## Introduction
CLCL is a software that records clipboard history.

## Functions
- Supports multiple clipboard formats
- Frequently used standard phrases can be registered as templates in a hierarchical structure
- Freely customize the menu displayed with hotkeys
- History items and template items can be selected from menu and automatically pasted into the current window
- Display thumbprint images in the menu
- Display tooltips in the menu
- The format to leave and the format to save can be set up
- It can be configured for which windows to save or not to save history
- The paste key can be set up individually for every window
- History is automatically saved when the program is closed and restored the next time it is started
- No limit on the maximum number of items that can be saved in the history
- View and edit history and templates in an Explorer-style viewer
- Program is extensible by plug-ins
- Supports Unicode

## Installation
Works on Windows XP and later OS.

CLCL will be installed when you launch the downloaded file.

Please uninstall from the Control Panel after closing CLCL.

Data will be saved in the following folder (for Windows 10):

    C:\Users\{username}\AppData\Local\CLCL

To save data in the same location as CLCL.exe, set clcl_app.ini as follows and then start CLCL.

    [GENERAL]
    portable=1

## Startup
When you start CLCL, a clip icon will be displayed in the task tray (the area with the clock in the corner of the taskbar).
Clicking on this task tray icon will display a menu.
By default, the menu displays the history in ascending order.
The menu can be customized in the settings.

Right-clicking on the task tray icon will display the viewer.
The left side of the viewer is a tree that displays the history and template items.
The right side of the viewer displays and edits the contents of the history and template items. The edited contents will be reflected in the item when you move the focus. Some formats cannot be edited. The contents of the current clipboard cannot be edited.

The "Clipboard" at the top of the tree is the current clipboard contents.
The "History" in the tree is a list of the history.
"Templates" in the tree is a list of template items (such as standard phrases).

	┌─■ Clipboard - Current clipboard contents
	│ ├─□ TEXT - Formats in the current clipboard
	│ ├─□ LOCALE
	│ └─□ OEM TEXT
	│
	├─■ History - Clipboard history
	│ ├─□ (BITMAP) - History items
	│ │ ├─□ BITMAP - Formats in history items
	│ │ └─□ DIB
	│ │
	│ ├─□ Hello...
	│ │ └─□ TEXT
	│ │
	│ ├─□ Today...
	│ └─□ (BITMAP)
	│
	└─■ Templates - Template items
	  │
	  ├─■ Folder - Folder
	  │ ├─□ Address...
	  │ └─□ (BITMAP)
	  │
	  └─□ http://www... - Template item
		└─□ TEXT - Format in template item

## Clipboard
### What is the clipboard?
The clipboard is an area for exchanging information between different applications.
For example, you can paste text copied in Notepad into Word, and this is because the clipboard is used.

### Clipboard format
The clipboard can hold multiple formats at once.
For example, if you copy text in Notepad, the following four formats will be stored in the clipboard (Windows 10):

	・UNICODE TEXT
	・LOCALE
	・TEXT
	・OEM TEXT
If you copy in Excel or Access, even more formats will be sent to the clipboard.

CLCL's default settings are:

	・UNICODE TEXT - Text
	・BITMAP - Bitmap
	・DROP FILE LIST - File
These formats will be saved in the history.
You can also save other formats in the history by using the optional "Filter".

## History
This is the history of data copied to the clipboard.
Newly copied data is added to the top of the history.

A single history item contains multiple clipboard formats. The clipboard formats with the highest priority are displayed in the menu and viewer, which are registered in the optional "Format".

The number of history items remains as set in the optional "History".
Only clipboard formats set to add in the optional "Filter" are added to the history.

## Templates
Templates can be used to register commonly used data such as standard phrases.
You can add folders to organize them into hierarchical structures and give names to items.

To add a template item, open the viewer, select the history item, and select "Add to Templates" from the menu.
Select the folder you want to add in the tree, and select "Create New" from the menu to create an item by loading the contents of an empty item or file.

To add a folder, open the viewer, right-click on the location in the templates where you want to add it, and select "Create Folder."

To rename a folder or item, open the viewer, select the item you want to rename, right-click on it, and select "Rename."
"Initialize Name" erases the name you set and displays the item's contents as the name.
If you name an item "-", it will be displayed as a separator in the menu. The format and data in the item will be ignored.
If you add & to the name, the character following it will be used as the shortcut key in the menu. If you want to display & itself in the menu, use &&.

Right-click on a template item to display the menu and select "Hotkey Settings" to assign a hotkey to the template item. Pressing that key will send the template item directly to the clipboard without displaying the menu, and will paste it directly if "Paste" is enabled.
You can check the registered hotkeys in the list display area of the viewer. If you select other template items, they will be displayed in the status bar.

There is no limit to the number of template items or the clipboard format.

## Send to Clipboard
There are several ways to send history or template items to the clipboard.

-  Click the task tray to bring up a menu.
If you select a history or template item from the menu, the data will be sent to the clipboard and automatically pasted into the active window.

- Press the hotkey (default is Alt + C) to bring up a menu.
If you select a history or template item from the menu, the data will be sent to the clipboard and automatically pasted into the active window.

- Select an item in the viewer and bring up a right-click menu.
If you select "Send to Clipboard", the selected item will be sent to the clipboard.

## Menu
The menu items displayed in the task tray or hotkey are set in the "Action" option.
You can customize the menu behavior and display in the "Menu" option.

If you move the mouse over a history or template item in the menu, a tooltip with detailed information will be displayed at the mouse position. If you select an item with the keyboard, a tooltip will be displayed under the menu item.

Right-clicking on a history or template item will display the registered tools in a menu, and the selected tool will be executed and sent to the clipboard.

To display the tool menu with the keyboard, press Ctrl and Enter to select an item.

History and template items display menu items according to the "Menu text display format" option. The displayed numbers start from 0, but if you want to change the starting value, set the starting number between the % and the character.

Examples:

	%0d -> 0,1,2,3...
	%8x -> 8,9,a,b...
	%1n -> 1,2,3...8,9,0,1,2...
	%10B -> K,L,M,N...

## Action
The action when the hotkey is pressed or the task tray icon is clicked can be set in the "Action" option.

When you specify "Menu" as the action in Edit Action, you can set the menu items to display in the menu settings at the bottom of the screen.

Invoking method sets the method for invoking the specified action.

When you specify "Hotkey", set the key to invoke.
"Ctrl + Ctrl", "Shift + Shift", and "Alt + Alt" invoke the specified action when you press the key twice.

If you specify Menu as the action, you can set "Paste".
When you select an item in the menu, the paste action is automatically sent to the application you are working in.
If you hold down Shift when selecting a menu item, it will not be pasted but will only be sent to the clipboard.

If you specify Menu as the action and Hotkey, Ctrl + Ctrl, Shift + Shift, or Alt + Alt as the invoking method, you can set "Show at caret position".
"Show at caret position" displays the menu at the caret position of the editor.
If you do not set it, the menu will be displayed at the mouse position.

If you specify Menu as the action and select History as the item, you can set the display range. The display range is specified from 1 to the number of items to be left in the history. Specifying 0 as the start number is the same as specifying 1, and specifying 0 as the end number is the same as specifying the number of items to be left in the history.

If the end number is smaller than the start number, nothing will be displayed. If the end number is larger than the number of items to be left in the history, only the number of items to be left in the history will be displayed.

## Clipboard format
CLCL can process all clipboard formats, but clipboard formats that are not registered will be displayed as binary dumps in the viewer.

Clipboard formats are registered in the options "Format". The registered format at the top takes priority, and the clipboard format with the highest priority among the items is displayed in the menu and viewer.

To register, set the format name, the DLL to be processed, and the function header. If you omit the DLL and press the function header selection button, a list of built-in function headers will be displayed.
For example, if you want to process the CSV clipboard format as text when copying in Excel, set it as follows:
	Format name: CSV
	DLL:
	Function header: text_
The menu and viewer will process it as text.

### Filter
To select the clipboard format, set it in the "Filter" option.

If you select "Add all formats to history", all clipboard formats except those set to be excluded will be added to the history.

If you select "Exclude all formats from history", only clipboard formats set to be added will be added to the history.

For clipboard formats set to be added in the filter, you can further set the size limit when adding to the history. Data exceeding the limit size will not be added to the history.

If you set the clipboard format set to be added in the filter to "Do not save", it will not be saved to a file when CLCL is closed.
For example, you can set it to add text and bitmaps to the history and save only the text.

## Window settings
If you want to change the behavior of CLCL depending on the application you use, set the window and behavior in the "Window" option.

Specify the window title and class name, and use "*" as any character.
For example, for Notepad, set it as follows:

	Title: * - Notepad
	Class name: Notepad

The behavior of CLCL will change when Notepad is active.
Either the title or the class name needs to be entered, and if it is not entered, it is the same as specifying only "*".

- Don't add to history:
    Copying in the set window will not be added to the history.
    If you specify this option for an application that causes problems when added to the history, copying from that application will be ignored.

- Don't set focus: 
	Focus will not be set after activating the set window.
	If the focus goes somewhere when pasting the selected menu item and the pasting is not performed correctly, specifying this option may make it work correctly.

- Paste even if the tool is canceled: 
	When canceling a cancelable tool, the subsequent pasting is usually not performed, but if this option is specified, the pasting will be performed even if it is canceled.
	If you set the copy key as the cut key in the key settings for each window, specifying this option will prevent the characters from disappearing even if you cancel the tool.

## Key settings for each window
Select the history or template item from the hotkey, and the paste action will automatically send the paste key to the window.
By default, Ctrl + V is sent to all windows, but depending on the window, the paste key may be a different key.

When the tool is called with a hotkey, the copy key (Ctrl + C) is sent to the window to perform the Copy -> Tool Processing -> Paste action.

The copy and paste keys for each window are set in the "Key" option.
Set the title and class name of the window to be set and set the copy and paste keys.

If the copy and paste keys are not set, the default key settings will be used.

Multiple keys can be set for one window. If multiple keys are set, the keys will be sent in order from the top.

## Tools (plug-ins)
To process history or template item data or expand the functions of CLCL, set it in the "Tool" option.

When you select the DLL and function name, the tool name and call type will be automatically set.
The call type "Action Menu" allows you to execute from the "Tools" popup menu.
The call type "Viewer" allows you to execute from the viewer's tool menu.

The "Send copy and paste" option copies the marked data from the active window, executes a tool on the copied data, and afterwards pastes the modified data back into the active window.
If this option is not checked, the tool is executed on the most recent history item and sent to the clipboard. In the tool menu that appears when you right-click an item in the action menu, the tool is executed on the selected item and sent to the clipboard.
If "Paste" is not enabled in the action settings, pasting will not be performed after copying and executing the tool.

Drag and drop a DLL into the tool list window to display a list of tools that can be registered, and you can select multiple tools to register them all at once.

## Command Line
When starting CLCL, you can specify a command line to specify the operation after startup.
If CLCL is already running, the command will be sent to the already running CLCL.
#### Format:

	CLCL.exe [/vownx]
		/v Display viewer
		/w Monitor clipboard
		/n Cancel monitoring clipboard
		/x Exit

## Special thanks
K.Takata ( http://webs.to/ken/ )

## Update history
- Ver 2.1.2 -> Ver 2.1.3

	- Changed the system tray icon when not monitoring the clipboard.
	- Improved the up and down movement buttons in the settings.
	- Improved so that the main screen does not appear when displaying the menu.

- Ver 2.1.1 -> Ver 2.1.2
	- Added the ability to scroll by dragging the mouse in image formats (BITMAP, DIB).

- Ver 2.1.0 -> Ver 2.1.1
	- Added a function to copy file names and file paths in file formats.
	- Added a setting to place settings and data in the same location as CLCL.exe.

- Ver 2.0.3 -> Ver 2.1.0
	- High DPI support.
	- Support for PNG and JPEG when saving image formats (BITMAP, DIB).
	- Enlarge or reduce image formats (BITMAP, DIB).
	  - Hold down Ctrl and use the mouse wheel, or Ctrl+↑↓ to change the magnification.
	- Binaries are no longer separated into Japanese and English versions.
	- Changed the settings save location to be saved in a separate area for each user.
	  -	For Windows 10/11, "C:\Users\(username)\AppData\Local\CLCL"
		(Automatically migrated on first launch)

- Ver 2.0.2 -> Ver 2.0.3
	- Fixed typos in the English version settings screen.

- Ver 2.0.1 -> Ver 2.0.2
    - Support for Unicode in settings files.
	  - Starting once with Ver 2.0.2 will make the settings file Unicode.
		The files before conversion are backed up with the file names "general.ini.back" and "clcl.ini.back".
		If you want to revert to an older version, use the above files.
	- The startup function has been changed from WinMain to wWinMain.

- Ver 2.0.0 -> Ver 2.0.1
	- Incorporated correction code from K. Takata
	  - Supports Unicode surrogate pairs
	  - Retry processing when task tray addition fails
	  - Prevents consecutive double startups and double startups from another user
	- Fixed a problem where the color of items disabled in the "Action" settings would sometimes become invisible
	- Improved the task tray icon when clipboard monitoring is stopped.

- Ver 1.1.2 -> Ver 2.0.0
	- Supports Unicode. As a result, older versions of Windows are no longer supported.
	  -	The settings of the older version will be inherited, but tools and plugin formats must be Unicode-compatible.
	- Changed the behavior to copy the file if the save is successful so that template items are not deleted if the save process fails when CLCL is closed.
	- Changed the icon design in the task tray.
	- Changed the default behavior when clicking on the task tray.
	  - Changed the menu to appear when left-clicking the task tray icon to take into account operation on tablets.

- Ver 1.1.1 -> Ver 1.1.2
	- Fixed the problem where the tooltip background was not displayed correctly in a 256-color environment.
	- Optimized the menu display method.
	- Optimized the text display in the viewer.

- Ver 1.1.0 -> Ver 1.1.1
	- Improved the behavior when selecting the character position with the mouse in TEXT editing.
	- Fixed the error that occurred when there was no history to save with the filter when closing.
	- Made it possible to move items between history and template items.
	- Changed the method of displaying thumbnail images.
	- Changed the initial value of the waiting time for copy and paste to 100.
	- Fixed a problem where memory was not being released.

- Ver 1.0.9 -> Ver 1.1.0
	- Fixed a problem where the menu would loop when Ctrl+V or other keys were specified as the hotkey to bring up the menu.
	- Now it is possible to specify the Windows key or Space key as the hotkey for actions and tools.

- Ver 1.0.8 -> Ver 1.0.9
	- Fixed a problem where the display would become distorted if the confirmation message for the same format was canceled when creating a new item in the viewer.
	- Fixed a problem where the edited format would be displayed unconditionally regardless of priority when editing the contents of the format in the viewer.
	- Fixed a problem where the status bar was not updated when editing an item in the viewer.
	- Fixed a problem where the position of Japanese input would shift when editing TEXT.

- Ver 1.0.7 -> Ver 1.0.8
	- Change the INI file name (user.ini -> clcl.ini)
	 The file name will be changed automatically at startup.
	- Changed display and editing of TEXT format from rich edit control to our own.
	- Improved behavior of the viewer when calling tools from the viewer.
	- Extended the binary display function.
	- Other

- Ver 1.0.6 -> Ver 1.0.7
	- When selecting an item in the menu while holding down the Shift key, it will only send it to the clipboard without pasting.
	- Added a command line option.
	- Fixed a bug that prevented keys from being sent to some windows.
	- Increased the default behavior settings for the list in the viewer options.

- Ver 1.0.5 -> Ver 1.0.6
	- Made it possible to copy items in the behavior settings.
	- Changed the number of history items displayed to the display range in the behavior settings.
	- Made minor adjustments to the options screen.

- Ver 1.0.4 -> Ver 1.0.5
	- Made it possible to assign hotkeys to template items.
	- Made hotkeys visible in the menu.
	- The viewer position is no longer adjusted when launched.
	- The position and size are now forcibly saved when closing the viewer.
	- 0x7F is no longer displayed in the character display area when displaying binary data.
	- Files can now be opened even if they are locked.

- Ver 1.0.3 -> Ver 1.0.4
	- Files can now be automatically registered as external applications when dragged and dropped into the operation settings screen.

- Ver 1.0.2 -> Ver 1.0.3
	- Fixed a bug where history was not saved correctly when the OS was shut down on Windows XP.

- Ver 1.0.1 -> Ver 1.0.2
	- Improved the timing of releasing memory associated with menus.
	- Folder icons now use system icons.

- Ver 1.0.0 -> Ver 1.0.1
	- Added settings for each window.
	- Fixed a bug where text was not redrawn after changing the font depending on the environment.
	- Fixed a bug where pasting was not possible in some software.
	- Fixed a bug where all windows were targeted when only one of the window name and class name was specified in the window settings and key settings.

- Ver 0.2.0 -> Ver 1.0.0
	- Recreated
	 Data and settings will not be carried over.
	 If you want to migrate template items from Ver 0.2.0 to Ver 1.0.0, start both and send the template items from Ver 0.2.0 to the clipboard and add them to the history of Ver 1.0.0. Then, register the items from the history added to Ver 1.0.0 to the template items of Ver 1.0.0. Tools that are compatible with Ver 0.2.0 can also be used with Ver 1.0.0. (Some restrictions apply) 


The author is not responsible for any problems caused by this program. It is strongly recommended that you back up important files. 

Please contact nakka@nakka.com for bugs, requests, and questions. 

Copyright (C) 1996-2022 by Ohno Tomoaki. All rights reserved. https://www.nakka.com/ 
2022/12/1

