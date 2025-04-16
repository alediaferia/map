#ifndef STRINGS_H
#define STRINGS_H

/*
    Replaces all occurrences of replstr in src with v.
    Always returns a new string. If no occurrences are found, returns a copy of src.
*/
const char *strreplall(const char *src, const char *replstr, const char *v);

#endif // STRINGS_H
