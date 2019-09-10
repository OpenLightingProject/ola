This currently only works with the original Saleae Logic and Logic 16.
The more recent Logic 4, Logic 8, Logic Pro 8 and Logic Pro 16 are not supported by the SDK.
https://support.saleae.com/saleae-api-and-sdk/other-information/device-sdk-status-for-new-products

In order to compile the logic rdm sniffer, you need the SaleaeDeviceSDK, there
is a version available from:
http://downloads.saleae.com/SDK/SaleaeDeviceSdk-1.1.14.zip

You will also need to set the include path to your SaleaeDeviceSDK. Example:

```
./configure CPPFLAGS="-I/path/to/your/SaleaeDeviceSdk-1.1.14/include"
```

The configure script should output something like this:

```
checking SaleaeDeviceApi.h usability... yes
checking SaleaeDeviceApi.h presence... yes
checking for SaleaeDeviceApi.h... yes
```
