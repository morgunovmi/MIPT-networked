# MIPT-networked
## Build

First clone with all submodules(important):
```
git clone --recurse-submodules -j8 https://github.com/morgunovmi/MIPT-networked.git
cd MIPT-networked
```

Go to the corresponding week folder and build, for example for w2:

```
cd w2
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```
