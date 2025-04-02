# map

Map is a command line utility that maps input content to an arbitrary value.
Incoming input is split by a 1-character separator (defaults to `\n`) and concatenated back (the concatenator can be customized).

Think of it like `xargs`, but worse (for now...).

## Usage

```bash
Usage: map [options] (-v <static-value> | --value-file <file-path> | --value-cmd) [--] [cmd]

Options:
     -v <static-value>          Value to map to (default: none)
     --value-file <file-path>   Path to a file containing the value to map to
     --value-cmd <cmd>          The command to run to get the value to map to
     -s <separator>             Separator character (default: '\n')
     -c <concatenator>          Concatenator character (default: same as separator)
     -h, --help                 Show this help message
```

## Example usage

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

```bash
 echo "There\nare\nmany\n\lines\n\in\nthis\nfile." | ./map -v "SOMETHING ELSE" -c,
SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,SOMETHING ELSE,
```
