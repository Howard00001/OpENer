
OpENer
====================

https://github.com/EIPStackGroup/OpENer



Build:
-------------

##### Install requirements

* CMake
* gcc
* make
* binutils
* the development library of libcap (libcap-dev or equivalient)

##### Make binary

```
cd <OpENer main folder>/bin/posix
```

```
cmake -DCMAKE_C_COMPILER=gcc -DOpENer_PLATFORM:STRING="POSIX" -DCMAKE_BUILD_TYPE:STRING="" -DBUILD_SHARED_LIBS:BOOL=OFF ../../source
```

```
make
```

##### Run

```
<OpENer main folder>/bin/posix/src/ports/POSIX/OpENer <interface>
```



EthernetLink
--------------------
[document](./doc/ethernetlink.pdf)


Identity (System)
-------------------
[document](./doc/identity[system].pdf)