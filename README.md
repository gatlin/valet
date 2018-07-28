Valet
===

A helpful XMPP chat bot.

© 2018- Gatlin Johnson <gatlin@niltag.net>

what
---

Valet is an XMPP chat bot inspired by [autobot][autobot]. The idea is simple:
Valet is allowed to run any executable you put in a special, configurable
directory.

When you send Valet a message it will interpret the first word as a command and
pass the rest as arguments.

Details
---

Valet uses [libpurple][libpurple] and the [lurch][lurch] plugin internally.

Configuration
---

A sample config file has been provided, but basically it looks like this:

```
[credentials]
username=user@server.tld
password=ourlittlesecret

[commands]
dir=/opt/valet/commands

[misc]
valetdata=/opt/valet/data
# the location of the OMEMO plugin for libpurple
lurchdir=etc/plugins
```

The credentials should be straightforward. The command dir is where you put any
(and only) those executable files you want Valet to be able to run.

**It is strongly recommended that you run Valet as a special user and clamp down
access to the commands.**

license
---

gplv3 or later you leeches

[libpurple]: https://developer.pidgin.im/wiki/WhatIsLibpurple
[autobot]: https://github.com/mhcerri/Autobot
[omemo]: https://conversations.im/omemo/
[lurch]: https://github.com/gkdr/lurch
