# Example of a Slackbot for Pico W

Connect your [Raspberry Pi Pico W](https://www.raspberrypi.com/products/raspberry-pi-pico/) board to [Slack](https://slack.com) using the [Slack API](https://api.slack.com).

Requires a Slack App token and Slack Bot token.

Learn more in the [Raspberry Pi "Create your own Slack bot with a Raspberry Pi Pico W" guest blog post](https://www.raspberrypi.com/news/create-your-own-slack-bot-with-a-raspberry-pi-pico-w/).


## Hardware

 * [Raspberry Pi Pico W](https://www.raspberrypi.com/products/raspberry-pi-pico/)

## Software

## MicroPython

1. Set up [MicroPython on the Raspberry Pi Pico W](https://projects.raspberrypi.org/en/projects/get-started-pico-w/1)
2. Copy files from [micropython](micropython) folder to the Raspberry Pi Pico W
3. Update [config.py](micropython/config.py) with your:
   * Wi-Fi SSID and Password
   * Slack App token and Bot token
4. Run [main.py](micropython/main.py)

## pico-sdk

### Cloning

```sh
git clone --recurse-submodules https://github.com/ArmDeveloperEcosystem/example-of-a-slackbot-for-pico-w.git
```

### Building

1. [Set up the Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
2. Set `PICO_SDK_PATH`
```sh
export PICO_SDK_PATH=/path/to/pico-sdk
```
3. Change directories:
```
example-of-a-slackbot-for-pico-w/pico-sdk
```
4. Create `build` dir, run `cmake` and `make`:
```
mkdir build

cd build

cmake .. \
  -DWIFI_SSID="<Wi-Fi SSID>" \
  -DWIFI_PASSWORD="<Wi-Fi Password>" \
  -DSLACK_APP_TOKEN="<Slack App token>" \
  -DSLACK_BOT_TOKEN="<Slack Bot token>" \
  -DPICO_BOARD=pico_w

make
```

5. Copy example `picow_slack_bot.uf2` to Pico W when in BOOT mode.

## License

[MIT](LICENSE)

## Acknowledgements

[Configuration files](pico-sdk/config) in the [pico-sdk](pico-sdk) based example are based on the following files from the [pico-examples GitHub repository](https://github.com/raspberrypi/pico-examples/):

* [FreeRTOSConfig.h](https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/freertos/ping/FreeRTOSConfig.h)
* [lwipopts_examples_common.h](https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/lwipopts_examples_common.h)
* [lwipopts.h](https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/freertos/ping/lwipopts.h)
* [mbedtls_config.h](https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/tls_client/mbedtls_config.h)

---

Disclaimer: This is not an official Arm product.
