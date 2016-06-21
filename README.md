# ZLIB
Port of the ZLIB code to allow build on IBM i with RELIC Package Manager. Source is from Open Source domain with some changes to allow install via the package manager

Please build into `ZLIBSRC`, programs will be installed into `ZLIB`:
```
RELICGET PLOC('https://github.com/ChrisHird/ZLIB/archive/V1.0.0.2-pre.zip') PDIR('ZLIB-1.0.0.2-pre') PNAME(ZLIBSRC)
```
