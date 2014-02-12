In order to compile the logic rdm sniffer, you need to set the include path to your SaleaeDeviceSDK. Example:

```
./configure CPPFLAGS="-I/path/to/your/SaleaeDeviceSdk-1.1.9/include"
```

The configure script should output something like this:

```
checking SaleaeDeviceApi.h usability... yes
checking SaleaeDeviceApi.h presence... yes
checking for SaleaeDeviceApi.h... yes
```
