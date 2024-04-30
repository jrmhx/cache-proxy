# Web Proxy with Cache

## Author: Jeremiah Xing

## Intro

This is a comandline tool to implement a web proxy with cache.

## Usage

`make all` to compile the program.

`make clean` to clean the object files.

`./proxy <port number> [cache replacement policy]` to run the program.

for example:

`./proxy 8080` to run the program with default cache replacement policy: LRU on 8080 port.

`./proxy 8080 LRU` to run the program with LRU cache replacement policy on 8080 port.

`./proxy 8080 LFU` to run the program with LFU cache replacement policy on 8080.


## Test Environment

macOS / Linux

### Recommended: Firefox

Easiest web browser to set up for using your proxy. Open up settings, and search for Proxy. In these settings, select manual proxy configuration. In the HTTP proxy field, enter in localhost (127.0.0.1) and the port your proxy is using. Next, browse to **about:config**. Search for **network.proxy.allow_hijacking_localhost** and set this value to **True**. This is required for firefox to use your proxy when accessing a web-server hosted on your local machine.

Additionally, you should disable browser caching to ensure all requests are served through your proxy. This can be done by setting **browser.cache.disk.enable** and **browser.cache.memory.enable** to **False** in **about:config**. Don't forget to turn these back on once you're done with the test!

### Other Browsers

Other browsers, such as Chrome and Safari, can use a proxy. However, the default method for using a proxy in these browsers is to configure the entire system to use a proxy, rather than just the browser. Configuration can be achieved in the relevant settings page of these browsers. If possible, it would be best to install Firefox as it allows more fine-grained control, proxying only plain HTTP requests from just the browser.


## Benchmark

run `./driver.sh` in a Linux environment.

## Reference
1. [Condition variables in C](https://www.youtube.com/watch?v=0sVGnxg6Z3k)
2. [Signaling for condition variables (pthread_cond_signal vs pthread_cond_broadcast)](https://www.youtube.com/watch?v=RtTlIvnBw10)
3. [How to prevent SIGPIPEs (or handle them properly)](https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly)
