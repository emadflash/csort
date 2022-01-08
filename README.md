# csort
sorting imports for *python* aka dumb language

## Building
```
$ mkdir build && make
```

## Run
```
$ ./build/csort
usage: csort [FILE] [options..]
options:

  -s| --show: [Bool]
    show changes after sanitizing

  -dw| --disable-wrapping: [Bool]
    disable wrapping for duplicate librarys

  -sd| --no-squash-duplicates: [Bool]
    disable squashing duplicate librarys

  -wa| --wrap-after: [Int]
    starts wrapping imports after n, imports
```

Currently *csort* doesn't make any changes to the file, you can view changes by turning on `-s` flag.

*csort* looks for user settings in `.csortconfig` in current directory
otherwise default settings are used

For settings options look into *[.csortconfig](.csortconfig)*
