#ifndef STRINGS_H
#define STRINGS_H

/*
    Replaces the first occurrence of replstr in src with v.
    If pos is not NULL it will be set to the offset of the last character of v + 1
    in the returned string.
    Returns a pointer to a newly created string if a replacement occurred, otherwise
    returns src. If no replacement occurred, pos is set to -1.
*/
const char *strrepl(const char *src, const char *replstr, const char *v, int *pos);

/*
    Replaces all occurrences of replstr in src with v. Returns src if untouched,
    otherwise returns a new string.
*/
const char *strreplall(const char *src, const char *replstr, const char *v);

#endif // STRINGS_H
