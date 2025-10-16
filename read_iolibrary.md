w5500 capability:
 - voltage: 3.3V
 - current: 126mA
 - ports: 8
 - buffer: 32K
 - spi freq max: 80M

setup scheme:
- gpio/spi pins init
- register callbacks for iolibrary
- wizchip_init to set tx/rx buff size
- physical layer: wizphy_phyconf()
- network layer: ctlnetwork-CN_SET_NETINFO for ip, gateway, subnet,..; wizchip_settimeout to set retry count and timeout

**socket apis in iolibrary:**