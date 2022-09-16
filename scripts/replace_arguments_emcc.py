#!/usr/bin/env python3

with open("build/test.js") as f:
    text = f.read()

text = text.replace("Module['arguments'] = window.location.search.substr(1).trim().split('&');",
                    "Module['arguments'] = window.location.search.substr(1).trim().split('&').map(it => decodeURI(it));")

with open("build/test.js", "w") as f:
    f.write(text)
