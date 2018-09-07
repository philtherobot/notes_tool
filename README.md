# Tool for notes


## Using

notes_tool can
 - reports deviations from the note format, 
 - fix the deviations
 - report on the spheres, projects and tags in use.


## Building

Build with "grind" build tool.

Dependencies are:
 - Boost (Filesystem, System, Range, Algorithm, Optional)
 - Googletest
 
This command line has been used successfully with the tests taken out:

```
g++ --std=gnu++17 -g notes_tool.cpp -lboost_filesystem -lboost_system -o notes_tool
```

GCC 7.3.0.

