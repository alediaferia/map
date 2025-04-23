# map

A fast and efficient CLI tool for mapping and transforming input values to output.

## Features

- Configurable input/output separators
- Multiple input sources support (stdin, files, commands)
- Supports replacement patterns (see [here](#pattern-string))
- Zero external dependencies

What you can do with this program is very similar to what you can do with `xargs`.

## Usage

```bash
map [-s separator] [-c concatenator] (-v pattern | --value-file file | --value-cmd command)
```

### Options

- `-s`: Input separator character (default: *newline*)
- `-c`: Output concatenator character (default: same as separator)
- `-v`: Specify a static map value to map each input item to. See `-I` for patterns support.
- `--value-file`: Read map value from file
- `--value-cmd`: Use command output as map value
- `-I <replstr>`: Replace any occurrence of `replstr` in the map value with the incoming input item. See [here](#pattern-string) for more examples.

Full usage screen:

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

## Usage examples

Some detailed examples here

### Map to the result of a command

```bash
echo -e "World\nKitty\n" | ./map --value-cmd -- echo -n 'Hello'   
Hello World
Hello Kitty
```

### Map to a static value by command line

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

### Static value, change concatenator on the fly

```bash
echo "There\nare\nmany\n\lines\n\in\nthis\nfile." | ./map -v "SOMETHING ELSE" -c,
SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,
```

### Pattern string

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
# Output:
# This is line1
# This is line2
# This is line3

# String pattern and concatenator change on the fly
echo "apple\nbanana" | map -c "," -I {} -v "Fruit: {}"
# Output: Fruit: apple,Fruit: banana
```

`-I` also supports the `--value-file` option, meaning it can replace on the fly a map value coming from a file.

### Other usage

Check `e2e_test.sh` for additional use cases.

## Limitations

- At the moment, map reads the input data from standard input only.
- Separator and concatenator are limited to 1 character currently.
- Input and output is currently limited to stdin/stdout (e.g. you need to pipe content in/out if you want to act on files)

## Building from source

```bash
git clone https://github.com/alediaferia/map.git
cd map
make
```

## Development

This project is still experimental. The command line interface will surely change and get simplified in the future.

There will likely be no stable release before adding support for referencing the input items in the map value (think `-I` in xargs).

Incoming features also include the ability to map directly from/to files.

## License

map is released under the BSD-2-Clause license. You can find a copy of the license in this repository.
