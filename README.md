# map

map is a command line utility that maps input content to an arbitrary value.

At the moment, map reads the input data from standard input only.

The map value can be retrieved from a file, specified statically from the command line, or be the result of a command invocation.

Incoming input is split by a 1-character separator (defaults to `\n`) and concatenated back (the concatenator can be customized).

What you can do with this program is very similar to what you can do with `xargs`.

## Usage

```bash
Usage: map [options] <value-source-modifier> [--] [cmd]

Available value source modifiers:
     -v <static-value>          Static value to map to (implies -z)

     --value-file <file-path>   Read map value from file (implies -z)

     --value-cmd                Use output from command as map value
                                Each mapped item will be appended to the command arguments list, unless -z is specified

Optional arguments:
     -s <separator>             Separator character (default: '\n')
     -c <concatenator>          Concatenator character (default: same as separator)
     -z, --discard-input        Exclude input value from map output
     -I <replstr>               Specifies a replacement pattern string. When used, it overrides -z.
                                When the pattern is found in the map value, it is replaced with the current item from the input.

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

## Map using a pattern string

You can use `-I` to specify a pattern string that will be replaced with the incoming input value.

```bash
echo "line1\nline2\nline3" | map -I {} --value-cmd -- echo -n 'This is {}'
This is line1
This is line2
This is line3
```

Or directly using a static map value, without having to run `echo`:

```bash
echo "line1\nline2\nline3" | ./map -I {} -v "This is {}"
This is line1
This is line2
This is line3
```

`-I` also supports the `--value-file` option, meaning it can replace on the fly a map value coming from a file.

## Other usage

Check `e2e_test.sh` for additional use cases.

## Development

This project is still experimental. The command line interface will surely change and get simplified in the future.

There will likely be no stable release before adding support for referencing the input items in the map value (think `-I` in xargs).

Incoming features also include the ability to map directly from/to files.
