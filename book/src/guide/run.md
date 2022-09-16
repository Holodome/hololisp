# Running

hololisp interpreter executable can be used in three modes: REPL, executing script files, and executing command strings.

To execute script pass its name to the program executable:

```shell
$ hololisp examples/game_of_life.hl
```

REPL session looks like this:

```shell
$ hololisp
hololisp> (define x 100)
x
hololisp> x
100
hololisp> (+ x (* x 100))
10100
```

To execute command string pass it after *-e* flag:

```shell
$ hololisp -e "(+ 1 2)"
3
```