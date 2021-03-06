Valet
===

A helpful, secure XMPP chat bot.

© 2018- Gatlin Johnson <gatlin@niltag.net>

what
---

Valet is an XMPP chat bot inspired by [autobot][autobot]. The idea is simple:
Valet is allowed to run any executable you put in a special, configurable
directory.

When you send Valet a message it will interpret the first word as a command and
pass the rest as arguments.

### Encryption

Valet uses [lurch][lurch] to support [OMEMO][omemo] encryption for XMPP
messages.
Encryption is only available for **normal** XMPP chats, **not** Bonjour chats.

If you want to disable OMEMO entirely for whatever reason, see the included
sample configuration file for details.

### Bonjour (Zeroconf) support

Valet can also advertise itself over Bonjour chat on local networks.
For example it might be handy to have a chat bot available to everyone in an
office or home.

Valet can operate in either XMPP or Bonjour mode separately *or** simultaneously.
To enable it, see the sample configuration file.

*Note: As stated above, OMEMO encryption is not available for Bonjour chats.*

How to build Valet
---

### Dependencies

On Ubuntu and other Debian systems:

    $> sudo apt install libpurple-dev libglib2.0-dev libmxml-dev libxml2-dev
    libsqlite3-dev libgcrypt20-dev

### Build lurch

Valet relies on [lurch][lurch] for [OMEMO][omemo] encryption, and so it has been
added as a sub-module. For our purposes you can run the following:

    $> git submodule update --init --recursive
    $> cd thirdparty/lurch
    $> make

### Build Valet

    $> make

Configuration
---

A sample config file has been provided and it resembles this:

```
[credentials]
username=user@server.tld
password=ourlittlesecret

[valet]
# paths can be relative or absolute
commands=etc/commands
libpurpledata=etc/account
lurch=thirdparty/lurch/build/lurch.so
```

The credentials should be straightforward.

`commands` is the directory where you will place the executables you want Valet
to have access to.

**It is strongly recommended that you run Valet as a special user and clamp down
access to the commands.**

`libpurpledata` is where libpurple should store its data.
`lurch` is the location of the `lurch` plugin you built. It should be correct by
default.

Usage
---

```
Usage:
  valet [OPTION?] - a helpful xmpp bot

Help Options:
  -h, --help       Show help options

Application Options:
  -c, --config     Location of configuration file
```

license
---

gplv3 or later you leeches

[libpurple]: https://developer.pidgin.im/wiki/WhatIsLibpurple
[autobot]: https://github.com/mhcerri/Autobot
[omemo]: https://conversations.im/omemo/
[lurch]: https://github.com/gkdr/lurch
[glib]: https://developer.gnome.org/glib/2.56/
