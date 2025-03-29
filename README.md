# map

Map is a command line utility that maps input content to an arbitrary value.
Incoming input is split by a 1-character separator (defaults to `\n`) and concatenated back (the concatenator can be customized).

Think of it like `xargs`, but worse (for now...).

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
