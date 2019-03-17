## SMC32

### Build Status
| Windows      | [![Build Status](https://dev.azure.com/gacj/smc32/_apis/build/status/IntelOrca.smc32?branchName=master&jobName=Windows)](https://dev.azure.com/gacj/smc32/_build/latest?definitionId=4&branchName=master) |
|--------------|-----------------|
| **Ubuntu 16.04** | [![Build Status](https://dev.azure.com/gacj/smc32/_apis/build/status/IntelOrca.smc32?branchName=master&jobName=Ubuntu%2016.04)](https://dev.azure.com/gacj/smc32/_build/latest?definitionId=4&branchName=master) |

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
