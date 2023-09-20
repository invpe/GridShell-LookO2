# 📜 LookO2 GridShell sketch

- These definitions are mandatory, provide your Grid Name, Grid Hash, WIFI and PASSWORD to start.

```
#define GRID_N ""
#define GRID_U ""
#define WIFI_A ""
#define WIFI_P ""
```

- Customize your settings by updating the following definitions:

```
#define LED_BRIGHTNESS 10
#define TELEMETRY_MINUTES 60000ULL * 1
```


- Telemetry CSV format:

`Epoch,Hour,RSSI,PM1,PM2.5,PM10,HCHO,TEMPERATURE`


- Access your data over GridShell API:

[JSON Datapoints history](https://api.gridshell.net/fs/JadeChartreuseDromiceiomimusLOOKO24c7525a25d82023918)
```https://api.gridshell.net/fs/*GridUserName*LOOKO2*macaddress*ymd```

[JSON Widget latest readings](https://api.gridshell.net/fs/JadeChartreuseDromiceiomimusLOOKO24c7525a25d8J)
```https://api.gridshell.net/fs/*GridUserName*LOOKO2*macaddress*ymdJ```

- The JSON can be visualized with Widgeridoo app.
