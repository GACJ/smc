## SMC32

### Build Status
[![Build Status](https://github.com/GACJ/smc/workflows/CI/badge.svg)](https://github.com/GACJ/smc/actions)

### Building SMC32
#### Windows
SMC32 can be built using Visual Studio 2017 Community or above.

**via VS2017 developer prompt:**
```
msbuild /p:Configuration=Release /p:Platform=Win32 /v:m
```

**via IDE:**

Open smc32.sln and build.

#### Linux
```
mkdir bin && cd bin
cmake ..
make
```
