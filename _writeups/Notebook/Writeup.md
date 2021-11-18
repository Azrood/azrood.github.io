# Main points:
* [JWT](https://jwt.io/)
* [Writeup#Priv Esc|Docker escape](/assets/Boxes/Notebook/Writeup%23Priv%20Esc%7CDocker%20escape)
* [CVE-2019-5736](https://www.openwall.com/lists/oss-security/2019/02/13/3)
* /var/backups

# Writeup
We have these open ports:

* 22/tcp open  ssh     syn-ack OpenSSH 7.6p1 Ubuntu 4ubuntu0.3 (Ubuntu Linux; protocol 2.0)
* 80/tcp open  http    syn-ack nginx 1.14.0 (Ubuntu)

We see there is a login page, we can register by going to the /register page
we create `user: user`
Let's look at the request in burp suite:
![Pasted image 20210709201119.png|1000](/assets/Pasted%20image%2020210709201119.png%7C1000)
The `auth` cookie is a JWT, let's look at it
![Pasted image 20210709201439.png](/assets/Pasted%20image%2020210709201439.png)

If we want to forge a new JWT with `admin_cap` set to `1` we need to private key, or we can use our own private key and replace the URL in the `kid` field.

Let's generate a private key with openssl
`openssl genrsa -out key.pem 2048`
We copy our private key to https://jwt.io to forge a new JWT, and we change the kid to point to our URL (don't forget to setup a Python HTTP server) and set `admin_cap` to `1`

![Pasted image 20210709201933.png](/assets/Pasted%20image%2020210709201933.png)

We copy the new token and replace it in the burp request and go to /admin page.

There, we see 2 buttons. 
![Pasted image 20210709202422.png](/assets/Pasted%20image%2020210709202422.png)

Our new JWT works, let's replace it in the cookie tab of our browser so we dont have to replace it every time we make a request
![Pasted image 20210709203845.png](/assets/Pasted%20image%2020210709203845.png)

Let's upload a php webshell, and setup a netcat listener in our machine.

After uploading our php webshell, we can query it to catch a shell
![Pasted image 20210709202851.png](/assets/Pasted%20image%2020210709202851.png)

Now we have a shell
![Pasted image 20210709202950.png](/assets/Pasted%20image%2020210709202950.png)

We see there is a user `noah`, we need to connect to it so we need the password.

Let's go back to the admin page and see if there are any notes.

![Pasted image 20210709203716.png](/assets/Pasted%20image%2020210709203716.png)
Interesting.

Let's check the 1st note:
![Pasted image 20210709203922.png](/assets/Pasted%20image%2020210709203922.png)
Well, we already exploited that. Let's continue.

![Pasted image 20210709204014.png](/assets/Pasted%20image%2020210709204014.png)
There are regular backups then, I thought it was related to a database, but then i saw a folder `backups` in `/var`

![Pasted image 20210709204406.png](/assets/Pasted%20image%2020210709204406.png)

let's download and unpack the `home.tar.gz`
![Pasted image 20210709204535.png](/assets/Pasted%20image%2020210709204535.png)

We have a backup of the user noah
![Pasted image 20210709204653.png](/assets/Pasted%20image%2020210709204653.png)

We have noah private key, we can use it to connect to ssh.
![Pasted image 20210709204737.png](/assets/Pasted%20image%2020210709204737.png)

We have ssh access, and we get the user flag
![Pasted image 20210709204827.png](/assets/Pasted%20image%2020210709204827.png)

## Priv Esc
When listing with `sudo -l` what we can run as sudo, we find 
![Pasted image 20210709204949.png](/assets/Pasted%20image%2020210709204949.png)

So we can run docker as root, we need a docker escape to get root in the system.
We check the version to look for a possible exploit
![Pasted image 20210709205043.png](/assets/Pasted%20image%2020210709205043.png)

We find this [Go implementation of PoC for CVE for Docker escape](https://github.com/Frichetten/CVE-2019-5736-PoC)

We download the file
change the payload to get a root reverse shell
![Pasted image 20210709211243.png](/assets/Pasted%20image%2020210709211243.png)
and compile it
```bash
go build main.go 
```
and upload the `main` file 

chmod and execute the `main` file while running docker exec with sudo
```
sudo /usr/bin/docker exec -it webapp-dev01 /bin/sh
```
### You need to run the exploit inside the docker container
![Pasted image 20210709221024.png](/assets/Pasted%20image%2020210709221024.png)

In another ssh session, run the docker command again 
![Pasted image 20210709221045.png](/assets/Pasted%20image%2020210709221045.png)

and you get a shell in your netcat listener (repeat a few times because it's unstable)
![Pasted image 20210709221127.png](/assets/Pasted%20image%2020210709221127.png)
## root hash
\$6\$OZ7vREXE$yXjcCfK6rhgAfN5oLisMiB8rE/uoZb7hSqTOYCUTF8lNPXgEiHi7zduz1mrTWtFnhKOCZA9XZu12osORyYnKF.