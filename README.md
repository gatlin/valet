Valet
===

A helper bot inspired by [https://github.com/mhcerri/Autobot](https://github.com/mhcerri/Autobot).

© 2018- Gatlin Johnson <gatlin@niltag.net>




setup
---

Very early young software here. Create a user on your server and add the account you want to issue commands from as a buddy.

Fill out a config file in `etc/figaro.conf` like so:

```
[credentials]
username=user@server.TLD
password=ourlittlesecret
```

Then you run

    $> make
    $> bin/valet
    
    
Finally - and this is where it gets 🔥 - run 

    $> bin/valet

Put executables in `etc/commands` and when you IM your valet the command and its arguments it will reply with the output.

license
---

gplv3 or later you leeches
