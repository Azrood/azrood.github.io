We scan with nmap and find these ports:
* 80/tcp   open  http         Apache httpd 2.4.46 ((Win64) OpenSSL/1.1.1j PHP/7.3.27)
* 135/tcp  open  msrpc        Microsoft Windows RPC
* 139/tcp  open  netbios-ssn  Microsoft Windows netbios-ssn
* 443/tcp  open  ssl/http     Apache httpd 2.4.46 (OpenSSL/1.1.1j PHP/7.3.27)
* 445/tcp  open  microsoft-ds Microsoft Windows 7 - 10 microsoft-ds (workgroup: WORKGROUP)
* 3306/tcp open  mysql?
* 5000/tcp open  http         Apache httpd 2.4.46 (OpenSSL/1.1.1j PHP/7.3.27)

1. Admin/ login

|Service|username|password|notes|
|---|---|---|--|
|love.htb/admin |admin|@LoveIsInTheAir!!!!||
|love.htb/index.php|7PUMHakwWT8pE1B|password||