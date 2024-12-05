![Pidgin logo](https://raw.githubusercontent.com/dadecoza/pidgin-meshtastic/refs/heads/main/nsis/pidgin-meshtastic.ico)

# a Meshtastic plugin for Pidgin

Make sure you already have [Pidgin](https://pidgin.im/install/) installed.

The Windows installer for the plugin is available [here](https://github.com/dadecoza/pidgin-meshtastic/releases/download/v0.1.0-alpha/pidgin-meshtastic-0.1.0-alpha.exe).

Precompiled libraries for Windows and Linux can be found under [releases](https://github.com/dadecoza/pidgin-meshtastic/releases).


## Build

### Debian
```shell
sudo apt-get update
sudo apt-get install pidgin-dev git
git clone https://github.com/dadecoza/pidgin-meshtastic.git --recursive
cd pidgin-meshtastic
make
sudo make install
 ```

### Windows
Follow the [Pidgin for Windows Build Instructions](https://developer.pidgin.im/wiki/BuildingWinPidgin.html) and navigate to $PIDGIN_DEV_ROOT .
```shell
git clone https://github.com/dadecoza/pidgin-meshtastic.git --recursive
cd pidgin-meshtastic
make -f Makefile.mingw
```
 * Copy libmeshtastic.dll to C:\Program Files (x86)\Pidgin\Plugins
 * Copy the pixmaps directory to C:\Program Files (x86)\Pidgin

# Adding the account
![Account screenshot](https://dade.co.za/images/account.png)
* *Protocol*: Meshtastic
* *Username*: this can be anything, your radio will update the username once connected
* *COM port or IP address*: Typically "COMx" on Windows, or "/dev/ttyUSBx" on Linux. It can also be a valid IPv4 address for socket connections.
* *Local Alias*: Can be left empty, it will populate on first connect.

# Usage
![Menus screenshot](https://dade.co.za/images/menus.png)
* Right-click on a node and navigate to the "Meshtastic" menu item for Meshtastic-specific actions.
* These actions will be applied to your local node (indicated by the blue flag) or any remote node where you have admin access.
* A note on admin access: The legacy admin channel will take precedence over admin keys. This means that if you want to administer newer nodes with admin keys configured, you must ensure the admin channel is disabled or you will get the "no channel" error message.



