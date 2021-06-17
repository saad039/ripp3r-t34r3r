##  How to Build

```shell
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## How to Run

The program will attempt to obtain the password hash and  salt directly from the host's `/etc/shadow` file. If you want to override this behaviour, specify a different file path in `util.hpp` or just override the values of `hash` and `salt` in `main.cpp`.

```shell
sudo mpiexec -n 26 --oversubscribe --allow-run-as-root --mca opal_warn_on_missing_libcuda 0 PasswordCracker
```

## License
[GPL3](https://www.gnu.org/licenses/gpl-3.0.en.html)
