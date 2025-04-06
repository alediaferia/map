# map

map is a command line utility that maps input content to an arbitrary value.

The map value can be retrieved from a file, specified statically from the command line, or be the result of a command invocation.

Incoming input is split by a 1-character separator (defaults to `\n`) and concatenated back (the concatenator can be customized).

What you can do with this program is very similar to what you can do with `xargs`.

## Usage

```bash
Usage: map [options] <value-source> [--] [cmd]

Value sources (one required):
     -v <static-value>          Static value to map to (implies -z)

     --value-file <file-path>   Read map value from file (implies -z)

     --value-cmd                Use output from command as map value.
                                Each mapped item will be appended to the command arguments list, unless -z is specified.

Optional arguments:
     -s <separator>             Separator character (default: '\n')
     -c <concatenator>          Concatenator character (default: same as separator)
     -z, --discard-input        Exclude input value from map output
     -h, --help                 Show this help message
```

## Map to the result of a command

```bash
echo -e "World\nKitty\n" | ./map --value-cmd -- echo -n 'Hello'   
Hello World
Hello Kitty
```

## Map to a static value by command line

```bash
echo "There\nare\nmany\n\lines\n\in\nthis\nfile." | ./map -v "SOMETHING ELSE"
SOMETHING ELSE
SOMETHING ELSE
SOMETHING ELSE
SOMETHING ELSE
SOMETHING ELSE
SOMETHING ELSE
SOMETHING ELSE
```

## Map to a static value with a different concatenator

```bash
 echo "There\nare\nmany\n\lines\n\in\nthis\nfile." | ./map -v "SOMETHING ELSE" -c,
SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,
```

## Other usage

Check `e2e_test.sh` for additional use cases.
