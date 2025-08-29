# Notes on `./configure`:
If `./configure` ever fails at checking your compiler, it's probably because your compiler doesn't support implicit `int` declarations. To fix it, run:
```sh
sed -i "1083s/^/int /" configure
```
.
P.S.: Also look at updating the `config.guess` and `config.sub` files as those are obsolete and could fail you. Run:
```sh
rm config.guess && rm config.sub && wget https://git.savannah.gnu.org/cgit/config.git/plain/config.guess && wget https://git.savannah.gnu.org/cgit/config.git/plain/config.sub
```
to fix the issue. 
